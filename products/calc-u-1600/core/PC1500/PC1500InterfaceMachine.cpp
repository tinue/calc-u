#include "PC1500/PC1500InterfaceMachine.hpp"

#include "../Connector/SoftwareDefinedCard.hpp"
#include "../Debug/Inspect/PC1500Inspector.hpp"
#include "../Machine/SharpCatalogue.hpp"
#include "../Machine/SharpDisplayOutput.hpp"
#include "../Machine/SharpLoaderMemory.hpp"
#include "PC1500TypedInput.hpp"
#include "typing/TypingSupport.hpp"

using namespace calcu::machine;

namespace {

PC1500Variant modelOf(const MachineDescriptor& d) {
    return d.model.value == sharp::kPC1500A ? PC1500Variant::PC1500A : PC1500Variant::PC1500;
}

} // namespace

// The capability objects, each over the core.
struct PC1500InterfaceMachine::Kinds {
    explicit Kinds(PC1500Machine& m) : memory(m), inspector(m), support(m), trace(m), tape(m), cassette(m.tapeDeck()), clock(m), printerPort(m), call(m), program(m) {}

    sharp::PC1500LoaderMemory memory;
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
    // Typing support, prompt readiness (no latches, no sample count).
    struct Support final : calcu::typing::TypingSupport {
        explicit Support(PC1500Machine& m) : m(m) {}
        std::optional<bool> promptReady() const override { return pc1500AtBasicPrompt(m); }
        PC1500Machine& m;
    } support;
    struct Trace final : sharp::TraceRecords {
        explicit Trace(PC1500Machine& m) : m(m) {}
        bool begin() override { return m.beginTraceRecords(TRACE_FULL); }
        bool take(std::vector<std::uint8_t>& out, std::uint64_t budget) override { return m.takeTraceRecords(out, budget); }
        void endRecord() override { m.endTraceRecord(); }
        void end() override { m.endTraceRecords(); }
        PC1500Machine& m;
    } trace;
    struct Tape final : sharp::TapeDeckAccess {
        explicit Tape(PC1500Machine& m) : m(m) {}
        TapeDeck::Status status() const override { return m.tapeStatus(); }
        PC1500Machine& m;
    } tape;
    sharp::CassettePlace cassette;
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
    // A routine call: SJP on the LH5801.
    struct Call final : sharp::RoutineCall {
        explicit Call(PC1500Machine& m) : m(m) {}
        bool call(const std::string& target, std::uint16_t address) override {
            if (target != "LH5801") return false;
            m.cpu().callRoutine(address);
            return true;
        }
        PC1500Machine& m;
    } call;
    struct Program final : sharp::ProgramState {
        explicit Program(PC1500Machine& m) : m(m) {}
        std::vector<std::string> busy() const override {
            if (m.toneSounding()) return {"the buzzer"};
            return {};
        }
        bool programEnded() const override { return m.programEnded(); }
        // The tape, or the CE-158's port with nothing on its way.
        bool awaitsMedium() const override {
            return m.tapeAwaited() || (m.ce158ReceiveWaitSeen() && !port->port().incoming());
        }
        PC1500Machine& m;
        SerialPortCapability* port = nullptr;   // the interface machine's CE-158 peer
    } program;
};

PC1500InterfaceMachine::PC1500InterfaceMachine(const MachineDescriptor& d, const VariantId& v)
    : SharpMachine(d, v), m_core(std::make_unique<PC1500Machine>(modelOf(d))), m_kinds(std::make_unique<Kinds>(*m_core)),
      m_debugging(sharp::makeSharpDebugging(*m_core)) {
    m_core->setCE158SerialLink(&m_ce158Port);
    m_kinds->program.port = &m_ce158Port;
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
    if (const auto* b = dynamic_cast<const sharp::BusRomRequest*>(req.content.get())) {
        m_core->attachBusRom(std::make_unique<BusRomCard>(b->bytes, b->address, b->me1, b->pv, b->pu, b->name));
        return;
    }
    const std::vector<std::uint8_t>* rom = sharp::romImageOf(req, d, 0);
    if (!rom) return;
    if (req.peripheral.value == sharp::kCE150) m_core->attachCE150(rom->data(), rom->size());
    else if (req.peripheral.value == sharp::kCE158) m_core->attachCE158(rom->data(), rom->size());
}

void PC1500InterfaceMachine::coreUnplug(const PlaceName& p, const PeripheralId& id) {
    if (p.value == sharp::kSlot1) m_core->expansionConnector().detach();
    else if (id.value == sharp::kCE150) { m_core->tapeEject(); m_core->detachCE150(); }   // the cassette goes with it
    else if (id.value == sharp::kCE158) m_core->detachCE158();
    else if (id.value == sharp::kBusRom) m_core->detachBusRoms();
}

void PC1500InterfaceMachine::offer(std::vector<Offered>& out, const std::vector<PlugEntry>&) {
    Kinds& k = *m_kinds;
    out.push_back(byComponent("LH5801", k.memory));
    out.push_back(byComponent("LH5801", k.inspector));
    out.push_back(byComponent("LH5801", k.support));
    out.push_back(byComponent("LH5801", k.trace));
    out.push_back(byComponent("LH5801", k.call));
    out.push_back(byComponent("LH5801", k.program));
    out.push_back(byComponent("LH5801", stateCapability()));
    out.push_back(byComponent(sharp::kRtcComponent, k.clock));
    if (const PlugEntry* e = at(sharp::kPrinter); e && e->peripheral.value == sharp::kCE150) {
        // The plotter's paper output, once its card is on the bus.
        if (calcu::paper::PaperOutput* paper = m_core->ce150PaperOutput()) out.push_back(byPeripheral(*e, *paper));
        out.push_back(byPeripheral(*e, k.cassette, "cassette-recorder"));   // the CE-150's cassette
        out.push_back(byPeripheral(*e, k.tape));
    }
    if (const PlugEntry* e = at(sharp::kInterface)) {
        out.push_back(byPeripheral(*e, m_ce158Port.port()));
        out.push_back(byPeripheral(*e, k.printerPort));
    }
}

// ---- The machine state ----

template <class Io>
void PC1500InterfaceMachine::interfaceStateIo(Io& io) {
    io.begin("LINE");
    m_ce158Port.stateIo(io);
    io.end();
    io.begin("CASS");
    m_kinds->cassette.stateIo(io);
    io.end();
}

void PC1500InterfaceMachine::coreSaveState(sharp::StateWriter& w) const {
    m_core->saveState(w);
    const_cast<PC1500InterfaceMachine*>(this)->interfaceStateIo(w);   // only reads
}

void PC1500InterfaceMachine::coreRestoreState(sharp::StateReader& r) {
    m_core->restoreState(r);
    interfaceStateIo(r);
}
