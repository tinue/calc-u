#include "PresetRunner.hpp"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iterator>
#include <optional>
#include <variant>

#include "../Connector/MemoryCardCatalog.hpp"
#include "../Connector/SoftwareDefinedCard.hpp"
#include "../BasicTyping.hpp"
#include "../FileIO.hpp"
#include "../Loaders.hpp"
#include "../Machine/SharpRoms.hpp"
#include "../TraceTypes.hpp"

std::vector<std::string> presetModuleDirs(const std::string& moduleDir,
                                          const std::vector<std::string>& extraModuleDirs) {
    std::vector<std::string> dirs{moduleDir};
    dirs.insert(dirs.end(), extraModuleDirs.begin(), extraModuleDirs.end());
    return dirs;
}

std::unique_ptr<ExpansionCard> makePresetModuleCard(const std::string& specFile, const std::string& specName,
                                                    const std::string& romFile,
                                                    const std::vector<std::string>& moduleDirs, CardHost host,
                                                    std::string* resolvedPath, std::string* error,
                                                    const calcu::roms::RomSet* romSet) {
    std::string specPath = specFile;
    std::optional<sharp::RomSetModule> fromSet;
    if (specPath.empty() && romSet) fromSet = sharp::romSetModule(*romSet, specName);
    if (specPath.empty() && !fromSet && !resolveModuleSpecByName(moduleDirs, specName, &specPath, error)) return nullptr;
    std::vector<uint8_t> rom;
    if (!romFile.empty() && !readWholeFile(romFile, &rom)) {
        *error = "could not read " + romFile;
        return nullptr;
    }
    const std::vector<uint8_t>* romOverride = romFile.empty() ? nullptr : &rom;
    std::unique_ptr<ExpansionCard> card =
        fromSet ? makeSoftwareDefinedCardFromText(fromSet->text, fromSet->fileName, host, error, romOverride)
                : makeSoftwareDefinedCard(specPath, host, error, romOverride);
    if (!card) {
        if (!romFile.empty()) *error = romFile + ": " + *error;
        return nullptr;
    }
    *resolvedPath = specPath;
    return card;
}

namespace {

constexpr uint64_t kSecondsPerHour = 3600;

// Rough line count for a progress-log string only (blank lines included,
// trailing newline or not). The authoritative typed-line accounting is the
// typer's own.
int countLines(const std::string& text) {
    if (text.empty()) return 0;
    int n = static_cast<int>(std::count(text.begin(), text.end(), '\n'));
    return text.back() == '\n' ? n : n + 1;
}

job::Task<bool> runSteps(PresetMachine& machine, const std::vector<PresetStep>& steps, std::string* error,
                         const PresetLogFn& log, const std::string& traceDir, const PresetSaveAsFn& onSaveAs,
                         const PresetTraceFn& traceHook) {
    const double hz = machine.cyclesPerSecond();
    for (const PresetStep& step : steps) {
        switch (step.kind) {
            case PresetStep::Kind::Key:
                if (!co_await machine.key(step.text, error)) {
                    if (log) log("  key: " + step.text + " FAILED: " + *error);
                    co_return false;
                }
                if (log) log("  key: " + step.text + machine.stepTag());
                break;
            case PresetStep::Kind::Type:
                if (log) log("  type: \"" + step.text + "\" ...");
                if (!co_await machine.typeLine(step.text, error)) {
                    if (log) log("  type: \"" + step.text + "\" FAILED: " + *error);
                    co_return false;
                }
                if (log) log("  type: \"" + step.text + "\" done" + machine.stepTag());
                break;
            case PresetStep::Kind::Wait: {
                if (step.waitSeconds < 0) {
                    // Parameterless `- wait:` -- block until the interpreter is
                    // back in its idle command loop (a long RUN / plot done).
                    // A short unconditional lead-in first, so a RUN / plot that
                    // hasn't spun up yet (interpreter momentarily back at the
                    // prompt right after the last typed step) can't trip an
                    // instant false "idle".
                    const uint64_t leadIn = static_cast<uint64_t>(hz / 2);                   // 0.5 s emulated
                    const uint64_t untilIdleCap = static_cast<uint64_t>(hz) * kSecondsPerHour;  // 1 h emulated
                    uint64_t spent = co_await machine.runCycles(leadIn);
                    spent += co_await machine.waitUntilBasicIdle(untilIdleCap);
                    if (log) {
                        char secs[24];
                        std::snprintf(secs, sizeof(secs), "%.1f", spent / hz);
                        log("  wait: (until idle, " + std::string(secs) +
                            (spent >= untilIdleCap ? "s -- CAP HIT)" : "s)") + machine.stepTag());
                    }
                } else {
                    co_await machine.runCycles(static_cast<uint64_t>(step.waitSeconds * hz));
                    if (log) {
                        char secs[16];
                        std::snprintf(secs, sizeof(secs), "%.3g", step.waitSeconds);
                        log("  wait: " + std::string(secs) + "s" + machine.stepTag());
                    }
                }
                break;
            }
            case PresetStep::Kind::Trace: {
                // L-A3: the app's trace session (LIB-10 §10.7).
                if (traceHook) {
                    std::string traceError;
                    if (!traceHook(step.text, &traceError)) {
                        *error = "trace: " + traceError;
                        if (log) log("  trace: FAILED: " + traceError);
                        co_return false;
                    }
                    if (log) log(step.text.empty() ? "  trace: stopped" : "  trace: started -> " + step.text);
                    break;
                }
                // Empty text -> stop; otherwise (re)start a capture to
                // <traceDir>/<text>. Starting a new one first closes any
                // open one; one left running is closed
                // by runPresetSections()'s TraceCloser guard.
                if (machine.cpuTraceActive()) machine.endCpuTrace();
                if (step.text.empty()) {
                    if (log) log("  trace: stopped");
                    break;
                }
                const std::string path = traceDir + "/" + step.text;
                errno = 0;
                std::FILE* fh = std::fopen(path.c_str(), "wb");
                if (!fh) {
                    *error = "could not open trace file: " + path + " (" + std::strerror(errno) + ")";
                    if (log) log("  trace: FAILED to open " + path + ": " + std::strerror(errno));
                    co_return false;
                }
                machine.beginCpuTrace(fh, TRACE_FULL);
                if (log) log("  trace: started -> " + path);
                break;
            }
            case PresetStep::Kind::Screenshot: {
                // PNG of the dot matrix, as it stands right now, into the
                // trace directory (same image as the GUI's Copy Screen).
                const std::string path = traceDir + "/" + step.text;
                std::string writeError;
                if (!machine.writeScreenshot(path, &writeError)) {
                    *error = "screenshot: " + writeError;
                    if (log) log("  screenshot: FAILED: " + writeError);
                    co_return false;
                }
                if (log) log("  screenshot: -> " + path);
                break;
            }
            case PresetStep::Kind::Expect: {
                const LcdText screen = machine.lcdText();
                if (!screen.contains(step.text)) {
                    *error = "expect: \"" + step.text + "\" is not on the LCD:\n" + screen.report();
                    if (log) log("  expect: \"" + step.text + "\" NOT FOUND" + machine.stepTag());
                    co_return false;
                }
                if (log) log("  expect: \"" + step.text + "\" found");
                break;
            }
            case PresetStep::Kind::SyncClock: {
                const std::tm t = machine.syncClock();
                char stamp[32];
                std::strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", &t);
                if (log) log(std::string("  syncclock: -> ") + stamp);
                break;
            }
            case PresetStep::Kind::SaveAs:
                if (!runPresetSaveAsStep(step, onSaveAs, log, error)) co_return false;
                break;
        }
    }
    co_return true;
}

// The bytes of a program file, or false with `error` (tag-prefixed).
bool readProgramFile(const PresetProgram& program, const std::string& tag, std::vector<uint8_t>* bytes,
                     std::string* error) {
    errno = 0;
    std::ifstream in(program.path, std::ios::binary);
    if (!in) {
        // errno detail -- "Permission denied" (a sandbox-denied path, e.g. a
        // preset's sibling binary the file picker never granted access to)
        // and "No such file or directory" (a genuinely missing/mistyped
        // path) look identical from `!in` alone otherwise.
        *error = tag + "could not read program file: " + program.path + " (" + std::strerror(errno) + ")";
        return false;
    }
    bytes->assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    // EC-4: a WAV gives its first file.
    if (auto first = sharp::loaders::image(*bytes, 0, nullptr)) *bytes = std::move(*first);
    return true;
}

// A machine-code `program:` block, through the one plan and writer of
// C1600-2 (sharp::loaders::loadCode): the file may carry a CE-158 or PC-1600
// header (load address, length, an auto-run address); the preset's
// `address:` / `length:` are given by hand and override the header's
// fields; a headerless file needs `address:`. On the PC-1600 the machine's
// MODE and TITLE decide the CPU and the target. A refusal is the menu's
// sentence after the section's tag and the file (AC-25). A non-zero
// auto-run address makes the loader type the starting command afterwards
// and wait for the interpreter to come back (C1600-2 AC-13; today's
// typeLine and waitUntilBasicIdle until M-46).
job::Task<bool> loadBinaryProgram(PresetMachine& machine, const PresetProgram& program, std::string tag,
                                  const PresetLogFn& log, std::string* error) {
    std::vector<uint8_t> bytes;
    if (!readProgramFile(program, tag, &bytes, error)) co_return false;
    sharp::loaders::CodeRequest request;
    if (program.hasAddress) request.address = program.address;
    if (program.hasLength) request.length = program.length;
    auto loaded = sharp::loaders::loadCode(machine.loaderMemory(), bytes, request);
    if (const std::string* refused = std::get_if<std::string>(&loaded)) {
        *error = tag + sharp::loaders::refusal(program.path, *refused);
        co_return false;
    }
    const sharp::loaders::CodeLoaded& code = std::get<sharp::loaders::CodeLoaded>(loaded);
    if (log) {
        const bool pc1600 = !code.area.empty();
        const std::string slot = pc1600 ? ", " + code.area + (code.bank != 0 ? ", bank " + std::to_string(code.bank) : "") +
                                              (code.cpu == machinecode::Cpu::LH5803 ? ", LH5803 address" : "")
                                        : "";
        char range[40];
        std::snprintf(range, sizeof(range), "$%04X..$%04X", code.address,
                      static_cast<uint32_t>(code.address + code.length - 1));
        log(tag + "binary " + program.path + " (" + std::to_string(code.length) + " bytes" +
            (code.header ? ", header" : "") + slot + ") -> " + range + machine.stepTag());
    }

    if (code.headerStart) {
        // The starting command of the reply (AC-11): `CALL #<bank>,&<addr>`
        // for code in banks 1-3, `XCALL` for LH5801 code, `CALL &<addr>`
        // otherwise.
        const std::string line = code.advice.callCommand;
        std::string typeError;
        if (!co_await machine.typeLine(line, &typeError)) {
            *error = tag + "auto-run '" + line + "' failed: " + typeError;
            co_return false;
        }
        co_await machine.waitUntilBasicIdle(static_cast<uint64_t>(machine.cyclesPerSecond()) * kSecondsPerHour);
        if (log) log("  auto-run " + line + machine.stepTag());
    }
    co_return true;
}

job::Task<bool> runProgram(PresetMachine& machine, const PresetProgram& program, std::string tag,
                           PresetLoadResult* result, const PresetLogFn& log, PresetTyping typing) {
    switch (program.format) {
        case PresetProgram::Format::BasicText: {
            if (log) log(tag + "program (typed, " + std::to_string(countLines(program.text)) + " lines)");
            // C1600-2 AC-7: through the typing engine inside the preset's
            // operation; without a session the old typer (until M-46).
            BasicTypeResult typed =
                typing.present()
                    ? co_await sharp::loaders::typeProgram(*typing.ctx, *typing.engine, *typing.machine, program.text)
                    : co_await machine.typeBasicProgramWithoutSession(program.text);
            for (const std::string& rejected : typed.rejectedLines) {
                result->rejectedBasicLines.push_back(rejected);
                if (log) log("  REJECTED: " + rejected);
            }
            if (!typed.ok) {
                result->error = typed.error;
                if (log) log("  program typing FAILED: " + typed.error);
                co_return false;
            }
            if (log) log("  program typed OK" + machine.stepTag());
            co_return true;
        }
        case PresetProgram::Format::BasicBinary: {
            if (log) log(tag + "program (BASIC, " + program.path + ")");
            std::vector<uint8_t> bytes;
            if (!readProgramFile(program, tag, &bytes, &result->error)) {
                if (log) log("  " + result->error);
                co_return false;
            }
            const sharp::loaders::BasicLoaded loaded = sharp::loaders::loadBasic(machine.loaderMemory(), bytes);
            if (!loaded.ok) {
                result->error = tag + sharp::loaders::refusal(program.path, loaded.refusal);
                if (log) log("  " + result->error);
                co_return false;
            }
            if (log) {
                char n[96];
                std::snprintf(n, sizeof(n), "  program loaded OK: $%04X..$%04X", loaded.start, loaded.end);
                log(std::string(n) + machine.stepTag());
            }
            co_return true;
        }
        case PresetProgram::Format::Binary:
            if (!co_await loadBinaryProgram(machine, program, tag, log, &result->error)) {
                if (log) log("  " + result->error);
                co_return false;
            }
            co_return true;
    }
    co_return false;
}

}  // namespace

job::Task<void> runPresetSectionsJob(PresetMachine& machine, const PresetFile& preset, PresetLoadResult* result,
                                     PresetLogFn log, std::string traceDir, PresetSaveAsFn onSaveAs,
                                     PresetTyping typing) {
    // A trace started by a `- trace:` step and never explicitly stopped is
    // closed (SESSION_END written, file closed) when the sections are done,
    // by ANY path. The app's trace (traceHook, L-A3) is the trace session's:
    // the end of the operation ends it.
    struct TraceCloser {
        PresetMachine& m;
        ~TraceCloser() {
            if (m.cpuTraceActive()) m.endCpuTrace();
        }
    } traceCloser{machine};

    const uint64_t programIdleCap = static_cast<uint64_t>(machine.cyclesPerSecond()) * kSecondsPerHour;
    const int sectionCount = static_cast<int>(preset.sections.size());
    int sectionNo = 0;
    for (const PresetSection& section : preset.sections) {
        sectionNo++;
        const std::string tag =
            "section " + std::to_string(sectionNo) + "/" + std::to_string(sectionCount) + ": ";
        if (section.kind == PresetSection::Kind::Keys) {
            const size_t n = section.keys.size();
            if (log) log(tag + "keys (" + std::to_string(n) + (n == 1 ? " step)" : " steps)"));
            if (!co_await runSteps(machine, section.keys, &result->error, log, traceDir, onSaveAs, preset.traceHook)) co_return;
            continue;
        }
        // A preceding `type:` step returns right after its ENTER, so a
        // command it started (e.g. a SAVE) may still be running -- let it
        // finish before this section types or pokes a program into memory
        // underneath it.
        co_await machine.waitUntilBasicIdle(programIdleCap);
        if (!co_await runProgram(machine, section.program, tag, result, log, typing)) co_return;
    }

    result->ok = true;
    if (log) log("preset applied OK" + machine.stepTag());
}

void runPresetSections(PresetMachine& machine, const PresetFile& preset, PresetLoadResult* result,
                       const PresetLogFn& log, const std::string& traceDir, const PresetSaveAsFn& onSaveAs) {
    job::runToCompletion(runPresetSectionsJob(machine, preset, result, log, traceDir, onSaveAs));
}
