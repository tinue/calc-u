#include "PC1500/PC1500InterfaceMachine.hpp"

#include "../Connector/SoftwareDefinedCard.hpp"
#include "../Debug/Inspect/PC1500Inspector.hpp"
#include "../Machine/SharpCatalogue.hpp"
#include "../Machine/SharpDisplayOutput.hpp"
#include "PC1500TypedInput.hpp"
#include "typing/TypingSupport.hpp"

using namespace calcu::machine;

namespace {

PC1500Variant modelOf(const MachineDescriptor& d) {
    return d.model.value == sharp::kPC1500A ? PC1500Variant::PC1500A : PC1500Variant::PC1500;
}

} // namespace

// The capability objects, each over the core (C1600-1 §3.6; design §5).
struct PC1500InterfaceMachine::Kinds {
    explicit Kinds(PC1500Machine& m) : memory(m), inspector(m), support(m), trace(m), tape(m), clock(m), printerPort(m) {}

    struct Memory final : sharp::LoaderMemory {
        explicit Memory(PC1500Machine& m) : m(m) {}
        std::uint8_t peek(std::uint16_t a) const override { return m.memory().peek(a); }
        bool poke(std::uint16_t a, const std::uint8_t* data, std::size_t n) override {
            for (std::size_t i = 0; i < n; ++i)
                if (!m.memory().poke(static_cast<std::uint16_t>(a + i), data[i])) return false;
            return true;
        }
        std::vector<std::uint8_t> internalRam() const override { return {}; }
        PC1500Machine& m;
    } memory;
    struct Inspector final : sharp::InspectorViews {
        explicit Inspector(PC1500Machine& m) : m(m) {}
        std::vector<std::string> view(inspect::View v) const override { return inspect::pc1500View(m, v); }
        std::vector<inspect::MenuEntry> menu(bool dumps) const override { return inspect::pc1500Menu(m, dumps); }
        std::vector<std::uint8_t> slotImage(int slot, int* bankCount) const override {
            const ExpansionCard* card = slot == 1 ? m.memory().expansionConnector().attachedCard() : nullptr;
            *bankCount = card ? card->debugBankCount() : -1;
            return card ? card->debugImage() : std::vector<std::uint8_t>{};
        }
        PC1500Machine& m;
    } inspector;
    // LIB-5 AC-35: typing support, prompt readiness (no latches, no sample count).
    struct Support final : calcu::typing::TypingSupport {
        explicit Support(PC1500Machine& m) : m(m) {}
        std::optional<bool> promptReady() const override { return pc1500AtBasicPrompt(m); }
        PC1500Machine& m;
    } support;
    struct Trace final : sharp::CpuTrace {
        explicit Trace(PC1500Machine& m) : m(m) {}
        bool begin(std::FILE* f, std::uint32_t flags) override { return m.beginCpuTrace(f, flags); }
        void end() override { m.endCpuTrace(); }
        bool active() const override { return m.cpuTraceActive(); }
        std::uint64_t bytes() const override { return m.cpuTraceBytes(); }
        PC1500Machine& m;
    } trace;
    struct Tape final : sharp::TapeDeckAccess {
        explicit Tape(PC1500Machine& m) : m(m) {}
        bool play(const std::string& p, std::string& e) override { return m.tapePlay(p, e); }
        void record(const std::string& p) override { m.tapeRecord(p); }
        bool eject(std::string* e) override { return m.tapeEject(e); }
        TapeDeck::Status status() const override { return m.tapeStatus(); }
        std::string takeLastError() override { return m.tapeTakeLastError(); }
        PC1500Machine& m;
    } tape;
    struct Clock final : calcu::peripherals::RealTimeClock {
        explicit Clock(PC1500Machine& m) : m(m) {}
        calcu::peripherals::ClockFields read() const override {
            const Upd1990ac::Fields f = m.memory().rtc().fields();   // the uPD1990AC keeps no year
            return {std::nullopt, f.month, f.day, f.dow, f.hour, f.minute, f.second};
        }
        void set(const calcu::peripherals::ClockFields& f, int ms) override {
            const auto now = read();
            m.seedClock(f.year.value_or(2000), f.month.value_or(*now.month), f.day.value_or(*now.day),
                        f.hour.value_or(*now.hour), f.minute.value_or(*now.minute), f.second.value_or(*now.second), ms);
        }
        PC1500Machine& m;
    } clock;
    struct PrinterPort final : sharp::Ce158PrinterPort {
        explicit PrinterPort(PC1500Machine& m) : m(m) {}
        std::vector<std::uint8_t> drain() override { return m.drainCE158ParallelOutput(); }
        PC1500Machine& m;
    } printerPort;
};

PC1500InterfaceMachine::PC1500InterfaceMachine(const MachineDescriptor& d, const VariantId& v)
    : SharpMachine(d, v), m_core(std::make_unique<PC1500Machine>(modelOf(d))), m_kinds(std::make_unique<Kinds>(*m_core)) {
    m_core->setCE158SerialLink(&m_ce158Port);
}

PC1500InterfaceMachine::~PC1500InterfaceMachine() = default;

void PC1500InterfaceMachine::coreKey(const std::string& id, bool down) {
    if (down) m_core->pressKey(id);
    else m_core->releaseKey(id);
}

DisplayOutput PC1500InterfaceMachine::readDisplay() {
    const PC1500Display disp = m_core->display();
    std::vector<std::uint8_t> dots(static_cast<std::size_t>(PC1500Display::kCols) * PC1500Display::kRows, 0);
    for (int row = 0; row < PC1500Display::kRows; ++row)
        for (int col = 0; col < PC1500Display::kCols; ++col)
            dots[static_cast<std::size_t>(row) * PC1500Display::kCols + col] = disp.pixel(col, row) ? 1 : 0;
    return sharp::displayOutput(descriptor(), m_core->isDisplayOn(), PC1500Display::kCols, PC1500Display::kRows,
                                std::move(dots), disp.statusLine());
}

ExpansionCard* PC1500InterfaceMachine::moduleIn(const PlaceName& p) {
    return p.value == sharp::kSlot1 ? m_core->memory().expansionConnector().attachedCard() : nullptr;
}

void PC1500InterfaceMachine::corePlug(const PeripheralRequest& req, const PeripheralDescriptor& d) {
    const std::string& place = req.place.steps.at(0).value;
    if (place == sharp::kSlot1) {
        if (auto card = sharp::moduleCardOf(req, d)) m_core->attachExpansionCard(std::move(card));
        return;
    }
    const std::vector<std::uint8_t>* rom = sharp::romImageOf(req, d, 0);
    if (!rom) return;
    if (req.peripheral.value == sharp::kCE150) m_core->attachCE150(rom->data(), rom->size());
    else if (req.peripheral.value == sharp::kCE158) m_core->attachCE158(rom->data(), rom->size());
}

void PC1500InterfaceMachine::coreUnplug(const PlaceName& p, const PeripheralId& id) {
    if (p.value == sharp::kSlot1) m_core->expansionConnector().detach();
    else if (id.value == sharp::kCE150) m_core->detachCE150();
    else if (id.value == sharp::kCE158) m_core->detachCE158();
}

std::vector<PlugEntry> PC1500InterfaceMachine::coreHasPlugged() const {
    std::vector<PlugEntry> out;
    if (const ExpansionCard* card = m_core->memory().expansionConnector().attachedCard())
        out.push_back({PlacePath{{PlaceName{sharp::kSlot1}}}, PeripheralId{card->moduleName()}, std::nullopt});
    if (m_core->ce150Attached()) out.push_back({PlacePath{{PlaceName{sharp::kPrinter}}}, PeripheralId{sharp::kCE150}, std::nullopt});
    if (m_core->ce158Attached())
        out.push_back({PlacePath{{PlaceName{sharp::kInterface}}}, PeripheralId{sharp::kCE158}, std::nullopt});
    return out;
}

void PC1500InterfaceMachine::offer(std::vector<Offered>& out, const std::vector<PlugEntry>&) {
    Kinds& k = *m_kinds;
    out.push_back(byComponent("LH5801", k.memory));
    out.push_back(byComponent("LH5801", k.inspector));
    out.push_back(byComponent("LH5801", k.support));
    out.push_back(byComponent("LH5801", k.trace));
    out.push_back(byComponent("LH5801", k.tape));
    out.push_back(byComponent(sharp::kRtcComponent, k.clock));
    // LIB-8: the plotter's paper output, once its card is on the bus.
    if (const PlugEntry* e = at(sharp::kPrinter); e && e->peripheral.value == sharp::kCE150)
        if (calcu::paper::PaperOutput* paper = m_core->ce150PaperOutput()) out.push_back(byPeripheral(*e, *paper));
    if (const PlugEntry* e = at(sharp::kInterface)) {
        out.push_back(byPeripheral(*e, m_ce158Port.port()));
        out.push_back(byPeripheral(*e, k.printerPort));
    }
}
