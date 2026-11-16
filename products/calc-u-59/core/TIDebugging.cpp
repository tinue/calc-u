#include "TIDebugging.hpp"
#include "TIMachine.hpp"
#include <algorithm>
#include <utility>

using namespace calcu::machine;

namespace {

// The registers of the processor frame (CpuFrame, TraceTypes.hpp), in this order.
enum Reg : std::size_t { A, B, C, D, E, KR, SR, FA, FB, FLAGS, R5, DIGIT, EXT, PREG, PC, kRegs };

std::uint64_t digits(const std::uint8_t (&n)[16]) {   // index 0 = least significant digit
    std::uint64_t v = 0;
    for (int i = 15; i >= 0; --i) v = (v << 4) | (n[i] & 0x0Fu);
    return v;
}

std::vector<std::uint64_t> registersOf(const CpuFrame& f) {
    std::vector<std::uint64_t> v(kRegs);
    v[A] = digits(f.A);
    v[B] = digits(f.B);
    v[C] = digits(f.C);
    v[D] = digits(f.D);
    v[E] = digits(f.E);
    v[KR] = f.KR;
    v[SR] = f.SR;
    v[FA] = f.fA;
    v[FB] = f.fB;
    v[FLAGS] = f.flags;
    v[R5] = f.R5;
    v[DIGIT] = f.digit;
    v[EXT] = f.EXT;
    v[PREG] = f.PREG;
    v[PC] = f.pc;
    return v;
}

TargetInfo tiInfo(int ramRegs) {
    TargetInfo t;
    t.name = DebugTargetId{"TMC0501"};
    for (const char* n : {"A", "B", "C", "D", "E"}) t.registers.push_back({n, 64, false});
    for (const char* n : {"KR", "SR", "fA", "fB", "flags"}) t.registers.push_back({n, 16, false});
    for (const char* n : {"R5", "digit"}) t.registers.push_back({n, 8, false});
    for (const char* n : {"EXT", "PREG", "pc"}) t.registers.push_back({n, 16, false});
    t.spaces.push_back(SpaceInfo{"rom", 13, 0x2000, false, {}});
    t.spaces.push_back(SpaceInfo{"ram", 64, static_cast<std::uint64_t>(ramRegs), false, {}});
    t.spaces.push_back(SpaceInfo{"scom", 64, 16, false, {}});
    t.spaces.push_back(SpaceInfo{"program", 8, static_cast<std::uint64_t>(ramRegs) * 8, false, {}});
    t.codeSpace = TIDebugTarget::Rom;
    t.offers.watches = false;
    t.offers.stepOverOut = false;
    t.offers.decode = true;
    t.offers.history = 1024;   // TMC0501::kFrameRingSize
    return t;
}

} // namespace

// ── The target ───────────────────────────────────────────────────────────────

TIDebugTarget::TIDebugTarget(TIMachine& m) : m_(m), m_info(tiInfo(m.m_hw.ramRegCount())) {}

std::uint64_t TIDebugTarget::nextAddress() const { return m_.m_hw.pc(); }

std::uint64_t TIDebugTarget::readRegister(std::size_t i) const {
    if (i >= kRegs) return 0;
    return registersOf(m_.m_hw.snapshotCPU())[i];
}

CellRead TIDebugTarget::peek(std::size_t space, std::uint64_t a, BankSelect) const {
    CellRead c;
    switch (space) {
    case Rom:
        if (a >= 0x2000) c.kind = CellRead::Unreadable;
        else c.value = m_.m_hw.readROMWord(static_cast<std::uint16_t>(a)) & 0x1FFFu;
        return c;
    case Ram: {
        if (a >= static_cast<std::uint64_t>(m_.m_hw.ramRegCount())) {
            c.kind = CellRead::Unreadable;
            return c;
        }
        std::uint8_t n[16];
        m_.m_hw.copyRAMReg(static_cast<int>(a), n);
        c.value = digits(n);
        return c;
    }
    case Scom: {
        if (a >= 16) {
            c.kind = CellRead::Unreadable;
            return c;
        }
        const CpuFrame f = m_.m_hw.snapshotCPU();
        c.value = digits(f.SCOM[a]);
        return c;
    }
    case Program:
        if (a >= static_cast<std::uint64_t>(m_.m_hw.ramRegCount()) * 8) c.kind = CellRead::Unreadable;
        else c.value = m_.m_hw.readProgramStep(static_cast<int>(a));
        return c;
    default: c.kind = CellRead::Unreadable; return c;
    }
}

bool TIDebugTarget::poke(std::size_t space, std::uint64_t a, std::uint64_t value, BankSelect) {
    switch (space) {
    case Ram: {
        if (a >= static_cast<std::uint64_t>(m_.m_hw.ramRegCount())) return false;
        std::uint8_t n[16];
        for (int i = 0; i < 16; ++i) n[i] = static_cast<std::uint8_t>((value >> (4 * i)) & 0x0Fu);
        m_.m_hw.writeRAMReg(static_cast<int>(a), n);
        return true;
    }
    case Program:
        if (a >= static_cast<std::uint64_t>(m_.m_hw.ramRegCount()) * 8 || value > 99) return false;
        m_.m_hw.writeProgramStep(static_cast<int>(a), static_cast<std::uint8_t>(value));
        return true;
    default: return false;   // rom and scom refuse a write (D-21)
    }
}

std::vector<HistoryEntry> TIDebugTarget::history(std::size_t count) const {
    std::vector<CpuFrame> frames(std::min<std::size_t>(count, 1024));
    frames.resize(m_.m_hw.readCpuFrames(frames.data(), static_cast<std::uint32_t>(frames.size())));
    std::vector<HistoryEntry> out;
    for (auto it = frames.rbegin(); it != frames.rend(); ++it)   // newest first
        out.push_back(HistoryEntry{it->pc, registersOf(*it), TI59Machine::disassemble(it->pc, it->opcode)});
    return out;
}

std::optional<Decoded> TIDebugTarget::decode(std::uint64_t address) const {
    if (address >= 0x2000) return std::nullopt;
    const auto pc = static_cast<std::uint16_t>(address);
    return Decoded{TI59Machine::disassemble(pc, m_.m_hw.readROMWord(pc) & 0x1FFFu), 1};
}

std::uint64_t TIDebugTarget::retired() const { return m_.m_retired; }

void TIDebugTarget::setBreakpoints(std::vector<std::uint64_t> addresses) {
    m_breakpoints.clear();
    for (auto a : addresses)
        if (a < 0x2000) m_breakpoints.push_back(static_cast<std::uint16_t>(a));
    std::sort(m_breakpoints.begin(), m_breakpoints.end());
    m_breakpoints.erase(std::unique(m_breakpoints.begin(), m_breakpoints.end()), m_breakpoints.end());
}

// ── Debugging ────────────────────────────────────────────────────────────────

void TIDebugging::setChecks(const Checks& c) {
    m_checks = c;
    if (m_checks.breakpoints && m_target.breakpoints().empty()) m_checks.breakpoints = false;
    m_.m_checksTrace = active();   // the history of what ran while checked (AC-28)
    m_.applyTraceFlags();
}

std::optional<DebugStop> TIDebugging::takeStop() { return std::exchange(m_stop, std::nullopt); }

std::string TIDebugging::disassemble(std::uint16_t pc, std::uint16_t opcode) { return TI59Machine::disassemble(pc, opcode); }

// ── c59.trace-records ────────────────────────────────────────────────────────

void TITraceRecords::begin() {
    m_.m_recordTrace = true;
    m_.applyTraceFlags();
    // The trace starts here: what the ring holds from before is not its.
    std::vector<CpuFrame> skip(1024);
    std::uint32_t lost = 0;
    while (m_.m_hw.drainCpuFrames(skip.data(), static_cast<std::uint32_t>(skip.size()), &lost) > 0) {}
}

std::uint32_t TITraceRecords::take(std::vector<CpuFrame>& out, std::uint32_t max, std::uint32_t& lost) {
    const std::size_t at = out.size();
    out.resize(at + max);
    const std::uint32_t n = m_.m_hw.drainCpuFrames(out.data() + at, max, &lost);
    out.resize(at + n);
    return n;
}

void TITraceRecords::end() {
    m_.m_recordTrace = false;
    m_.applyTraceFlags();
}
