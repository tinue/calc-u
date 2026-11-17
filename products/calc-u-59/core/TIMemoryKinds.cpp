#include "TIMemoryKinds.hpp"
#include "TIMachine.hpp"

using cu59::RegisterNibbles;

namespace {
constexpr int kExtraBase = 60;   // H00–H03 are raw RAM 60–63 on the TI-58C (rom/TI58C-ROM-Guide.md:37)
}

int TIProgramMemory::lastStep() const { return hasLargeMemory(m_.m_variant) ? 959 : 479; }

std::uint8_t TIProgramMemory::readStep(int step) const {
    if (step < 0 || step > lastStep()) return 0;
    return m_.m_hw.readProgramStep(step);
}

void TIProgramMemory::writeStep(int step, std::uint8_t keycode) {
    if (step < 0 || step > lastStep()) return;
    m_.m_hw.writeProgramStep(step, keycode);
}

int TIProgramMemory::dataRegisterCount() const { return hasLargeMemory(m_.m_variant) ? 100 : 60; }

RegisterNibbles TIProgramMemory::readRegister(int n) const {
    RegisterNibbles out{};
    if (n < 0 || n >= dataRegisterCount()) return out;
    const int idx = m_.m_hw.dataRegCount() - 1 - n;
    if (idx >= 0 && idx < m_.m_hw.ramRegCount()) m_.m_hw.copyRAMReg(idx, out.data());
    return out;
}

void TIProgramMemory::writeRegister(int n, const RegisterNibbles& v) {
    if (n < 0 || n >= dataRegisterCount()) return;
    m_.m_hw.writeDataRegister(n, v.data());
}

int TIProgramMemory::extraCount() const { return hasConstantMemory(m_.m_variant) ? 4 : 0; }

RegisterNibbles TIProgramMemory::readExtra(int n) const {
    RegisterNibbles out{};
    if (n >= 0 && n < extraCount()) m_.m_hw.copyRAMReg(kExtraBase + n, out.data());
    return out;
}

void TIProgramMemory::writeExtra(int n, const RegisterNibbles& v) {
    if (n >= 0 && n < extraCount()) m_.m_hw.writeRAMReg(kExtraBase + n, v.data());
}

int TIProgramMemory::partitionSteps() const { return m_.m_hw.partitionProgramRegs() * 8; }

void TIProgramMemory::setPartition(int steps) { m_.m_hw.setPartitionProgramRegs(steps / 8); }

RegisterNibbles TIProgramMemory::readRaw(int r) const {
    RegisterNibbles out{};
    m_.m_hw.copyRAMReg(r, out.data());
    return out;
}

int TIProgramMemory::rawCount() const { return m_.m_hw.ramRegCount(); }

std::array<RegisterNibbles, cu59::ConstantMemory::kRegisters> TIConstantMemoryCapability::content() const {
    std::array<RegisterNibbles, kRegisters> out{};
    for (int r = 0; r < kRegisters; ++r) m_.m_hw.copyRAMReg(r, out[static_cast<std::size_t>(r)].data());
    return out;
}

std::uint64_t TIConstantMemoryCapability::changeCount() const {
    auto now = content();
    if (seen_ != now) {
        seen_ = now;
        ++count_;
    }
    return count_;
}

std::optional<int> TILibraryProgram::selected() const {
    const int n = m_.m_hw.insertedModuleNumber();
    return n > 0 ? std::optional<int>(n) : std::nullopt;
}
