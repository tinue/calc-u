#pragma once
#include "machine/DebugTarget.hpp"
#include "machine/ProductData.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// The test processor of LIB-10 (design §6, "The test machines"; LIB-1
// AC-24), switched on by the build option TestProgram: an 8-bit accumulator
// processor behind test-seg's target `test-cpu` and test-dot's `main` and
// `inner`, which share the bus by turns (one instruction each). Library tests
// use it, never product code.
//
// Registers: a, x (8 bits), pc (16 bits, read-only), sp (16 bits), z (1 bit).
// Instructions (opcode, cells):
//   00 nop             a program step boundary (mark kProgramStep)
//   01 ld a, #n   (2)  a = n; z = a == 0
//   02 ld a, [m]  (2)  a = memory[m] (a data read)
//   03 st a, [m]  (2)  memory[m] = a (a data write; a ROM cell keeps its value)
//   04 in a, p    (2)  a = io[p] (write-only ports read 0xff; a device port counts up)
//   05 out p, a   (2)  io[p] = a; port 0 switches the bank, port 1 the vertical bank
//   06 jmp m      (2)
//   07 jz m       (2)
//   08 call m     (2)  sp -= 1, memory[sp] = the return address (a data write)
//   09 ret        (1)  the return address from memory[sp] (a data read), sp += 1
//   0a wait       (1)  x -= 1 while x > 0; z = x == 0; stays at its address while z is clear
//   0b ld x, #n   (2)  x = n
//   0c st2 a, [m] (2)  memory[m] = memory[m + 1] = a (two data writes)
//   others        (1)  `db 0x..`, nothing done
// Spaces: `memory` (256 cells of 8 bits; 0x80-0xbf banked: 4 banks, bank 3
// with 2 vertical banks; 0xf0-0xff ROM, writes refused), `io` (16 cells: 0-7
// write-only, 8-15 a device whose reading would disturb it, so unreadable)
// and `io-written` (AC-48). Qualifier field `bank` (0..3), decoding, a
// history of 8, watches, step over and out (callDepth: 0x10000 - sp).
// Layout: the program at 0x00, its data at 0x30, the inner target's program
// at 0x50 (its stack below 0x70), the main stack below 0x80.

namespace calcu::machine::test {

struct TestProgram : ProductData {
    TestProgram() = default;
    explicit TestProgram(std::vector<std::uint8_t> p, std::vector<std::uint8_t> d = {},
                         std::vector<std::uint8_t> i = {})
        : program(std::move(p)), data(std::move(d)), inner(std::move(i)) {}
    std::vector<std::uint8_t> program, data, inner;
    bool bare = false;   // the targets offer none of the optional parts (no watches, stepping over and out, decoding, history, qualifier)
};

namespace cpu {
inline constexpr std::uint8_t kNop = 0x00, kLdA = 0x01, kLdMem = 0x02, kSt = 0x03, kIn = 0x04, kOut = 0x05,
                              kJmp = 0x06, kJz = 0x07, kCall = 0x08, kRet = 0x09, kWait = 0x0a, kLdX = 0x0b,
                              kSt2 = 0x0c;
inline constexpr std::uint64_t kDataAt = 0x30, kInnerAt = 0x50, kInnerStack = 0x70, kMainStack = 0x80;
inline constexpr std::uint64_t kBankFirst = 0x80, kBankCount = 0x40, kRomFirst = 0xf0;
inline constexpr std::uint32_t kProgramStep = 1;   // the mark of a `nop`
inline constexpr std::size_t kMemory = 0, kIo = 1, kIoWritten = 2;   // space indexes
inline constexpr std::size_t kA = 0, kX = 1, kPc = 2, kSp = 3, kZ = 4;   // register indexes
} // namespace cpu

class TestCpu final : public Debugging {
public:
    // One target (`test-cpu`) or two (`main`, `inner`).
    TestCpu(std::vector<DebugTargetId> names, TestProgram);
    ~TestCpu();

    // Debugging
    std::size_t targetCount() const override { return cores_.size(); }
    DebugTarget& target(std::size_t i) override;
    const DebugTarget& target(std::size_t i) const override;
    std::size_t current() const override { return turn_; }
    void setChecks(const Checks& c) override { checks_ = c; }
    std::optional<DebugStop> takeStop() override { return std::exchange(stop_, std::nullopt); }
    void passOnce(std::size_t t) override;

    // The machine's advance: before() is false when the advance stops before
    // the next instruction (a breakpoint); execute() carries it out and is
    // true when the advance stops after it (a watch, the observer).
    bool before();
    bool execute();
    void reset();       // the machine's reset and power cycle: registers, banks
    void coldBoot();    // and the memory loaded again, io cleared

    // For the tests and the trace recorder of TestDebugParts.hpp.
    std::uint8_t memoryAt(std::uint64_t a) const { return mem_[a & 0xff]; }
    std::uint32_t bank() const { return bank_; }
    std::uint32_t vbank() const { return vbank_; }
    std::uint8_t deviceReads(std::size_t port) const { return device_[port & 7]; }
    bool recording = false;
    std::vector<std::pair<std::uint8_t, std::uint8_t>> recorded;   // (target, address) per instruction while recording

private:
    class Core;
    std::uint8_t readMem(std::uint64_t a) const;                  // as the processor sees it now
    std::uint8_t* cell(std::uint64_t a, BankSelect);              // nullptr: none
    const std::uint8_t* cell(std::uint64_t a, BankSelect) const;
    void writeIo(std::uint64_t p, std::uint8_t v);
    void load();

    TestProgram program_;
    std::vector<std::unique_ptr<Core>> cores_;
    std::array<std::uint8_t, 256> mem_{};
    std::array<std::array<std::uint8_t, 64>, 5> banks_{};   // banks 0-2, bank 3 vbank 0, bank 3 vbank 1
    std::array<std::optional<std::uint8_t>, 16> written_{};
    std::array<std::uint8_t, 8> device_{};
    std::uint32_t bank_ = 0, vbank_ = 0;
    std::size_t turn_ = 0;
    Checks checks_;
    std::optional<DebugStop> stop_;
    std::optional<std::size_t> pass_;
    std::vector<WatchHit> hits_;   // of the instruction being carried out
};

// The text of one instruction at `bytes` (two cells; the second may be unused).
Decoded decodeTestInstruction(std::uint8_t op, std::uint8_t arg);

} // namespace calcu::machine::test
