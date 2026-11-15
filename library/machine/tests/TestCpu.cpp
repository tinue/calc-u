#include "TestCpu.hpp"
#include <algorithm>
#include <cstdio>

namespace calcu::machine::test {

using namespace cpu;

namespace {
std::string hex2(std::uint64_t v) {
    char buf[8];
    std::snprintf(buf, sizeof buf, "0x%02x", static_cast<unsigned>(v & 0xff));
    return buf;
}

TargetInfo infoFor(const DebugTargetId& name, bool bare) {
    TargetInfo i;
    i.name = name;
    i.registers = {RegisterInfo{"a", 8, true}, RegisterInfo{"x", 8, true}, RegisterInfo{"pc", 16, false},
                   RegisterInfo{"sp", 16, true}, RegisterInfo{"z", 1, true}};
    SpaceInfo memory{"memory", 8, 256, false, {BankWindow{kBankFirst, kBankCount, 4, 2}}};
    i.spaces = {memory, SpaceInfo{"io", 8, 16, false, {}}, SpaceInfo{"io-written", 8, 16, true, {}}};
    i.codeSpace = kMemory;
    i.offers.watches = true;
    i.offers.stepOverOut = true;
    i.offers.decode = true;
    i.offers.history = 8;
    i.offers.qualifier = {QualifierField{"bank", {"0", "1", "2", "3"}}};
    if (bare) i.offers = TargetOffers{};
    return i;
}

int cellsOf(std::uint8_t op) {
    switch (op) {
    case kLdA: case kLdMem: case kSt: case kIn: case kOut: case kJmp: case kJz: case kCall: case kLdX: case kSt2: return 2;
    default: return 1;
    }
}
} // namespace

Decoded decodeTestInstruction(std::uint8_t op, std::uint8_t arg) {
    const std::string n = hex2(arg);
    switch (op) {
    case kNop: return {"nop", 1};
    case kLdA: return {"ld a, #" + n, 2};
    case kLdMem: return {"ld a, [" + n + "]", 2};
    case kSt: return {"st a, [" + n + "]", 2};
    case kIn: return {"in a, " + n, 2};
    case kOut: return {"out " + n + ", a", 2};
    case kJmp: return {"jmp " + n, 2};
    case kJz: return {"jz " + n, 2};
    case kCall: return {"call " + n, 2};
    case kRet: return {"ret", 1};
    case kWait: return {"wait", 1};
    case kLdX: return {"ld x, #" + n, 2};
    case kSt2: return {"st2 a, [" + n + "]", 2};
    default: return {"db " + hex2(op), 1};
    }
}

class TestCpu::Core final : public DebugTarget {
public:
    Core(TestCpu& cpu, DebugTargetId name, std::uint64_t pc, std::uint64_t sp)
        : cpu_(cpu), info_(infoFor(name, cpu.program_.bare)), startPc_(pc), startSp_(sp) {
        reset();
    }
    void reset() {
        regs_ = {0, 0, startPc_, startSp_, 0};
        next_ = kept_ = 0;
    }

    const TargetInfo& info() const override { return info_; }
    std::uint64_t nextAddress() const override { return regs_[kPc]; }
    std::uint64_t readRegister(std::size_t r) const override { return r < regs_.size() ? regs_[r] : 0; }
    bool writeRegister(std::size_t r, std::uint64_t v) override {
        if (r >= regs_.size() || !info_.registers[r].writable) return false;
        const int bits = info_.registers[r].bits;
        if (bits < 64 && v >> bits) return false;   // does not fit: refused, nothing changed
        regs_[r] = v;
        return true;
    }
    CellRead peek(std::size_t space, std::uint64_t a, BankSelect b) const override {
        if (space == kMemory) {
            const std::uint8_t* c = cpu_.cell(a, b);
            if (!c) return CellRead{CellRead::Unreadable, 0};
            return CellRead{CellRead::Value, *c};
        }
        if (space == kIo) return CellRead{CellRead::Unreadable, 0};   // write-only or a device (AC-2)
        if (space == kIoWritten) {
            if (a >= 16 || !cpu_.written_[a]) return CellRead{CellRead::NoneWritten, 0};
            return CellRead{CellRead::Value, *cpu_.written_[a]};
        }
        return CellRead{CellRead::Unreadable, 0};
    }
    bool poke(std::size_t space, std::uint64_t a, std::uint64_t v, BankSelect b) override {
        if (v > 0xff) return false;
        if (space == kMemory) {
            if (a >= kRomFirst && a < 256) return false;   // ROM
            std::uint8_t* c = cpu_.cell(a, b);
            if (!c) return false;
            *c = static_cast<std::uint8_t>(v);
            return true;
        }
        if (space == kIo && a < 16) {
            cpu_.writeIo(a, static_cast<std::uint8_t>(v));
            return true;
        }
        return false;   // the written view is read-only
    }
    std::vector<HistoryEntry> history(std::size_t count) const override {
        std::vector<HistoryEntry> out;
        for (std::size_t k = 0; k < kept_ && out.size() < count; ++k) {
            const Past& p = past_[(next_ + past_.size() - 1 - k) % past_.size()];
            out.push_back(HistoryEntry{p.address, std::vector<std::uint64_t>(p.regs.begin(), p.regs.end()),
                                       decode(p.address)->text});
        }
        return out;
    }
    std::optional<Decoded> decode(std::uint64_t a) const override {
        return decodeTestInstruction(cpu_.readMem(a), cpu_.readMem(a + 1));
    }
    std::vector<std::string> qualifierNow() const override { return {std::to_string(cpu_.bank_)}; }
    bool qualifierMatches(const std::vector<std::string>& wanted, std::uint64_t) const override {
        return wanted.empty() || wanted[0].empty() || wanted[0] == std::to_string(cpu_.bank_);
    }
    Flow flowAt(std::uint64_t a) const override {
        const std::uint8_t op = cpu_.readMem(a);
        return op == kCall ? Flow::Call : op == kRet ? Flow::Return : Flow::Other;
    }
    std::uint64_t callDepth() const override { return 0x10000 - regs_[kSp]; }
    std::uint64_t retired() const override { return retired_; }
    void setBreakpoints(std::vector<std::uint64_t> a) override { breakpoints_ = std::move(a); }
    bool setWatches(std::vector<WatchRange> w) override {
        watches_ = std::move(w);
        return true;
    }

    bool atBreakpoint() const {
        return std::find(breakpoints_.begin(), breakpoints_.end(), regs_[kPc]) != breakpoints_.end();
    }
    bool watched(std::size_t space, std::uint64_t a, bool write) const {
        return std::any_of(watches_.begin(), watches_.end(), [&](const WatchRange& w) {
            return w.space == space && a >= w.first && a < w.first + w.count && (write ? w.write : w.read);
        });
    }

    // One instruction; the event of it.
    InstructionEvent run(std::size_t index) {
        if (info_.offers.history) {   // a ring, no allocation per instruction
            past_[next_] = Past{regs_[kPc], regs_};
            next_ = (next_ + 1) % past_.size();
            kept_ = std::min(kept_ + 1, past_.size());
        }
        const std::uint64_t at = regs_[kPc];
        const std::uint8_t op = cpu_.readMem(at), arg = cpu_.readMem(at + 1);
        std::uint64_t next = (at + static_cast<std::uint64_t>(cellsOf(op))) & 0xff;
        std::uint64_t& a = regs_[kA];
        std::uint64_t& x = regs_[kX];
        std::uint64_t& sp = regs_[kSp];
        auto data = [&](std::size_t space, std::uint64_t addr, bool write) {
            if (cpu_.checks_.watches && info_.offers.watches && watched(space, addr, write)) cpu_.hits_.push_back(WatchHit{space, addr, write});
        };
        switch (op) {
        case kLdA: a = arg; regs_[kZ] = a == 0; break;
        case kLdMem: data(kMemory, arg, false); a = cpu_.readMem(arg); regs_[kZ] = a == 0; break;
        case kSt:
            data(kMemory, arg, true);
            if (arg < kRomFirst)
                if (std::uint8_t* c = cpu_.cell(arg, {})) *c = static_cast<std::uint8_t>(a);
            break;
        case kIn: {
            const std::uint64_t p = arg & 0x0f;
            data(kIo, p, false);
            if (p < 8) a = 0xff;
            else a = cpu_.device_[p - 8]++;   // reading disturbs the device
            regs_[kZ] = a == 0;
            break;
        }
        case kOut: data(kIo, arg & 0x0f, true); cpu_.writeIo(arg & 0x0f, static_cast<std::uint8_t>(a)); break;
        case kJmp: next = arg; break;
        case kJz: if (regs_[kZ]) next = arg; break;
        case kCall:
            sp = (sp - 1) & 0xffff;
            data(kMemory, sp & 0xff, true);
            if (std::uint8_t* c = cpu_.cell(sp & 0xff, {})) *c = static_cast<std::uint8_t>(next);
            next = arg;
            break;
        case kRet:
            data(kMemory, sp & 0xff, false);
            next = cpu_.readMem(sp & 0xff);
            sp = (sp + 1) & 0xffff;
            break;
        case kWait:
            if (x > 0) --x;
            regs_[kZ] = x == 0;
            if (!regs_[kZ]) next = at;
            break;
        case kLdX: x = arg; break;
        case kSt2:
            for (std::uint64_t k = 0; k < 2; ++k) {
                const std::uint64_t to = (arg + k) & 0xff;
                data(kMemory, to, true);
                if (to < kRomFirst)
                    if (std::uint8_t* c = cpu_.cell(to, {})) *c = static_cast<std::uint8_t>(a);
            }
            break;
        default: break;
        }
        regs_[kPc] = next;
        ++retired_;
        return InstructionEvent{index, at, next, op == kNop ? kProgramStep : 0u};
    }

private:
    TestCpu& cpu_;
    TargetInfo info_;
    std::uint64_t startPc_, startSp_;
    std::array<std::uint64_t, 5> regs_{};
    std::uint64_t retired_ = 0;
    struct Past {
        std::uint64_t address = 0;
        std::array<std::uint64_t, 5> regs{};
    };
    std::array<Past, 8> past_{};
    std::size_t next_ = 0, kept_ = 0;
    std::vector<std::uint64_t> breakpoints_;
    std::vector<WatchRange> watches_;
};

TestCpu::TestCpu(std::vector<DebugTargetId> names, TestProgram p) : program_(std::move(p)) {
    for (std::size_t i = 0; i < names.size(); ++i)
        cores_.push_back(std::make_unique<Core>(*this, names[i], i == 0 ? 0 : kInnerAt, i == 0 ? kMainStack : kInnerStack));
    load();
}

TestCpu::~TestCpu() = default;
DebugTarget& TestCpu::target(std::size_t i) { return *cores_[i]; }
const DebugTarget& TestCpu::target(std::size_t i) const { return *cores_[i]; }

void TestCpu::load() {
    mem_.fill(0);
    for (auto& b : banks_) b.fill(0);
    for (std::size_t i = 0; i < program_.program.size() && i < kDataAt; ++i) mem_[i] = program_.program[i];
    for (std::size_t i = 0; i < program_.data.size() && kDataAt + i < kInnerAt; ++i) mem_[kDataAt + i] = program_.data[i];
    if (program_.inner.empty()) {   // the inner target loops on itself
        mem_[kInnerAt] = kJmp;
        mem_[kInnerAt + 1] = static_cast<std::uint8_t>(kInnerAt);
    }
    for (std::size_t i = 0; i < program_.inner.size() && kInnerAt + i < 0x60; ++i) mem_[kInnerAt + i] = program_.inner[i];
    for (std::uint64_t a = kRomFirst; a < 256; ++a) mem_[a] = static_cast<std::uint8_t>(0xa0 + (a - kRomFirst));
    // Each bank cell starts as its bank's mark, so a reading names the bank.
    for (std::size_t b = 0; b < banks_.size(); ++b) banks_[b].fill(static_cast<std::uint8_t>(0x10 * (b + 1)));
    written_.fill(std::nullopt);
    device_.fill(0);
}

void TestCpu::reset() {
    for (auto& c : cores_) c->reset();
    bank_ = vbank_ = 0;
    turn_ = 0;
    pass_.reset();
    stop_.reset();
}

void TestCpu::coldBoot() {
    reset();
    load();
}

void TestCpu::passOnce(std::size_t t) { pass_ = t; }

const std::uint8_t* TestCpu::cell(std::uint64_t a, BankSelect b) const {
    if (a >= 256) return nullptr;
    if (a < kBankFirst || a >= kBankFirst + kBankCount) return b.bank || b.vbank ? nullptr : &mem_[a];
    const std::uint32_t bank = b.bank ? *b.bank : bank_;
    std::uint32_t v = b.vbank ? *b.vbank : (bank == 3 ? vbank_ : 0);
    if (bank > 3 || v > 1 || (bank != 3 && b.vbank && *b.vbank != 0)) return nullptr;
    if (bank != 3) v = 0;
    const std::size_t index = bank < 3 ? bank : 3 + v;
    return &banks_[index][a - kBankFirst];
}

std::uint8_t* TestCpu::cell(std::uint64_t a, BankSelect b) {
    return const_cast<std::uint8_t*>(static_cast<const TestCpu*>(this)->cell(a, b));
}

std::uint8_t TestCpu::readMem(std::uint64_t a) const {
    const std::uint8_t* c = cell(a & 0xff, {});
    return c ? *c : 0;
}

void TestCpu::writeIo(std::uint64_t p, std::uint8_t v) {
    written_[p & 0x0f] = v;
    if (p == 0) bank_ = v & 3;
    if (p == 1) vbank_ = v & 1;
}

bool TestCpu::before() {
    Core& c = *cores_[turn_];
    if (checks_.breakpoints && c.atBreakpoint() && pass_ != turn_) {
        stop_ = DebugStop{DebugStop::Breakpoint, turn_, c.nextAddress(), {}};
        return false;
    }
    return true;
}

bool TestCpu::execute() {
    const std::size_t t = turn_;
    if (pass_ == t) pass_.reset();   // passed once
    hits_.clear();
    const InstructionEvent ev = cores_[t]->run(t);
    if (recording) recorded.emplace_back(static_cast<std::uint8_t>(t), static_cast<std::uint8_t>(ev.address));
    turn_ = (turn_ + 1) % cores_.size();
    const bool observed = checks_.observer && checks_.observer->stopAfter(ev);
    if (!hits_.empty()) {
        stop_ = DebugStop{DebugStop::Watch, t, ev.address, hits_};
        return true;
    }
    if (observed) {
        stop_ = DebugStop{DebugStop::Observer, t, ev.address, {}};
        return true;
    }
    return false;
}

} // namespace calcu::machine::test
