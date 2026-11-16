#pragma once
#include "TIProductKinds.hpp"
#include <cstdint>
#include <optional>

// The product kinds of TIProductKinds.hpp on a TIMachine (C59-2 AC-17,
// C59-3 AC-1). Members of TIMachine; it lists them in capabilities().

class TIMachine;

class TIProgramMemory final : public cu59::ProgramMemory {
public:
    explicit TIProgramMemory(TIMachine& m) : m_(m) {}
    int lastStep() const override;
    std::uint8_t readStep(int step) const override;
    void writeStep(int step, std::uint8_t keycode) override;
    int dataRegisterCount() const override;
    cu59::RegisterNibbles readRegister(int n) const override;
    void writeRegister(int n, const cu59::RegisterNibbles&) override;
    int extraCount() const override;
    cu59::RegisterNibbles readExtra(int n) const override;
    void writeExtra(int n, const cu59::RegisterNibbles&) override;
    int partitionSteps() const override;
    void setPartition(int steps) override;
    cu59::RegisterNibbles readRaw(int r) const override;
    int rawCount() const override;

private:
    TIMachine& m_;
};

// D-1 (C59-3 design): the counter changes when R000–R063 differ from what
// was seen at the last read. It is computed when asked (between two
// advances), not on the RAM write path, so the advances carry no cost.
class TIConstantMemoryCapability final : public cu59::ConstantMemory {
public:
    explicit TIConstantMemoryCapability(TIMachine& m) : m_(m) {}
    std::array<cu59::RegisterNibbles, kRegisters> content() const override;
    std::uint64_t changeCount() const override;

private:
    TIMachine& m_;
    mutable std::optional<std::array<cu59::RegisterNibbles, kRegisters>> seen_;
    mutable std::uint64_t count_ = 0;
};
