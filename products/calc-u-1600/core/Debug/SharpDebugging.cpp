#include "SharpDebugging.hpp"

#include <algorithm>
#include <cstdlib>

#include "../PC1500/PC1500Machine.hpp"
#include "../PC1600/PC1600Machine.hpp"
#include "Disasm/LH5801Disassembler.hpp"
#include "Disasm/Z80Disassembler.hpp"

namespace sharp {

using calcu::machine::BankSelect;
using calcu::machine::CellRead;
using calcu::machine::Ticks;

namespace {

disasm::Decoded decodeAs(debug::CpuKind kind, std::uint16_t addr, const disasm::FetchFn& fetch,
                         const disasm::SymbolFn& symbols) {
    return kind == debug::CpuKind::Z80 ? disasm::decodeZ80(addr, fetch, symbols)
                                       : disasm::decodeLH5801(addr, fetch, symbols);
}

std::vector<std::string> values(int n) {
    std::vector<std::string> v;
    for (int i = 0; i < n; ++i) v.push_back(std::to_string(i));
    return v;
}

calcu::machine::SpaceInfo space(const char* name, bool written = false, std::vector<calcu::machine::BankWindow> banked = {},
                                std::uint64_t size = 0x10000) {
    calcu::machine::SpaceInfo s;
    s.name = name;
    s.cellBits = 8;
    s.size = size;
    s.writtenView = written;
    s.banked = std::move(banked);
    return s;
}

// The LH580x's spaces and qualifier: ME0 and ME1 banked by PU and PV.
std::vector<calcu::machine::SpaceInfo> lhSpaces() {
    const std::vector<calcu::machine::BankWindow> pupv{{0, 0x10000, 4, 0}};
    return {space("me0", false, pupv), space("me1", false, pupv), space("me1-written", true)};
}
std::vector<calcu::machine::QualifierField> lhQualifier() {
    return {{"me", values(2)}, {"pu", values(2)}, {"pv", values(2)}};
}

int fieldOf(const std::vector<std::string>& v, std::size_t i) {
    if (i >= v.size() || v[i].empty()) return -1;
    return std::atoi(v[i].c_str());
}

} // namespace

// ── SharpDebugTarget ─────────────────────────────────────────────────────

SharpDebugTarget::SharpDebugTarget(SharpDebugging& owner, std::size_t index, std::string name,
                                   std::unique_ptr<debug::CpuView> view, std::vector<calcu::machine::SpaceInfo> spaces,
                                   std::vector<calcu::machine::QualifierField> qualifier)
    : m_owner(owner), m_index(index), m_view(std::move(view)) {
    m_info.name = calcu::machine::DebugTargetId{std::move(name)};
    for (const debug::Register& r : m_view->registers()) m_info.registers.push_back({r.name, r.bits, true});
    m_info.spaces = std::move(spaces);
    m_info.codeSpace = 0;
    m_info.offers.watches = true;
    m_info.offers.stepOverOut = true;
    m_info.offers.decode = true;
    m_info.offers.history = 32;
    m_info.offers.qualifier = std::move(qualifier);
}

SharpDebugTarget::~SharpDebugTarget() = default;

std::uint64_t SharpDebugTarget::readRegister(std::size_t i) const {
    const std::vector<debug::Register> regs = m_view->registers();
    return i < regs.size() ? regs[i].value : 0;
}

bool SharpDebugTarget::writeRegister(std::size_t i, std::uint64_t value) {
    if (i >= m_info.registers.size()) return false;
    const int bits = m_info.registers[i].bits;
    if (bits < 64 && value >= (std::uint64_t{1} << bits)) return false;
    return m_view->writeRegister(m_info.registers[i].name, static_cast<std::uint32_t>(value));
}

CellRead SharpDebugTarget::peek(std::size_t space, std::uint64_t address, BankSelect bank) const {
    if (space >= m_info.spaces.size() || address >= m_info.spaces[space].size) return CellRead{CellRead::Unreadable, 0};
    return m_owner.peekCell(m_index, space, address, bank);
}

bool SharpDebugTarget::poke(std::size_t space, std::uint64_t address, std::uint64_t value, BankSelect bank) {
    if (space >= m_info.spaces.size() || address >= m_info.spaces[space].size || value > 0xFF) return false;
    return m_owner.pokeCell(m_index, space, address, static_cast<std::uint8_t>(value), bank);
}

bool SharpDebugTarget::peekByte(debug::Space space, std::uint16_t address, std::uint8_t* value) const {
    const CellRead c = m_owner.peekCell(m_index, space == debug::kSpaceME1 ? 1 : 0, address, {});
    if (c.kind != CellRead::Value) return false;
    *value = static_cast<std::uint8_t>(c.value);
    return true;
}

bool SharpDebugTarget::pokeByte(debug::Space space, std::uint16_t address, std::uint8_t value) {
    return m_owner.pokeCell(m_index, space == debug::kSpaceME1 ? 1 : 0, address, value, {});
}

std::vector<calcu::machine::HistoryEntry> SharpDebugTarget::history(std::size_t count) const {
    std::vector<calcu::machine::HistoryEntry> out;
    const std::size_t n = std::min<std::size_t>(count, m_view->historySize());
    for (std::size_t age = 0; age < n; ++age) {
        const debug::HistoryEntry h = m_view->history(static_cast<std::uint32_t>(age));
        calcu::machine::HistoryEntry e;
        e.address = h.pc;
        for (const debug::Register& r : h.registers) e.registers.push_back(r.value);
        if (h.interrupt || h.len == 0) e.text = "(interrupt)";
        else e.text = decodeEntry(h).text;
        out.push_back(std::move(e));
    }
    return out;
}

std::optional<calcu::machine::Decoded> SharpDebugTarget::decode(std::uint64_t address) const {
    if (address > 0xFFFF) return std::nullopt;
    const disasm::Decoded d = decodeAt(static_cast<std::uint16_t>(address));
    return calcu::machine::Decoded{d.text, d.len};
}

disasm::Decoded SharpDebugTarget::decodeAt(std::uint16_t address, const disasm::SymbolFn& symbols) const {
    auto fetch = [this](std::uint16_t a) -> std::uint8_t {
        std::uint8_t v = 0xFF;
        peekByte(debug::kSpaceMain, a, &v);   // code is fetched from ME0 / the Z-80's memory
        return v;
    };
    return decodeAs(kind(), address, fetch, symbols);
}

disasm::Decoded SharpDebugTarget::decodeEntry(const debug::HistoryEntry& entry, const disasm::SymbolFn& symbols) const {
    auto fetch = [&entry](std::uint16_t a) -> std::uint8_t {
        const std::uint16_t i = std::uint16_t(a - entry.pc);
        return i < entry.len ? entry.bytes[i] : 0;
    };
    return decodeAs(kind(), entry.pc, fetch, symbols);
}

calcu::machine::Flow SharpDebugTarget::flowAt(std::uint64_t address) const {
    if (address > 0xFFFF) return calcu::machine::Flow::Other;
    switch (decodeAt(static_cast<std::uint16_t>(address)).flow) {
    case disasm::Flow::Call:
    case disasm::Flow::CondCall:
    case disasm::Flow::Vector: return calcu::machine::Flow::Call;
    case disasm::Flow::Return:
    case disasm::Flow::CondReturn: return calcu::machine::Flow::Return;
    default: return calcu::machine::Flow::Other;
    }
}

int SharpDebugTarget::bankAt(std::uint16_t address) const { return m_owner.bankAt(m_index, address); }

std::vector<debug::BankField> SharpDebugTarget::bankState() const { return m_owner.bankState(m_index); }

std::vector<std::string> SharpDebugTarget::qualifierNow() const {
    if (kind() == debug::CpuKind::Z80) return {std::to_string(bankAt(m_view->pc()))};
    return {"0", m_view->pu() ? "1" : "0", m_view->pv() ? "1" : "0"};   // code runs from ME0
}

bool SharpDebugTarget::qualifierMatches(const std::vector<std::string>& wanted, std::uint64_t address) const {
    debug::BankKey key;
    if (kind() == debug::CpuKind::Z80) {
        key.bank = fieldOf(wanted, 0);
    } else {
        key.me = fieldOf(wanted, 0);
        key.pu = fieldOf(wanted, 1);
        key.pv = fieldOf(wanted, 2);
    }
    return bankMatches(key, static_cast<std::uint16_t>(address));
}

bool SharpDebugTarget::bankMatches(const debug::BankKey& key, std::uint16_t address) const {
    if (key.me == 1) return false;   // code runs from ME0 only
    if (key.bank >= 0 && bankAt(address) != key.bank) return false;
    if (key.pu >= 0 && m_view->pu() != (key.pu != 0)) return false;
    if (key.pv >= 0 && m_view->pv() != (key.pv != 0)) return false;
    return true;
}

std::vector<std::string> SharpDebugTarget::qualifierOf(const debug::BankKey& key) const {
    if (!key.any()) return {};
    auto text = [](int v) { return v < 0 ? std::string() : std::to_string(v); };
    if (kind() == debug::CpuKind::Z80) return {text(key.bank)};
    return {text(key.me), text(key.pu), text(key.pv)};
}

debug::ExpressionContext SharpDebugTarget::expressionContext(std::function<bool(const std::string&, std::int64_t*)> symbols) const {
    debug::ExpressionContext ctx;
    ctx.lookup = [this, symbols](const std::string& name, std::int64_t* value) {
        std::uint32_t reg = 0;
        if (readRegister(name, &reg)) {
            *value = reg;
            return true;
        }
        return symbols ? symbols(name, value) : false;
    };
    ctx.readByte = [this](std::uint16_t addr, bool me1, std::uint8_t* value) {
        return peekByte(me1 ? debug::kSpaceME1 : debug::kSpaceMain, addr, value);
    };
    ctx.bigEndian = kind() != debug::CpuKind::Z80;
    return ctx;
}

void SharpDebugTarget::setBreakpoints(std::vector<std::uint64_t> addresses) {
    m_breakpoints.clear();
    for (std::uint64_t a : addresses)
        if (a <= 0xFFFF) m_breakpoints.push_back(static_cast<std::uint16_t>(a));
    std::sort(m_breakpoints.begin(), m_breakpoints.end());
    m_breakpoints.erase(std::unique(m_breakpoints.begin(), m_breakpoints.end()), m_breakpoints.end());
    m_view->setBreakpoints(m_breakpoints);
}

bool SharpDebugTarget::setWatches(std::vector<calcu::machine::WatchRange> ranges) {
    m_watches.clear();
    for (const auto& r : ranges) {
        // The CPUs watch ME0 / the Z-80's memory and ME1; the Z-80's I/O
        // ports and the written views have no watch.
        if (r.space > 1 || (kind() == debug::CpuKind::Z80 && r.space != 0) || r.count == 0) continue;
        WatchSet::Watch w;
        w.lo = static_cast<std::uint16_t>(r.first);
        w.hi = static_cast<std::uint16_t>(std::min<std::uint64_t>(r.first + r.count - 1, 0xFFFF));
        w.space = static_cast<std::uint8_t>(r.space);
        w.read = r.read;
        w.write = r.write;
        m_watches.add(w);
    }
    return true;
}

// ── SharpDebugging ───────────────────────────────────────────────────────

SharpDebugging::~SharpDebugging() = default;

void SharpDebugging::addTarget(std::string name, std::unique_ptr<debug::CpuView> view,
                               std::vector<calcu::machine::SpaceInfo> spaces,
                               std::vector<calcu::machine::QualifierField> qualifier) {
    m_targets.push_back(std::make_unique<SharpDebugTarget>(*this, m_targets.size(), std::move(name), std::move(view),
                                                           std::move(spaces), std::move(qualifier)));
}

void SharpDebugging::detachAll() {
    for (std::size_t i = 0; i < m_targets.size(); ++i) {
        attachWatchSet(i, nullptr);
        m_targets[i]->view().setBreakpoints({});
        m_targets[i]->view().enableBreakpoints(false);
    }
}

void SharpDebugging::setChecks(const calcu::machine::Checks& c) {
    m_checks = c;
    armChecks();
}

// While an operation runs the library leaves breakpoints and watches off
// (G1): the CPUs check them only while the Checks say so.
void SharpDebugging::armChecks() {
    for (std::size_t i = 0; i < m_targets.size(); ++i) {
        SharpDebugTarget& t = *m_targets[i];
        t.view().enableBreakpoints(m_checks.breakpoints && !t.breakpoints().empty());
        attachWatchSet(i, m_checks.watches && !t.watchSet().empty() ? &t.watchSet() : nullptr);
    }
}

std::optional<calcu::machine::DebugStop> SharpDebugging::takeStop() {
    std::optional<calcu::machine::DebugStop> s = std::move(m_stop);
    m_stop.reset();
    return s;
}

void SharpDebugging::latch(const ::DebugStop& core) {
    if (core.kind == ::DebugStop::None || core.cpu == 0) return;
    const std::size_t t = std::min<std::size_t>(core.cpu - 1u, m_targets.size() - 1);
    SharpDebugTarget& target = *m_targets[t];
    calcu::machine::DebugStop s;
    s.target = t;
    if (core.kind == ::DebugStop::Breakpoint) {
        s.kind = calcu::machine::DebugStop::Breakpoint;
        s.address = target.pc();
    } else {
        if (!target.watchSet().hitPending()) return;
        const WatchHit h = target.watchSet().consumeHit();
        s.kind = calcu::machine::DebugStop::Watch;
        s.address = h.addr;
        s.hits.push_back(calcu::machine::WatchHit{h.space, h.addr, h.write});
    }
    m_stop = std::move(s);
}

// Only the target about to run passes the breakpoint it stands on; a
// pass another target had is dropped.
void SharpDebugging::passOnce(std::size_t target) {
    for (std::size_t i = 0; i < m_targets.size(); ++i) {
        SharpDebugTarget& t = *m_targets[i];
        const auto& list = t.breakpoints();
        if (i == target && !t.halted() && std::binary_search(list.begin(), list.end(), t.pc()))
            t.view().resumePastBreakpoint();
        else
            t.view().clearBreakpointSkip();
        if (t.watchSet().hitPending()) t.watchSet().consumeHit();
    }
    consumeCoreStop();   // a stop latched but never reported is stale now
    m_stop.reset();
}

Ticks SharpDebugging::run(Ticks ticks) {
    m_stop.reset();
    if (!m_checks.observer) {
        // Breakpoints and watches: the core runs on its own path and ends the
        // advance at the latched stop.
        const Ticks r = runCore(ticks);
        latch(consumeCoreStop());
        return r;
    }
    // The observer: one machine step at a time, the observer called after
    // each instruction with the target that
    // carried it out.
    const std::uint64_t start = elapsed();
    while (elapsed() - start < ticks) {
        std::uint64_t before[2] = {0, 0};   // a Sharp machine has at most two CPUs
        for (std::size_t i = 0; i < m_targets.size() && i < 2; ++i) before[i] = m_targets[i]->retired();
        const std::uint64_t at = elapsed();
        stepCore();
        latch(consumeCoreStop());
        if (m_stop) break;
        bool ran = false;
        for (std::size_t i = 0; i < m_targets.size() && i < 2; ++i) {
            if (m_targets[i]->retired() == before[i]) continue;
            ran = true;
            const debug::HistoryEntry h = m_targets[i]->historyEntry(0);
            calcu::machine::InstructionEvent ev;
            ev.target = i;
            ev.address = h.pc;
            ev.nextAddress = m_targets[i]->pc();
            if (m_checks.observer->stopAfter(ev)) {
                calcu::machine::DebugStop s;
                s.kind = calcu::machine::DebugStop::Observer;
                s.target = i;
                s.address = ev.nextAddress;
                m_stop = std::move(s);
                break;
            }
        }
        if (m_stop) break;
        if (!ran && elapsed() == at) break;   // a machine that does not move
    }
    return elapsed() - start;
}

// ── PC-1500, PC-1500A ────────────────────────────────────────────────────

namespace {

class PC1500Debugging final : public SharpDebugging {
public:
    explicit PC1500Debugging(PC1500Machine& m) : m_machine(m) {
        // PU/PV select memory banks; the bus must see an edited value at once.
        addTarget("LH5801",
                  std::make_unique<debug::LhCpuView>(m.cpu(), debug::CpuKind::LH5801, "LH5801",
                                                     [&m](bool pu, bool pv) { m.memory().updatePUPV(pu, pv); }),
                  lhSpaces(), lhQualifier());
    }
    ~PC1500Debugging() override { detachAll(); }

    CellRead peekCell(std::size_t, std::size_t space, std::uint64_t address, BankSelect bank) const override {
        const auto a = static_cast<std::uint16_t>(address);
        if (space == 2) {
            const int w = m_machine.cpu().me1Written(a);
            return w < 0 ? CellRead{CellRead::NoneWritten, 0} : CellRead{CellRead::Value, std::uint64_t(w)};
        }
        Banked b(m_machine, bank);
        if (space == 0) return CellRead{CellRead::Value, m_machine.memory().peek(a)};
        bool readable = false;
        const std::uint8_t v = m_machine.memory().debugPeekME1(a, &readable);
        return readable ? CellRead{CellRead::Value, v} : CellRead{CellRead::Unreadable, 0};
    }
    bool pokeCell(std::size_t, std::size_t space, std::uint64_t address, std::uint8_t value, BankSelect bank) override {
        if (space == 2) return false;   // the written view is what the CPU wrote
        Banked b(m_machine, bank);
        // ME1 is I/O: a real store, with its effects, as the CPU's.
        if (space == 1) {
            m_machine.cpu().debugStoreME1(static_cast<std::uint16_t>(address), value);
            return true;
        }
        return m_machine.memory().poke(static_cast<std::uint16_t>(address), value);
    }
    int bankAt(std::size_t, std::uint16_t) const override { return -1; }
    std::vector<debug::BankField> bankState(std::size_t) const override {
        return {{"PU", m_machine.cpu().pu() ? "1" : "0"}, {"PV", m_machine.cpu().pv() ? "1" : "0"}};
    }

protected:
    std::size_t busOwner() const override { return 0; }
    Ticks runCore(Ticks n) override { return m_machine.runCycles(n); }
    void stepCore() override { m_machine.step(); }
    std::uint64_t elapsed() const override { return m_machine.elapsedCycles(); }
    ::DebugStop consumeCoreStop() override { return m_machine.consumeDebugStop(); }
    void attachWatchSet(std::size_t, WatchSet* w) override { m_machine.setWatches(1, w); }

private:
    // The PU/PV of a bank= for one access, put back after it (bank = PU * 2 + PV).
    struct Banked {
        PC1500Machine& m;
        bool set = false;
        Banked(PC1500Machine& machine, BankSelect b) : m(machine) {
            if (!b.bank) return;
            set = true;
            m.memory().updatePUPV((*b.bank & 2) != 0, (*b.bank & 1) != 0);
        }
        ~Banked() {
            if (set) m.memory().updatePUPV(m.cpu().pu(), m.cpu().pv());
        }
    };
    PC1500Machine& m_machine;
};

// ── PC-1600 ──────────────────────────────────────────────────────────────

class PC1600Debugging final : public SharpDebugging {
public:
    static constexpr std::size_t kZ80 = 0, kLh5803 = 1;

    explicit PC1600Debugging(PC1600Machine& m) : m_machine(m) {
        // The Z-80's memory by the page banks of port 31H; page B also by
        // the ROM half of bank 3 (port 3DH bit 2: vertical bank 0 is bank 3,
        // 1 the hidden bank 3b), page C by the vertical bank of a module in
        // Slot 2 (port 28H).
        std::vector<calcu::machine::BankWindow> pages{
            {0x0000, 0x4000, 2, 0}, {0x4000, 0x4000, 8, 2}, {0x8000, 0x4000, 8, 256}, {0xC000, 0x4000, 2, 0}};
        addTarget("SC7852", std::make_unique<debug::Z80CpuView>(m.sc7852()),
                  {space("memory", false, pages), space("io", false, {}, 256), space("io-written", true, {}, 256)},
                  {{"bank", values(8)}});
        // PV gates the CE-150 ROM window on the LH5803's bus.
        addTarget("LH5803",
                  std::make_unique<debug::LhCpuView>(m.lh5803(), debug::CpuKind::LH5803, "LH5803",
                                                     [&m](bool pu, bool pv) { m.lh5803Memory().updatePUPV(pu, pv); }),
                  lhSpaces(), lhQualifier());
    }
    ~PC1600Debugging() override { detachAll(); }

    CellRead peekCell(std::size_t target, std::size_t space, std::uint64_t address, BankSelect bank) const override {
        PC1600Machine& m = m_machine;
        if (target == kZ80) {
            if (space == 1) {   // io: read only where reading has no effect (the bank registers)
                switch (address) {
                case 0x28: return CellRead{CellRead::Value, m.bank().slot2VerticalBank()};
                case 0x31: return CellRead{CellRead::Value, m.bank().readPort31()};
                case 0x3C: return CellRead{CellRead::Value, m.bank().readPort3C()};
                default: return CellRead{CellRead::Unreadable, 0};
                }
            }
            if (space == 2) {
                const int w = m.sc7852().ioWritten(static_cast<std::uint8_t>(address));
                return w < 0 ? CellRead{CellRead::NoneWritten, 0} : CellRead{CellRead::Value, std::uint64_t(w)};
            }
            PageBanked b(m, static_cast<std::uint16_t>(address), bank);
            return CellRead{CellRead::Value, m.memory().peek(static_cast<std::uint16_t>(address))};
        }
        const auto a = static_cast<std::uint16_t>(address);
        if (space == 2) {
            const int w = m.lh5803().me1Written(a);
            return w < 0 ? CellRead{CellRead::NoneWritten, 0} : CellRead{CellRead::Value, std::uint64_t(w)};
        }
        LhBanked b(m, bank);
        bool readable = false;
        const std::uint8_t v = m.lh5803Memory().debugPeek(a, space == 1, &readable);
        return readable ? CellRead{CellRead::Value, v} : CellRead{CellRead::Unreadable, 0};
    }
    bool pokeCell(std::size_t target, std::size_t space, std::uint64_t address, std::uint8_t value, BankSelect bank) override {
        if (space == 2) return false;   // the written views are what the CPUs wrote
        // I/O and ME1: a real OUT or store, with its effects, as the CPU's.
        if (target == kZ80 && space == 1) {
            m_machine.sc7852().debugOut(static_cast<std::uint8_t>(address), value);
            return true;
        }
        if (target != kZ80 && space == 1) {
            LhBanked b(m_machine, bank);
            m_machine.lh5803().debugStoreME1(static_cast<std::uint16_t>(address), value);
            return true;
        }
        if (target == kZ80) {
            PageBanked b(m_machine, static_cast<std::uint16_t>(address), bank);
            return m_machine.pokeMemory(static_cast<std::uint16_t>(address), &value, 1);
        }
        // The LH5803's ME0 0000-7FFF is the Z-80's 8000-FFFF; the rest is ROM or I/O.
        std::uint16_t z80 = 0;
        if (!LH5803SharedMemory::toZ80Address(static_cast<std::uint16_t>(address), &z80)) return false;
        return m_machine.pokeMemory(z80, &value, 1);
    }
    int bankAt(std::size_t target, std::uint16_t address) const override {
        if (target != kZ80) return -1;
        const PC1600Bank& b = m_machine.bank();
        switch (address >> 14) {
        case 0: return b.pageABank();
        case 1: return b.pageBBank();
        case 2: return b.pageCBank();
        default: return b.pageDBank();
        }
    }
    std::vector<debug::BankField> bankState(std::size_t target) const override {
        if (target != kZ80)
            return {{"PU", m_machine.lh5803().pu() ? "1" : "0"}, {"PV", m_machine.lh5803().pv() ? "1" : "0"}};
        static const char* const kPages[4] = {"Page A (0000-3FFF)", "Page B (4000-7FFF)", "Page C (8000-BFFF)",
                                              "Page D (C000-FFFF)"};
        std::vector<debug::BankField> list;
        for (int p = 0; p < 4; p++) list.push_back({kPages[p], "bank " + std::to_string(bankAt(kZ80, std::uint16_t(p << 14)))});
        return list;
    }

protected:
    std::size_t busOwner() const override { return m_machine.sc7852Owns() ? kZ80 : kLh5803; }
    Ticks runCore(Ticks n) override { return m_machine.runCycles(n); }
    void stepCore() override { m_machine.step(); }
    std::uint64_t elapsed() const override { return m_machine.elapsedTStates(); }
    ::DebugStop consumeCoreStop() override { return m_machine.consumeDebugStop(); }
    void attachWatchSet(std::size_t target, WatchSet* w) override { m_machine.setWatches(target == kZ80 ? 1 : 2, w); }

private:
    // A page bank (and vertical bank) of a bank= / vbank= for one access,
    // put back after it: port 31H's field of the address's page; on page B
    // port 3DH's latch, on page C port 28H. Page C's vertical bank is the
    // module's own latch, set by OUT (28H) on the bus: it is driven for the
    // access and driven back to what it was (the OUT of the CPU is not, so
    // io-written keeps the CPU's value).
    struct PageBanked {
        PC1600Machine& m;
        std::uint8_t port31, port28, port3d;
        bool vbank = false, romHalf = false;
        PageBanked(PC1600Machine& machine, std::uint16_t address, BankSelect b)
            : m(machine), port31(machine.bank().readPort31()), port28(machine.bank().slot2VerticalBank()),
              port3d(machine.bank().port3DLatch()) {
            if (b.bank) {
                static const int kShift[4] = {0, 1, 4, 7};
                static const int kMask[4] = {0x01, 0x07, 0x07, 0x01};
                const int page = address >> 14;
                const auto field = static_cast<std::uint8_t>((*b.bank & kMask[page]) << kShift[page]);
                m.bank().writePort31(static_cast<std::uint8_t>((port31 & ~(kMask[page] << kShift[page])) | field));
            }
            if (b.vbank && address >> 14 == 1) {
                romHalf = true;
                m.bank().writePort3D(static_cast<std::uint8_t>(*b.vbank ? port3d & ~0x04 : port3d | 0x04));
            } else if (b.vbank) {
                vbank = true;
                m.memory().writeIO(0x28, static_cast<std::uint8_t>(*b.vbank));
            }
        }
        ~PageBanked() {
            m.bank().writePort31(port31);
            if (romHalf) m.bank().writePort3D(port3d);
            if (vbank) m.memory().writeIO(0x28, port28);
        }
    };
    struct LhBanked {
        PC1600Machine& m;
        bool set = false;
        LhBanked(PC1600Machine& machine, BankSelect b) : m(machine) {
            if (!b.bank) return;
            set = true;
            m.lh5803Memory().updatePUPV((*b.bank & 2) != 0, (*b.bank & 1) != 0);
        }
        ~LhBanked() {
            if (set) m.lh5803Memory().updatePUPV(m.lh5803().pu(), m.lh5803().pv());
        }
    };
    PC1600Machine& m_machine;
};

} // namespace

std::unique_ptr<SharpDebugging> makeSharpDebugging(PC1500Machine& m) { return std::make_unique<PC1500Debugging>(m); }
std::unique_ptr<SharpDebugging> makeSharpDebugging(PC1600Machine& m) { return std::make_unique<PC1600Debugging>(m); }

} // namespace sharp
