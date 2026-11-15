// Flat C API over the TI machine for the Python headless harness
// (tools/headless/ti59.py, loaded via ctypes). Built by
// tools/headless/build.sh into tools/headless/build/libti59headless.dylib.
//
// Since C59-1 (M-19) it drives the machine through the machine interface
// (calcu::machine::Machine, built with tiCatalogue()) and reaches what the
// interface does not have yet through the adapters of core/TIAdapters.hpp.
// The surface is deliberately small: lifecycle, keys by name, single steps,
// and a bulk "run n steps and record each one" call so tracing a keystroke
// does not pay a ctypes round trip per instruction.

#include "../../app/MachineModelInfo.hpp"
#include "../../app/ModuleCatalog.hpp"
#include "../../app/ProductRoms.hpp"
#include "../../core/TIAdapters.hpp"
#include "../../core/TICatalogue.hpp"
#include "machine/Machine.hpp"
#include "roms/Bundle.hpp"
#include <cstring>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include <variant>

namespace {

using calcu::machine::Machine;

// One recorded instruction: identity of the instruction executed, then the
// CPU state *after* it ran (same post-exec semantics as the app's trace ring).
#pragma pack(push, 1)
struct HRec {
    uint16_t pc, opcode;
    uint16_t KR, SR, fA, fB, flags, PREG, EXT, libAddr;
    uint8_t  R5, digit, weight, pad;
    uint8_t  A[16], B[16], C[16], D[16], E[16];
    uint8_t  SCOM[16][16];
};
#pragma pack(pop)

void fill(HRec& r, uint16_t pc, uint16_t op, const CpuFrame& f, int w) {
    r.pc = pc; r.opcode = op;
    r.KR = f.KR; r.SR = f.SR; r.fA = f.fA; r.fB = f.fB; r.flags = f.cpuFlags;
    r.PREG = f.PREG; r.EXT = f.EXT; r.libAddr = f.m_libAddr;
    r.R5 = f.R5; r.digit = f.digit; r.weight = static_cast<uint8_t>(w & 0xFF); r.pad = 0;
    memcpy(r.A, f.A, 16); memcpy(r.B, f.B, 16); memcpy(r.C, f.C, 16);
    memcpy(r.D, f.D, 16); memcpy(r.E, f.E, 16);
    memcpy(r.SCOM, f.SCOM, sizeof(r.SCOM));
}

// A handle: the model until h_load_model_roms has built the machine, and the
// ROM set of the folder it was given, looked up once (LIB-12; ADR 0035).
struct Handle {
    cu59::Model model;
    std::unique_ptr<Machine> machine;
    std::string romsDir;
    std::shared_ptr<const calcu::roms::RomSet> roms;
    std::string error;   // describe() of a failed lookup (LIB-12 AC-7)
};
Machine& M(void* h) { return *static_cast<Handle*>(h)->machine; }
TIDebugTargetAdapter& debug(void* h) { return *debugTargetOf(M(h)); }
TIMemoryAdapter& memory(void* h) { return *memoryOf(M(h)); }
TIPeripheralsAdapter& peripherals(void* h) { return *peripheralsOf(M(h)); }
TICardReaderAdapter& cards(void* h) { return *cardReaderOf(M(h)); }

const calcu::machine::MachineDescriptor& descriptorOf(int model) {
    return *tiCatalogue().model(cu59::modelId(static_cast<cu59::Model>(model)));
}

// A key name: a key ID of C59-1 AC-9 or one of its other spellings
// (docs/formats/preset.md §8.1), resolved here as the tools do (LIB-1 D-2).
std::optional<calcu::machine::KeyId> keyNamed(Machine& m, const char* name) {
    for (const auto& k : m.descriptor().keys) {
        if (k.id.value == name) return k.id;
        for (const auto& s : k.otherSpellings)
            if (s == name) return k.id;
    }
    return std::nullopt;
}

} // namespace

extern "C" {

int h_rec_size() { return static_cast<int>(sizeof(HRec)); }

// The model's instruction rate (tick rate / 32), so Python keeps no copies.
double h_instruction_hz(int model) { return descriptorOf(model).tickRate / 32.0; }
// Printer BUSY in instruction cycles: a printed line, or one PRT.FEED.
uint32_t h_printer_busy_cycles(int feed) {
    return feed ? TIPeripheralsAdapter::printerFeedCycles() : TIPeripheralsAdapter::printerLineCycles();
}
// The idle/key-detect range: out = {lo, hi, extraLo, extraHi} (extra empty when lo > hi).
void h_idle_range(int model, int* out) {
    const auto r = TIPacingHints::idleAndKeyDetectRange(cu59::modelId(static_cast<cu59::Model>(model)));
    for (int i = 0; i < 4; ++i) out[i] = r[static_cast<size_t>(i)];
}

// Model: 0 TI-59, 1 TI-58, 2 TI-58C (cu59::Model).
void* h_new(int model) { return new Handle{static_cast<cu59::Model>(model), nullptr, {}, nullptr, {}}; }
void  h_free(void* h) { delete static_cast<Handle*>(h); }

// Build the machine of `model` from the ROM set of `romsDir` (the lookup
// over DirectoryBundle(romsDir), LIB-12), through the machine interface like
// the apps (the state after a cold boot). 0 on success; -1 when a file of the
// full set is missing or bad: h_error() then names the files (AC-7).
int h_load_model_roms(void* hv, int model, const char* romsDir) {
    auto* h = static_cast<Handle*>(hv);
    if (!h->roms || h->romsDir != romsDir) {
        auto r = cu59::lookUpRoms(calcu::roms::DirectoryBundle(romsDir));
        h->romsDir = romsDir;
        h->roms = r.set;
        h->error = r.ok() ? std::string() : calcu::roms::describe(r.missing);
        if (!r.ok()) return -1;
    }
    h->model = static_cast<cu59::Model>(model);
    h->machine = cu59::buildMachine(h->model, *h->roms);
    return 0;
}

// Why the last h_load_model_roms failed ("" after a success).
const char* h_error(void* h) { return static_cast<Handle*>(h)->error.c_str(); }

// A solid-state module by its cuecards.txt id (romsDir/solid-state/cuecards.txt,
// the module list until M-25), its ROM from the set (`<ID>.rom.yaml`).
int h_load_module(void* hv, const char* moduleID, const char* romsDir) {
    auto* h = static_cast<Handle*>(hv);
    if (!h->roms || !h->machine) return -1;
    std::ifstream f(std::string(romsDir) + "/solid-state/cuecards.txt", std::ios::binary);
    if (!f) return -1;
    std::ostringstream catalog;
    catalog << f.rdbuf();
    auto file = cu59::moduleRomFile(catalog.str(), moduleID);
    if (!file) return -1;
    auto image = cu59::moduleImage(*h->roms, *file);
    if (!image) return -1;
    peripherals(h).loadModuleImage(image->data(), image->size());
    return 0;
}

// 2.3.0's core reset: a power cycle (C59-1 AC-6).
void h_reset(void* h) { M(h).powerCycle(); }

// Keys by key ID or other spelling; -1 for an unknown name (C59-1 EC-3).
int h_key_down(void* h, const char* name) {
    auto id = keyNamed(M(h), name);
    return id && M(h).keyDown(*id) == calcu::machine::KeyCall::Done ? 0 : -1;
}
int h_key_up(void* h, const char* name) {
    auto id = keyNamed(M(h), name);
    return id && M(h).keyUp(*id) == calcu::machine::KeyCall::Done ? 0 : -1;
}
void h_printer(void* h, int connected)  { peripherals(h).setPrinterConnected(connected != 0); }
void h_trace_sw(void* h, int on)        { peripherals(h).setPrinterTrace(on != 0); }
void h_printer_print(void* h, int down) { peripherals(h).pressPrinterPrint(down != 0); }
void h_printer_adv(void* h, int down)   { peripherals(h).pressPrinterAdv(down != 0); }

// n instructions: n × advance(1), so instruction counts stay exact.
uint32_t h_stepn(void* h, uint32_t n) {
    for (uint32_t i = 0; i < n; ++i) M(h).advance(1);
    return n;
}
uint16_t h_pc(void* h) { return debug(h).pc(); }
// Weighted instructions (idle steps count 4): the tick count / 32. It goes on
// across a reset (C59-1 AC-3).
uint64_t h_elapsed(void* h) { return M(h).tickCount() / 32; }

// Run n steps, recording each into out[0..n). Stops early (returning the
// count recorded) when stopPC >= 0 and the next PC equals it.
int h_record(void* h, HRec* out, int n, int stopPC) {
    for (int i = 0; i < n; ++i) {
        CpuFrame pre = debug(h).snapshotCPU();
        int w = static_cast<int>(debug(h).step() & 0xFF);
        CpuFrame post = debug(h).snapshotCPU();
        fill(out[i], pre.pc, pre.opcode, post, w);
        if (stopPC >= 0 && post.pc == stopPC) return i + 1;
    }
    return n;
}

void h_snapshot(void* h, HRec* out) {
    CpuFrame f = debug(h).snapshotCPU();
    fill(*out, f.pc, f.opcode, f, 0);
}

void h_ram_read(void* h, int reg, uint8_t* out16)        { memory(h).copyRAMReg(reg, out16); }
void h_ram_write(void* h, int reg, const uint8_t* in16)  { memory(h).writeRAMReg(reg, in16); }
void h_write_program(void* h, const uint8_t* k, int n)   { memory(h).writeProgram(k, n); }
uint16_t h_lib_exec_pc(void* h) { return cards(h).libExecPC(); }

// The display since the last call (Machine::readDisplay), index 0 rightmost:
// per position the lit segments A–G as bits 0–6, the decimal point lit (0/1)
// and the position's brightness 0–1 (0: not driven since the last call).
void h_display(void* h, uint8_t* segments12, uint8_t* dp12, float* brightness12) {
    const auto out = M(h).readDisplay();
    const auto& ps = std::get<calcu::machine::SegmentFrame>(out.frame).positions;
    for (size_t i = 0; i < 12; ++i) {
        uint8_t bits = 0;
        for (size_t s = 0; s < 7; ++s)
            if (ps[i].lit[s]) bits |= static_cast<uint8_t>(1u << s);
        segments12[i] = bits;
        dp12[i] = ps[i].lit[7] ? 1 : 0;
        brightness12[i] = ps[i].brightness.value_or(0.0f);
    }
}

// Magnetic card: insert (count 0 = blank card for writing), eject (returns the
// bytes written, up to cap), and the reader's state.
void h_card_insert(void* h, const uint8_t* b, int n) { cards(h).insertCard(b, static_cast<size_t>(n)); }
int  h_card_eject(void* h, uint8_t* out, int cap) {
    std::vector<uint8_t> v = cards(h).cardEject();
    int n = static_cast<int>(v.size()) < cap ? static_cast<int>(v.size()) : cap;
    if (n) memcpy(out, v.data(), static_cast<size_t>(n));
    return n;
}
int  h_card_present(void* h) { return cards(h).isCardPresent() ? 1 : 0; }
int  h_card_mode(void* h)    { return cards(h).cardMode(); }
int  h_card_waiting(void* h) { return cards(h).isWaitingForCard() ? 1 : 0; }

// Printer output lines, joined with '\n'. Returns bytes written (truncated to cap).
int h_printer_lines(void* h, char* out, int cap) {
    std::string s;
    for (auto& l : peripherals(h).drainPrinterLines()) { s += l; s += '\n'; }
    int n = static_cast<int>(s.size()) < cap ? static_cast<int>(s.size()) : cap;
    memcpy(out, s.data(), static_cast<size_t>(n));
    return n;
}

// Printer code lines (the dot-matrix queue, separate from the text lines):
// 23 bytes each: the 20 character codes, the complete rows (0-7), the
// columns of the next row started before a cut (0-5), and the cycles the
// leftmost of them was heated (capped at 255). Returns the number of lines
// written; the queue is drained, so lines beyond cap are lost.
int h_printer_code_lines(void* h, uint8_t* out, int cap) {
    int n = 0;
    for (auto& l : peripherals(h).drainPrinterCodeLines()) {
        if (n == cap) break;
        memcpy(out + 23 * n, l.codes.data(), 20);
        out[23 * n + 20] = l.rowCount;
        out[23 * n + 21] = l.partialColumns;
        out[23 * n + 22] = l.partialHeat;
        ++n;
    }
    return n;
}

int h_disasm(uint16_t pc, uint16_t op, char* out, int cap) {
    std::string s = TIDebugTargetAdapter::disassemble(pc, op);
    int n = static_cast<int>(s.size()) < cap - 1 ? static_cast<int>(s.size()) : cap - 1;
    memcpy(out, s.data(), static_cast<size_t>(n));
    out[n] = 0;
    return n;
}

} // extern "C"
