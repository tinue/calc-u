#pragma once
#include "TIProductKinds.hpp"
#include <cstdint>
#include <optional>

// The product kinds of TIProductKinds.hpp on a TIMachine. Members of TIMachine; it lists them in capabilities().

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
    int registerTop() const override;

private:
    TIMachine& m_;
};

// The counter changes when R000–R063 differ from what
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

// SCOM register 9's digits 4 and 3; none for 0.
class TILibraryProgram final : public cu59::LibraryProgram {
public:
    explicit TILibraryProgram(TIMachine& m) : m_(m) {}
    std::optional<int> selected() const override;

private:
    TIMachine& m_;
};
