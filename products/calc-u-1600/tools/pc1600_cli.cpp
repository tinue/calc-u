// Headless CLI for the PC-1600. The --preset form runs a full PC-1600
// through the machine interface and a `.pc1600` preset (below). The plain form boots
// only the SC7852 from romI-0 + romII-0 (no LH5803), runs a fixed cycle
// budget and reports a PC-visit histogram that shows convergence onto a
// small repeating loop, like pc1500_cli.
//
// Usage: pc1600_cli <romI-0-file> <romII-0-file> [maxCycles]
//        pc1600_cli --preset <preset-file.pc1600> [maxCycles] [--dump-basic] [--modules-dir <dir>] [--save-dir <dir>] [--wav <out.wav>] [--tape-in <in.wav>] [--tape-out <out.wav>] [--dump-mem <addr>,<len>] [--inspect <view>] [--rom new|old] [--ce1600p-rom new|old] [--lcd-png <out.png>] [--lcd-text <out.txt|->]
//
// A preset is checked with `calcu --product 1600 check-preset` (LIB-6).
//
// The --preset form loads the confirmed PC-1600 ROM set from roms/ (same
// names pc1600_preset_tests.cpp uses), builds a PC-1600 from the catalogue, and
// applies a `.pc1600` preset via the library's loader (CliPresetRun) -- the SC7852-only
// histogram mode below is skipped. --dump-basic then prints the BASIC
// program pointers and the raw program-area bytes (read-only), the oracle
// the fast BASIC loader is checked against.
//
// --rom and --ce1600p-rom: from M-46 the preset names its variants (model:
// PC-1600:old, printer: CE-1600P:old); only `new` is taken here. Formerly:
// --rom new|old (--preset only) overrides the preset's PC-1600 ROM version
// (`model: PC-1600:new|old`, default new).
// --ce1600p-rom new|old (--preset only) overrides the preset's CE-1600P ROM
// version (`plotter: CE-1600P:new|old`, default new); independent of --rom.
//
// `- saveas:` steps write cards and floppies like the GUI (Core/Preset/
// PresetMedia.hpp): the `file:<path>` form writes that file; a by-name
// save goes to --save-dir <dir> (<dir>/<name>.card.yaml / .floppy.yaml) and
// fails without it. A by-name save refuses a name of the first
// --modules-dir (the bundled cards) and never overwrites a template.
//
// CE-158 (--preset only, a preset with `interface: CE-158`): --ce158-pty,
// --ce158-rx <file>, --ce158-rx-hold <n>, --ce158-tx <file> -- same as
// pc1500_cli (see its header). --run-after <tstates> keeps the machine
// running that long after the preset script (e.g. to finish a serial
// exchange with a --ce158-rx peer).
//
// --lcd-png <out.png> (--preset only) writes the LCD, as Copy Screen does,
// once the preset (and any --run-after) has finished. --lcd-text <out.txt>
// (or - for stdout) writes it as text at the same point (Core/Display/
// LcdText.hpp): the rows, the lit status symbols, and the number of cells
// that are not text.
//
// --wav <out.wav> (--preset only) records the buzzer (OPC 18H, see
// PiezoSampler.hpp) while the preset script runs, as 48 kHz mono 16-bit
// PCM -- as the host hears it, i.e. through the PC-1600 transducer model.
//
// --tape-in <in.wav> / --tape-out <out.wav> (--preset only) put a cassette
// into the recorder behind the CE-1600P (Core/Tape/TapeDeck.hpp) before the
// preset runs: a WAV to play for the preset's CLOAD, or a blank tape whose
// recording (the preset's CSAVE) is saved to <out.wav> when the motor stops. The
// tape moves only while the CE-1600P's remote relay runs it. The preset
// needs `plotter: CE-1600P`.
//
// --dump-mem <addr>,<len> (--preset only; repeatable) prints that much of
// the Z-80's view of memory as hex once the preset has run, e.g.
// --dump-mem 0xD000,300 after a CLOAD M.

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "../core/Audio/WavFile.hpp"
#include "../core/CPU/SC7852/SC7852.hpp"
#include "../core/Display/DisplayOutputText.hpp"
#include "../core/Display/LcdScreenshot.hpp"
#include "../core/Machine/LegacyCalls.hpp"
#include "../core/PC1600/PC1600Bank.hpp"
#include "../core/PC1600/PC1600Memory.hpp"
#include "Ce158CliPeer.hpp"
#include "CliCommon.hpp"
#include "CliPresetRun.hpp"

namespace {
// --preset: the preset loaded by the library's loader (CliPresetRun), then
// the tool's own dumps of the machine it built.
int runPreset(const std::string& presetPath, uint64_t maxCycles, bool dumpBasic,
              const std::string& moduleDir, const std::vector<std::string>& extraModuleDirs,
              const std::string& wavPath, const std::string& romOverride,
              const std::string& ce1600pRomOverride, const std::string& saveDir,
              Ce158CliPeer& ce158Peer, uint64_t runAfter, const std::string& lcdPng,
              const std::string& lcdTextPath, const cli::TapeOptions& tape, const cli::MemDumps& memDumps,
              const cli::InspectViews& inspectViews) {
    (void)maxCycles;
    (void)moduleDir;
    (void)extraModuleDirs;
    (void)saveDir;
    // The ROMs are the preset's own (`model: PC-1600:old`, `printer:
    // CE-1600P:old`): an override other than the default is refused (M-46).
    if ((!romOverride.empty() && romOverride != "new") || (!ce1600pRomOverride.empty() && ce1600pRomOverride != "new")) {
        std::fprintf(stderr, "--rom and --ce1600p-rom are no longer taken: name the variant in the preset "
                             "(model: PC-1600:old, printer: CE-1600P:old)\n");
        return 1;
    }
    // --preset: through the library's loader in the harness (adapter
    // CliPresetRun, until M-51), to the idle machine.
    std::string error;
    std::unique_ptr<cli::CliPresetRun> run = cli::CliPresetRun::load(presetPath, &error);
    if (!run) {
        std::fprintf(stderr, "failed to apply preset '%s': %s\n", presetPath.c_str(), error.c_str());
        return 1;
    }
    cli::Machine* loadedMachine = run->machine();
    if (!loadedMachine || loadedMachine->model().value != sharp::kPC1600) {
        std::fprintf(stderr, "preset '%s' is not a PC-1600 preset\n", presetPath.c_str());
        return 1;
    }
    cli::Machine& machine = *loadedMachine;
    // --wav: the sampler only buffers ~1 s, so drain it from the core's
    // yield hook while the run after the load goes on (sharp::legacy).
    std::vector<int16_t> wav;
    auto drainWav = [&] {
        int16_t chunk[4096];
        size_t n;
        while ((n = machine.takeSound(chunk)) > 0) wav.insert(wav.end(), chunk, chunk + n);
    };
    if (!wavPath.empty()) sharp::legacy::setYieldHook(machine, drainWav, machine.descriptor().tickRate / 20);
    if (!ce158Peer.open()) return 1;
    if (!tape.arm(machine)) return 1;
    // The CE-158's peer is pumped between slices of the run after the load.
    const uint64_t slice = machine.descriptor().tickRate / 100;
    const auto pump = [&] { ce158Peer.pump(machine); };
    auto pk = [&](uint16_t a) { return cli::peek(machine, a); };
    std::printf("Preset '%s' applied successfully.\n", presetPath.c_str());
    if (runAfter) cli::advanceSliced(machine, runAfter, slice, pump);
    if (!lcdPng.empty()) {
        std::string pngError;
        if (!writeLcdScreenshotPng(lcdBitmapOf(machine.readDisplay()), kPC1600ScreenMm, lcdPng, &pngError)) {
            std::fprintf(stderr, "failed to write '%s': %s\n", lcdPng.c_str(), pngError.c_str());
            return 1;
        }
    }
    if (!lcdTextPath.empty()) {
        std::string textError;
        if (!writeLcdTextReport(displayText(machine), lcdTextPath, &textError)) {
            std::fprintf(stderr, "failed to write '%s': %s\n", lcdTextPath.c_str(), textError.c_str());
            return 1;
        }
    }
    if (!cli::writePaperIfAsked(machine)) return 1;
    cli::printCe150Report(machine);
    if (!ce158Peer.report(machine)) return 1;
    if (!tape.finish(machine)) return 1;

    if (!wavPath.empty()) {
        sharp::legacy::setYieldHook(machine, {}, 0);
        drainWav();
        if (!writeWavMono16(wavPath, wav, sharp::kSoundRate)) {
            std::fprintf(stderr, "failed to write '%s'\n", wavPath.c_str());
            return 1;
        }
        std::printf("Wrote %zu samples (%.2f s) of buzzer audio to %s\n", wav.size(),
                    static_cast<double>(wav.size()) / sharp::kSoundRate, wavPath.c_str());
    }

    cli::printMemDumps(machine, memDumps);
    cli::printInspect(machine, inspectViews);

    if (dumpBasic) {
        auto be16 = [&](uint16_t a) { return static_cast<uint16_t>((pk(a) << 8) | pk(static_cast<uint16_t>(a + 1))); };
        auto le16 = [&](uint16_t a) { return static_cast<uint16_t>(pk(a) | (pk(static_cast<uint16_t>(a + 1)) << 8)); };
        uint16_t st = be16(0xF865), end = be16(0xF867), edt = be16(0xF869);
        // F865/F867/F869 hold LH5803-side addresses; the LH5803's
        // $0000-$7FFF aliases the SC7852's $8000-$FFFF, so the Z-80 address
        // (the loader memory's view) is value + $8000: $C0C5 stock ($40C5),
        // $80C5 with the program in a Slot-1/Slot-2 RAM module ($00C5).
        auto toZ80 = [](uint16_t a) -> uint16_t {
            return a < 0x8000 ? static_cast<uint16_t>(a + 0x8000) : a;
        };
        uint16_t stZ = toZ80(st), endZ = toZ80(end);
        std::printf("--- BASIC pointers (PC-1600) ---\n");
        std::printf("BASPRG_ST  $F865 = $%04X  (Z-80 $%04X)\n", st, stZ);
        std::printf("BASPRG_END $F867 = $%04X  (Z-80 $%04X, len = %d)\n", end, endZ,
                    static_cast<int>(end) - static_cast<int>(st));
        std::printf("BASPRG_EDT $F869 = $%04X\n", edt);
        std::printf("F5CF (LE, RAM base) = $%04X   F89D (LE) = $%04X\n", le16(0xF5CF), le16(0xF89D));
        std::printf("VARIABLE_PTR $F899 = $%04X\n", be16(0xF899));
        std::printf("--- program area $%04X..$%04X+16 ---\n", stZ, endZ);
        for (uint32_t a = stZ; a <= static_cast<uint32_t>(endZ) + 16 && a <= 0xFFFF; a += 16) {
            std::printf("%04X: ", a);
            for (int i = 0; i < 16; i++) std::printf("%02X ", pk(static_cast<uint16_t>(a + i)));
            std::printf(" |");
            for (int i = 0; i < 16; i++) {
                uint8_t b = pk(static_cast<uint16_t>(a + i));
                std::printf("%c", (b >= 0x20 && b < 0x7F) ? static_cast<char>(b) : '.');
            }
            std::printf("|\n");
        }
    }
    return 0;
}
} // namespace

int main(int argc, char** argv) {
    if (argc >= 3 && std::strcmp(argv[1], "--preset") == 0) {
        uint64_t maxCycles = 2'000'000ull;
        bool dumpBasic = false;
        std::string wavPath;
        std::string romOverride;
        std::string ce1600pRomOverride;
        std::string saveDir;
        Ce158CliPeer ce158Peer;
        uint64_t runAfter = 0;
        std::string lcdPng;
        std::string lcdTextPath;
        cli::TapeOptions tape;
        cli::MemDumps memDumps;
        cli::InspectViews inspectViews;
        std::string moduleDir = "frontends/qt6/resources/cards";
        std::vector<std::string> extraModuleDirs;  // 2nd+ `--modules-dir`, searched after `moduleDir`
        bool moduleDirSet = false;
        for (int i = 3; i < argc; ++i) {
            if (std::strcmp(argv[i], "--dump-basic") == 0) dumpBasic = true;
            else if (std::strcmp(argv[i], "--wav") == 0 && i + 1 < argc) wavPath = argv[++i];
            else if (std::strcmp(argv[i], "--rom") == 0 && i + 1 < argc) romOverride = argv[++i];
            else if (std::strcmp(argv[i], "--ce1600p-rom") == 0 && i + 1 < argc) ce1600pRomOverride = argv[++i];
            else if (std::strcmp(argv[i], "--save-dir") == 0 && i + 1 < argc) saveDir = argv[++i];
            else if (std::strcmp(argv[i], "--run-after") == 0 && i + 1 < argc) runAfter = std::strtoull(argv[++i], nullptr, 10);
            else if (std::strcmp(argv[i], "--lcd-png") == 0 && i + 1 < argc) lcdPng = argv[++i];
            else if (std::strcmp(argv[i], "--lcd-text") == 0 && i + 1 < argc) lcdTextPath = argv[++i];
            else if (tape.parseArg(argc, argv, i)) {}
            else if (std::strcmp(argv[i], "--dump-mem") == 0 && i + 1 < argc) {
                if (!cli::parseMemDump(argv[++i], &memDumps)) return 1;
            }
            else if (std::strcmp(argv[i], "--inspect") == 0 && i + 1 < argc) {
                if (!cli::parseInspect(argv[++i], &inspectViews)) return 1;
            }
            else if (ce158Peer.parseArg(argc, argv, i)) {}
            else if (std::strcmp(argv[i], "--modules-dir") == 0 && i + 1 < argc) {
                if (!moduleDirSet) { moduleDir = argv[++i]; moduleDirSet = true; }
                else               { extraModuleDirs.push_back(argv[++i]); }
            }
            else maxCycles = std::strtoull(argv[i], nullptr, 10);
        }
        return runPreset(argv[2], maxCycles, dumpBasic, moduleDir, extraModuleDirs, wavPath, romOverride,
                         ce1600pRomOverride, saveDir, ce158Peer, runAfter, lcdPng, lcdTextPath, tape, memDumps, inspectViews);
    }
    if (argc < 3) {
        std::fprintf(stderr, "usage: %s <romI-0-file> <romII-0-file> [maxCycles]\n", argv[0]);
        std::fprintf(stderr, "       %s --check-preset <preset-file>...\n", argv[0]);
        std::fprintf(stderr, "       %s --preset <preset-file.pc1600> [maxCycles] [--dump-basic] [--wav <out.wav>] [--tape-in <in.wav>] [--tape-out <out.wav>] [--dump-mem <addr>,<len>] [--inspect <view>] [--rom new|old] [--ce1600p-rom new|old] [--lcd-png <out.png>] [--lcd-text <out.txt|->]\n", argv[0]);
        return 1;
    }
    uint64_t maxCycles = 2'000'000ull;
    if (argc >= 4) maxCycles = std::strtoull(argv[3], nullptr, 10);

    std::vector<uint8_t> lower, upper;
    if (!cli::readFileExact(argv[1], 16384, &lower)) {
        std::fprintf(stderr, "failed to read '%s' (expected exactly 16384 bytes)\n", argv[1]);
        return 1;
    }
    if (!cli::readFileExact(argv[2], 16384, &upper)) {
        std::fprintf(stderr, "failed to read '%s' (expected exactly 16384 bytes)\n", argv[2]);
        return 1;
    }

    PC1600Bank bank;
    PC1600Memory mem(bank);
    if (!mem.loadBank0(lower.data(), lower.size(), upper.data(), upper.size())) {
        std::fprintf(stderr, "loadBank0 failed\n");
        return 1;
    }
    SC7852 cpu(mem);
    cpu.reset();
    std::printf("Reset -> PC=0x%04X\n", cpu.pc());

    static std::array<uint64_t, 65536> pcHistogram{};
    uint64_t consumed = 0;
    uint64_t steps = 0;
    while (consumed < maxCycles) {
        uint16_t pcBefore = cpu.pc();
        int c = cpu.step();
        if (c == 0) {
            std::printf("CPU halted (HALT) at PC=0x%04X after %llu cycles with no pending interrupt\n",
                        pcBefore, (unsigned long long)consumed);
            break;
        }
        pcHistogram[pcBefore]++;
        consumed += static_cast<uint64_t>(c);
        steps++;
    }

    std::printf("Ran %llu instructions, %llu cycles\n", (unsigned long long)steps, (unsigned long long)consumed);

    uint64_t topCount = 0;
    uint32_t topPC = 0;
    uint32_t distinctPCs = 0;
    for (uint32_t pc = 0; pc < pcHistogram.size(); pc++) {
        if (pcHistogram[pc] == 0) continue;
        distinctPCs++;
        if (pcHistogram[pc] > topCount) { topCount = pcHistogram[pc]; topPC = pc; }
    }
    std::printf("Most-visited PC: 0x%04X (%llu visits, %u distinct PCs total)\n",
                topPC, (unsigned long long)topCount, distinctPCs);
    if (distinctPCs < 500) {
        std::printf("Signature matches a converged small loop (stable, bounded execution).\n");
    }

    std::printf("Final registers: AF=0x%04X BC=0x%04X DE=0x%04X HL=0x%04X SP=0x%04X PC=0x%04X IM=%d IFF1=%d\n",
                cpu.af(), cpu.bc(), cpu.de(), cpu.hl(), cpu.sp(), cpu.pc(), cpu.im(), cpu.iff1());
    return 0;
}
