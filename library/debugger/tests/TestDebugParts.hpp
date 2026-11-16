#pragma once
#include "TestMachines.hpp"
#include "debugger/DebuggerPart.hpp"
#include "debugger/Levels.hpp"
#include "debugger/RunState.hpp"
#include "debugger/Symbols.hpp"
#include "debugger/TraceSession.hpp"
#include "formats/Number.hpp"
#include "machine/Build.hpp"
#include <chrono>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

// The debugger's parts built for the tests (LIB-10 design §6):
//   TestLevelRules   the test product's levels: `program` lands at the
//                    next program step boundary (the mark of a `nop`), one
//                    `program` step runs to the next boundary; `cpu` lands
//                    at once; the cpu repeats, the restart rule and the
//                    pause-on-start of `program` (the next boundary) are
//                    settable.
//   TestRecorder     a trace format of the tests: the header "TTRC", 0x01
//                    for the start of a session, 3 bytes per instruction
//                    (0x02, target, address), 0x03 for its end; the button's
//                    file name TEST_TRACE.bin.
//   TestSymbols      a table of the tests: a file `*.sym` of lines
//                    `name = <number>`.
// The rig of the session tests is DebugRig.hpp.

namespace calcu::debugger::test {

namespace mt = calcu::machine::test;
using machine::Ticks;

class TestLevelRules final : public LevelRules {
public:
    bool hasProgramLevel() const override { return program; }
    bool pauseLandsAtOnce(const LevelName& l) const override { return l == kCpu; }
    bool pauseLandsAfter(const LevelName&, const machine::InstructionEvent& e) override {
        return (e.marks & mt::cpu::kProgramStep) != 0;
    }
    void pauseAsked(const LevelName&, machine::Machine&) override { ++pausesAsked; }
    bool programStepDoneAfter(const machine::InstructionEvent& e) override { return (e.marks & mt::cpu::kProgramStep) != 0; }
    int cpuStepRepeats() const override { return repeats; }
    bool startFiresAfter(const LevelName& l, const machine::InstructionEvent& e, const machine::Machine& m) override {
        if (l == kProgram) return (e.marks & mt::cpu::kProgramStep) != 0;
        return LevelRules::startFiresAfter(l, e, m);
    }
    bool startFiresAtRestart(const LevelName& l, RestartKind k) const override {
        return l == kCpu && k == RestartKind::ColdBoot && coldBootFires;
    }
    bool program = true;
    int repeats = 0;
    bool coldBootFires = false;
    int pausesAsked = 0;
};

class TestRecorder final : public TraceRecorder {
public:
    explicit TestRecorder(mt::TestMachine& m) : m_(&m) {}
    void begin(std::vector<std::uint8_t>& out) override {
        out.insert(out.end(), {'T', 'T', 'R', 'C', 0x01});
        if (m_ && m_->cpu()) {
            m_->cpu()->recording = true;
            m_->cpu()->recorded.clear();
        }
    }
    bool take(std::vector<std::uint8_t>& out, std::uint64_t budget) override {
        if (!m_ || !m_->cpu()) return true;
        auto& rec = m_->cpu()->recorded;
        std::size_t n = 0;
        for (; n < rec.size() && budget >= 3; ++n, budget -= 3) out.insert(out.end(), {0x02, rec[n].first, rec[n].second});
        const bool all = n == rec.size();
        rec.erase(rec.begin(), rec.begin() + static_cast<std::ptrdiff_t>(n));
        return all;
    }
    std::uint64_t endBytes() const override { return 1; }
    void end(std::vector<std::uint8_t>& out) override {
        out.push_back(0x03);
        if (m_ && m_->cpu()) m_->cpu()->recording = false;
    }
    std::string buttonFileName() const override { return "TEST_TRACE.bin"; }
    void machineGone() override { m_ = nullptr; }

private:
    mt::TestMachine* m_;
};

class TestSymbols final : public SymbolReader {
public:
    bool reads(std::string_view name, const std::vector<std::uint8_t>&) const override {
        return name.size() > 4 && name.substr(name.size() - 4) == ".sym";
    }
    std::optional<SymbolTable> read(const std::vector<std::uint8_t>& bytes, std::string* error) const override {
        SymbolTable t;
        std::istringstream in(std::string(bytes.begin(), bytes.end()));
        for (std::string line; std::getline(in, line);) {
            if (line.empty()) continue;
            const auto eq = line.find('=');
            if (eq == std::string::npos) {
                *error = "A symbol line is name = number.";
                return std::nullopt;
            }
            auto trim = [](std::string s) {
                while (!s.empty() && s.front() == ' ') s.erase(s.begin());
                while (!s.empty() && s.back() == ' ') s.pop_back();
                return s;
            };
            const auto n = formats::parseNumber(trim(line.substr(eq + 1)));
            if (!n) {
                *error = "A symbol line is name = number.";
                return std::nullopt;
            }
            t.add(trim(line.substr(0, eq)), static_cast<std::uint64_t>(*n));
        }
        return t;
    }
};

// The program of the tests (TestCpu.hpp): a loop with a program step
// boundary, a store, a call and a return.
//   00 nop             ; boundary
//   01 ld a, #5
//   03 st a, [0x30]
//   05 call 0x10
//   07 nop             ; boundary
//   08 ld a, [0x31]
//   0a jmp 0x00
//   10 ld a, [0x30]    ; the routine
//   12 out 0x02, a
//   14 ret
inline mt::TestProgram defaultProgram() {
    using namespace mt::cpu;
    std::vector<std::uint8_t> p(0x20, 0);
    const std::uint8_t main[] = {kNop, kLdA, 5, kSt, 0x30, kCall, 0x10, kNop, kLdMem, 0x31, kJmp, 0x00};
    std::copy(std::begin(main), std::end(main), p.begin());
    const std::uint8_t routine[] = {kLdMem, 0x30, kOut, 0x02, kRet};
    std::copy(std::begin(routine), std::end(routine), p.begin() + 0x10);
    return mt::TestProgram(p, {0, 7});
}

} // namespace calcu::debugger::test
