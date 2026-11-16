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
#include "../../core/TIMagneticCard.hpp"
#include "media/MediaPlace.hpp"
#include "../../core/PC100C.hpp"
#include "../../core/TIMachineTesting.hpp"   // LIB-8 P-E3: the PC-100C's code lines (until M-59)
#include "paper/Paper.hpp"
#include "../../core/TICatalogue.hpp"
#include "peripherals/Controls.hpp"
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
    // The PC-100C's paper (LIB-8 59, M-53): its text layer, and the lines taken.
    std::unique_ptr<calcu::paper::Paper> paper;
    size_t linesTaken = 0;
};
Machine& M(void* h) { return *static_cast<Handle*>(h)->machine; }
TIDebugPanelAdapter& debug(void* h) { return *debugPanelOf(M(h)); }
TIMemoryAdapter& memory(void* h) { return *memoryOf(M(h)); }
// The PC-100C and the module in their places (C59-6, design §7): the
// capabilities while a PC-100C is plugged in. Its paper output is bound to
// the handle's Paper whenever a new one is there (LIB-8 AC-24).
calcu::paper::Paper& paper(void* h) {
    Handle& hd = *static_cast<Handle*>(h);
    if (!hd.paper) hd.paper = std::make_unique<calcu::paper::Paper>(pc100cpaper::description());
    auto* out = calcu::machine::capabilityAs<calcu::paper::PaperOutput>(M(h), calcu::machine::CapabilityId{"printer/paper-output"});
    if (!out) hd.paper->release();
    else if (out->connected() != hd.paper.get()) {
        hd.paper->release();
        hd.paper->bind(out);
    }
    return *hd.paper;
}
calcu::peripherals::Controls* printerControls(void* h) {
    return calcu::machine::capabilityAs<calcu::peripherals::Controls>(M(h), calcu::machine::CapabilityId{"printer/controls"});
}
void control(void* h, const calcu::machine::ControlId& id, bool on, bool isSwitch) {
    if (auto* c = printerControls(h)) {
        if (isSwitch) c->set(id, on);
        else if (on) c->press(id);
        else c->release(id);
    }
}
// The card reader's media place (C59-7, M-55); nullptr on a TI-58 or TI-58C.
calcu::media::MediaPlace* cards(void* h) {
    return calcu::machine::capabilityAs<calcu::media::MediaPlace>(M(h), calcu::machine::CapabilityId{"card-reader/media-place"});
}

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
    return feed ? PC100C::kFeedInstructions : PC100C::kLineInstructions;
}
// The idle/key-detect range: out = {lo, hi, extraLo, extraHi} (extra empty when lo > hi).
void h_idle_range(int model, int* out) {
    const auto r = idleAndKeyDetectRange(cu59::modelId(static_cast<cu59::Model>(model)));
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
    // As 2.3.0's core started: the PC-100C plugged in, the slot empty; the
    // caller plugs a module with h_load_module, the printer with h_printer.
    h->paper.reset();   // before the old machine goes: its capability with it
    h->linesTaken = 0;
    h->machine = cu59::buildMachine(h->model, *h->roms);
    TIMachineTesting::printedCodeLines(static_cast<TIMachine&>(*h->machine));   // the record on (P-E3)
    paper(h);   // bound from the start, as 2.3.0 kept every line
    return 0;
}

// Why the last h_load_model_roms failed ("" after a success).
const char* h_error(void* h) { return static_cast<Handle*>(h)->error.c_str(); }

// A Solid State ROM module by its ID (C59-6 AC-8) into `slot-1`, its ROM
// from the set (`<ID>.rom.yaml`); the module there before is replaced. The
// power cycle that follows is the caller's (ti59.py runs h_reset). `romsDir`
// is no longer read. -1: no such module, or the attach was refused.
int h_load_module(void* hv, const char* moduleID, const char* /*romsDir*/) {
    auto* h = static_cast<Handle*>(hv);
    if (!h->roms || !h->machine || !isTIModule(moduleID)) return -1;
    auto req = cu59::moduleRequest(moduleID, *h->roms);
    std::optional<calcu::machine::PeripheralId> old;
    for (const auto& e : M(h).plugged().byPlace)
        if (e.place == req.place) old = e.peripheral;
    auto r = old ? M(h).replace(*old, std::move(req), tiCatalogue()) : M(h).attach(std::move(req), tiCatalogue());
    return r.refused ? -1 : 0;
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
// The PC-100C attached or detached (no power cycle here: the caller's); its
// TRACE switch and PRINT/ADV keys through its controls (C59-6 AC-5).
void h_printer(void* h, int connected) {
    paper(h);   // what was printed before a detach stays on the handle's paper
    const bool there = printerControls(h) != nullptr;
    if (connected && !there) M(h).attach(cu59::printerRequest(false), tiCatalogue());
    if (!connected && there) M(h).detach(calcu::machine::PlacePath{{kTIPrinterPlace}}, kPC100C, tiCatalogue());
}
void h_trace_sw(void* h, int on)        { control(h, PC100C::kTrace, on != 0, true); }
void h_printer_print(void* h, int down) { control(h, PC100C::kPrint, down != 0, false); }
void h_printer_adv(void* h, int down)   { control(h, PC100C::kAdv, down != 0, false); }

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
        int w = static_cast<int>(M(h).advance(1) / 32);   // one instruction: its weight × 32 ticks
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
uint16_t h_lib_exec_pc(void* h) { return debug(h).libExecPC(); }

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
void h_card_insert(void* h, const uint8_t* b, int n) {
    auto* place = cards(h);
    if (!place) return;
    auto card = std::make_shared<cu59::TIMagneticCardData>();
    if (b && n > 0) card->sides[0] = std::vector<uint8_t>(b, b + n);
    place->insert(card, 0);
}
int  h_card_eject(void* h, uint8_t* out, int cap) {
    auto* place = cards(h);
    if (!place) return 0;
    place->eject();
    const auto back = place->takeGivenBack();
    if (!back || !back->written) return 0;
    const auto* card = dynamic_cast<const cu59::TIMagneticCardData*>(back->content.get());
    if (!card || !card->lastWritten) return 0;
    const auto* v = std::get_if<std::vector<uint8_t>>(&card->sides[static_cast<size_t>(*card->lastWritten)]);
    if (!v) return 0;
    int k = static_cast<int>(v->size()) < cap ? static_cast<int>(v->size()) : cap;
    if (k) memcpy(out, v->data(), static_cast<size_t>(k));
    return k;
}
int  h_card_present(void* h) { return cards(h) && cards(h)->side() ? 1 : 0; }
int  h_card_mode(void* h)    { return TIMachineTesting::hardware(static_cast<TIMachine&>(M(h))).cardMode(); }
int  h_card_waiting(void* h) { return TIMachineTesting::hardware(static_cast<TIMachine&>(M(h))).isWaitingForCard() ? 1 : 0; }

// Printer output lines, joined with '\n'. Returns bytes written (truncated to cap).
int h_printer_lines(void* h, char* out, int cap) {
    std::string s;
    calcu::paper::Paper& p = paper(h);
    Handle& hd = *static_cast<Handle*>(h);
    for (size_t i = hd.linesTaken; i < p.text().size(); ++i) { s += p.text()[i].text; s += '\n'; }
    hd.linesTaken = p.text().size();
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
    std::vector<PC100C::CodeLine> lines = TIMachineTesting::printedCodeLines(static_cast<TIMachine&>(M(h)));
    for (auto& l : lines) {
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
    std::string s = TIDebugging::disassemble(pc, op);
    int n = static_cast<int>(s.size()) < cap - 1 ? static_cast<int>(s.size()) : cap - 1;
    memcpy(out, s.data(), static_cast<size_t>(n));
    out[n] = 0;
    return n;
}

} // extern "C"
