#include "TIDebugKindsImpl.hpp"
#include "TIMachine.hpp"

// ── c59.asm-overlay (C59-4 AC-13, AC-17, AC-18, AC-26) ──────────────────────

bool TIAsmOverlay::load(const std::vector<std::uint16_t>& words) {
    if (words.size() > kMaxInstructions) return false;
    m_.m_hw.clearDebugOverlay();
    if (!words.empty() && !m_.m_hw.loadDebugOverlay(words.data(), words.size())) return false;
    m_loaded = words.size();
    return true;
}

void TIAsmOverlay::clear() {
    m_.m_hw.clearDebugOverlay();
    m_loaded = 0;
    m_running = false;
}

TIAsmOverlay::Entry TIAsmOverlay::enter(std::uint32_t maxInstructions) {
    Entry e;
    if (m_loaded == 0) return e;
    // Park the processor at a program step boundary (the digit counter
    // mid-cycle), then on at 1800 with COND set (TMC0501::runDebugInjectedProgram).
    std::uint32_t done = 0;
    while (done < 50000) {
        const std::uint32_t r = m_.stepOne();
        done += r & TI59Machine::kWeightMask;   // weight: 1 active, 4 IDLE
        if (r & TI59Machine::kStepBoundary) break;
    }
    e.entered = m_.m_hw.runDebugOverlay(kEntry, maxInstructions, &e.instructions);
    m_.m_hw.beginNextStep();
    if (e.entered) m_running = true;
    return e;
}

// ── c59.calculator-state (AC-3, AC-4, AC-6; D-3) ────────────────────────────

std::uint16_t TICalculatorState::pc() const { return m_.m_hw.pc(); }
CpuFrame TICalculatorState::snapshotCPU() const { return m_.m_hw.snapshotCPU(); }
TIStepBoundary TICalculatorState::lastStepBoundary() const { return m_.m_hw.lastStepBoundary(); }
std::uint8_t TICalculatorState::readROMKeycode(int addr) const { return m_.m_hw.readROMKeycode(addr); }
std::uint32_t TICalculatorState::readCpuFrames(CpuFrame* out, std::uint32_t max) const {
    return m_.m_hw.readCpuFrames(out, max);
}
void TICalculatorState::setFrameRing(bool on) {
    m_.m_panelTrace = on;
    m_.applyTraceFlags();
}

// ── c59.machine-log (AC-11) ──────────────────────────────────────────────────

void TIMachineLog::setLevel(int level) {
    m_level = level < 0 ? 0 : level > 2 ? 2 : level;
    m_.m_hw.setDebugLevel(static_cast<std::uint8_t>(m_level));
}

std::vector<cu59::MachineLog::Line> TIMachineLog::take() {
    std::vector<Line> out;
    for (const auto& e : m_.m_hw.drainDebugEvents())
        if (m_level > 0 && e.level <= m_level) out.push_back({e.level, e.msg});
    return out;
}

double cu59::decodeRegister(const std::uint8_t* n) { return TI59Machine::decodeBCD(n); }
