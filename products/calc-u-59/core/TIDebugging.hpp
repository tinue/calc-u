#pragma once
#include "TraceRecords.hpp"
#include "machine/DebugTarget.hpp"
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

// The TI machines' debug target (LIB-10 design §12.4; spec AC-28, D-21,
// D-22): one target, TMC0501, on the TI-59, TI-58 and TI-58C. Registers of
// today's processor frame (read only); the spaces rom (13-bit words), ram
// (16-digit registers), scom (16 digits × 16) and program (8-bit program
// steps); ram and program take writes, rom and scom refuse them; a history
// (the core's frame ring, newest first) and decoding; no watches, no step
// over or out, no qualifier.
//
// TIMachine::advance asks the library's observer after every instruction
// while Checks say so, with InstructionEvent::marks (kMark…): the next
// instruction is a program step boundary, it lies in the keyboard scan loop,
// and the instruction's weight (1 active, 4 IDLE). A breakpoint stops the
// advance before the instruction at its address. When the observer stops an
// advance, the next instruction's pre-execution phase is run at once
// (TMC0501::beginNextStep, idempotent), so that what is read while paused is
// the state of the next step (AC-3).

class TIMachine;

namespace timarks {

inline constexpr std::uint32_t kMarkBoundary = 1u << 0;   ///< the next instruction is a program step boundary
inline constexpr std::uint32_t kMarkScanLoop = 1u << 1;   ///< the next instruction is in the keyboard scan loop
inline constexpr int kMarkWeightShift = 8;                ///< bits 8–15: the instruction's weight
constexpr std::uint32_t markWeight(std::uint32_t marks) { return (marks >> kMarkWeightShift) & 0xFFu; }

} // namespace timarks

class TIDebugTarget final : public calcu::machine::DebugTarget {
public:
    enum Space : std::size_t { Rom = 0, Ram = 1, Scom = 2, Program = 3 };
    explicit TIDebugTarget(TIMachine& m);

    const calcu::machine::TargetInfo& info() const override { return m_info; }
    std::uint64_t nextAddress() const override;
    std::uint64_t readRegister(std::size_t) const override;
    bool writeRegister(std::size_t, std::uint64_t) override { return false; }   // D-21
    calcu::machine::CellRead peek(std::size_t space, std::uint64_t address, calcu::machine::BankSelect) const override;
    bool poke(std::size_t space, std::uint64_t address, std::uint64_t value, calcu::machine::BankSelect) override;
    std::vector<calcu::machine::HistoryEntry> history(std::size_t count) const override;
    std::optional<calcu::machine::Decoded> decode(std::uint64_t address) const override;
    std::uint64_t retired() const override;
    void setBreakpoints(std::vector<std::uint64_t> addresses) override;

    const std::vector<std::uint16_t>& breakpoints() const { return m_breakpoints; }

private:
    TIMachine& m_;
    calcu::machine::TargetInfo m_info;
    std::vector<std::uint16_t> m_breakpoints;   // sorted
};

class TIDebugging final : public calcu::machine::Debugging {
public:
    explicit TIDebugging(TIMachine& m) : m_(m), m_target(m) {}

    std::size_t targetCount() const override { return 1; }
    calcu::machine::DebugTarget& target(std::size_t) override { return m_target; }
    const calcu::machine::DebugTarget& target(std::size_t) const override { return m_target; }
    std::size_t current() const override { return 0; }
    void setChecks(const calcu::machine::Checks&) override;
    std::optional<calcu::machine::DebugStop> takeStop() override;
    void passOnce(std::size_t) override { m_passOnce = true; }

    /// The disassembly of one 13-bit word (the headless tool, decode()).
    static std::string disassemble(std::uint16_t pc, std::uint16_t opcode);
    /// While `runOn` says so and the observer is set, an advance goes on past
    /// the ticks asked for until the observer stops it (a pause or a step of
    /// the run control waiting to land runs in one host call, as 2.3.0's
    /// freeze and STEP did: at most 50000 weights, TILevelRules); unset: never.
    void setRunOn(std::function<bool()> runOn) { m_runOn = std::move(runOn); }

private:
    friend class TIMachine;
    bool active() const { return m_checks.observer || m_checks.breakpoints; }

    TIMachine& m_;
    TIDebugTarget m_target;
    calcu::machine::Checks m_checks;
    std::optional<calcu::machine::DebugStop> m_stop;
    bool m_passOnce = false;
    std::function<bool()> m_runOn;
};

/// The c59.trace-records capability of a TI machine (TraceRecords.hpp).
class TITraceRecords final : public TraceRecords {
public:
    explicit TITraceRecords(TIMachine& m) : m_(m) {}
    void begin() override;
    std::uint32_t take(std::vector<CpuFrame>& out, std::uint32_t max, std::uint32_t& lost) override;
    void end() override;

private:
    TIMachine& m_;
};
