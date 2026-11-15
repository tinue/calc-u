#include "FreezeController.hpp"

namespace cu59 {

void FreezeController::armScanLoopExit(uint16_t currentPC) {
    m_pcArmed = false;
    m_seenLoop = false;
    m_armPC = currentPC;
    m_scanArmed = true;
}

void FreezeController::armPcChange() {
    disarmScanLoopExit();
    m_pcArmed = true;
}

void FreezeController::disarmScanLoopExit() {
    m_seenLoop = false;
    m_scanArmed = false;
}

void FreezeController::disarmPcChange() { m_pcArmed = false; }

std::optional<FreezeOwner> FreezeController::checkStep(uint16_t pc, bool stepBoundary, bool inScanLoop) {
    if (stepBoundary && m_pcArmed) {
        m_pcArmed = false;
        return FreezeOwner::Calculator;
    }
    if (!m_scanArmed) return std::nullopt;
    if (inScanLoop) {
        m_seenLoop = true;
        return std::nullopt;
    }
    if (m_seenLoop || (pc == 0x0000 && m_armPC != 0x0000)) {
        disarmScanLoopExit();
        return FreezeOwner::Cpu;
    }
    return std::nullopt;
}

bool FreezeController::stopAfter(uint16_t pc, bool stepBoundary, bool inScanLoop) {
    m_fired = checkStep(pc, stepBoundary, inScanLoop);
    return m_fired.has_value();
}

} // namespace cu59
