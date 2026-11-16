#pragma once
#include "TIDebugKinds.hpp"
#include <algorithm>
#include <vector>

// The product kinds of TIDebugKinds.hpp on a TIMachine (C59-4). Members of
// TIMachine; it lists them in capabilities().

class TIMachine;

class TIAsmOverlay final : public cu59::AsmOverlay {
public:
    explicit TIAsmOverlay(TIMachine& m) : m_(m) {}
    bool load(const std::vector<std::uint16_t>& words) override;
    void clear() override;
    std::size_t loaded() const override { return m_loaded; }
    Entry enter(std::uint32_t maxInstructions) override;
    bool running() const override { return m_running; }

private:
    friend class TIMachine;
    TIMachine& m_;
    std::size_t m_loaded = 0;
    bool m_running = false;
};

class TIRomHeatmap final : public cu59::RomHeatmapCounter {
public:
    explicit TIRomHeatmap(TIMachine&) {}
    void setOn(bool on) override { m_on = on; }
    bool on() const override { return m_on; }
    const std::vector<std::uint32_t>& counts() const override { return m_counts; }
    void reset() override { std::fill(m_counts.begin(), m_counts.end(), 0u); }
    /// TIMachine::stepOne, before the instruction at `pc`.
    void count(std::uint16_t pc) {
        if (m_on && pc < kAddresses) ++m_counts[pc];
    }

private:
    bool m_on = false;
    std::vector<std::uint32_t> m_counts = std::vector<std::uint32_t>(kAddresses, 0u);
};

class TICalculatorState final : public cu59::CalculatorState {
public:
    explicit TICalculatorState(TIMachine& m) : m_(m) {}
    std::uint16_t pc() const override;
    CpuFrame snapshotCPU() const override;
    TIStepBoundary lastStepBoundary() const override;
    std::uint8_t readROMKeycode(int addr) const override;
    std::uint32_t readCpuFrames(CpuFrame* out, std::uint32_t max) const override;
    void setFrameRing(bool on) override;

private:
    TIMachine& m_;
};

class TIMachineLog final : public cu59::MachineLog {
public:
    explicit TIMachineLog(TIMachine& m) : m_(m) {}
    void setLevel(int level) override;
    int level() const override { return m_level; }
    std::vector<Line> take() override;

private:
    TIMachine& m_;
    int m_level = 0;
};
