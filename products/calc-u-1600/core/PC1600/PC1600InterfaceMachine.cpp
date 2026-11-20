#include "PC1600/PC1600InterfaceMachine.hpp"

#include <filesystem>

#include "../Debug/Inspect/PC1600Inspector.hpp"
#include "../Machine/SharpCatalogue.hpp"
#include "../Machine/SharpDisplayOutput.hpp"
#include "../Machine/SharpLoaderMemory.hpp"
#include "PC1600TypedInput.hpp"
#include "files/Files.hpp"
#include "peripherals/RealTimeClock.hpp"
#include "typing/TypingSupport.hpp"

using namespace calcu::machine;

namespace {


} // namespace

struct PC1600InterfaceMachine::Kinds {
    explicit Kinds(PC1600Machine& m)
        : memory(m), inspector(m), support(m), trace(m), tape(m), cassette(m.tapeDeck()), clock(m), floppy(m),
          drive(driveOf(m)), printerPort(m), hostDrive(m), call(m), program(m) {}

    static sharp::FloppyPlace::Drive driveOf(PC1600Machine& m) {
        sharp::FloppyPlace::Drive d;
        d.attached = [&m] { return m.ce1600fAttached(); };
        d.hasDisk = [&m] { return m.ce1600fHasDisk(); };
        d.motorOn = [&m] { return m.ce1600fMotorOn(); };
        d.load = [&m](const std::uint8_t* p, std::size_t n) { return m.ce1600fLoadImage(p, n); };
        d.eject = [&m] { m.ce1600fEject(); };
        d.image = [&m] { return m.ce1600fDiskImage(); };
        d.revision = [&m] { return m.ce1600fRevision(); };
        d.side = [&m] { return m.ce1600fSide(); };
        d.setSide = [&m](int s) { m.ce1600fSetSide(s); };
        return d;
    }

    sharp::PC1600LoaderMemory memory;
    struct Inspector final : sharp::InspectorViews {
        explicit Inspector(PC1600Machine& m) : m(m) {}
        std::vector<std::string> view(inspect::View v) const override { return inspect::pc1600View(m, v); }
        std::vector<inspect::MenuEntry> menu(bool dumps) const override { return inspect::pc1600Menu(m, dumps); }
        std::vector<std::uint8_t> slotImage(int slot, int* bankCount) const override {
            const ExpansionCard* card = slot == 1 || slot == 2 ? m.memory().slotCard(slot) : nullptr;
            *bankCount = card ? card->debugBankCount() : -1;
            return card ? card->debugImage() : std::vector<std::uint8_t>{};
        }
        PC1600Machine& m;
    } inspector;
    // Typing support, prompt readiness and the SHIFT and KBII latches.
    struct Support final : calcu::typing::TypingSupport {
        explicit Support(PC1600Machine& m) : m(m) {}
        std::optional<bool> promptReady() const override { return pc1600AtBasicPrompt(m); }
        std::vector<std::pair<std::string, bool>> latches() const override {
            const KeyLatches l = pc1600ReadLatches(m);
            return {{"shift", l.shift}, {"kbii", l.kbii}};
        }
        PC1600Machine& m;
    } support;
    struct Trace final : sharp::TraceRecords {
        explicit Trace(PC1600Machine& m) : m(m) {}
        bool begin() override { return m.beginTraceRecords(TRACE_FULL); }
        bool take(std::vector<std::uint8_t>& out, std::uint64_t budget) override { return m.takeTraceRecords(out, budget); }
        void endRecord() override { m.endTraceRecord(); }
        void end() override { m.endTraceRecords(); }
        PC1600Machine& m;
    } trace;
    struct Tape final : sharp::TapeDeckAccess {
        explicit Tape(PC1600Machine& m) : m(m) {}
        TapeDeck::Status status() const override { return m.tapeStatus(); }
        PC1600Machine& m;
    } tape;
    sharp::CassettePlace cassette;
    struct Clock final : calcu::peripherals::RealTimeClock {
        explicit Clock(PC1600Machine& m) : m(m) {}
        calcu::peripherals::ClockFields read() const override {
            // The sub-CPU keeps month, day, hour, minute and second; no year,
            // no weekday (PC1600Machine::seedClock).
            const PC1600SubCpu::DateTime t = m.memory().subCpu().dateTime();
            return {std::nullopt,
                    t.month,
                    PC1600SubCpu::unpackBcd(t.day),
                    std::nullopt,
                    PC1600SubCpu::unpackBcd(t.hour),
                    PC1600SubCpu::unpackBcd(t.minute),
                    PC1600SubCpu::unpackBcd(t.second)};
        }
        void set(const calcu::peripherals::ClockFields& f, int ms) override {
            const auto now = read();
            m.seedClock(f.year.value_or(2000), f.month.value_or(*now.month), f.day.value_or(*now.day),
                        f.hour.value_or(*now.hour), f.minute.value_or(*now.minute), f.second.value_or(*now.second), ms);
        }
        PC1600Machine& m;
    } clock;
    struct Floppy final : sharp::FloppyDriveAccess {
        explicit Floppy(PC1600Machine& m) : m(m) {}
        bool motorOn() const override { return m.ce1600fMotorOn(); }
        PC1600Machine& m;
    } floppy;
    sharp::FloppyPlace drive;
    struct PrinterPort final : sharp::Ce158PrinterPort {
        explicit PrinterPort(PC1600Machine& m) : m(m) {}
        std::vector<std::uint8_t> drain() override { return m.drainCE158ParallelOutput(); }
        PC1600Machine& m;
    } printerPort;
    struct HostDrive final : sharp::HostDriveFolder {
        explicit HostDrive(PC1600Machine& m) : m(m) {}
        std::string folder() const override { return calcu::files::fromPath(m.hostDriveDirectory()); }
        void setFolder(const std::string& f) override { m.setHostDriveDirectory(calcu::files::toPath(f)); }
        PC1600Machine& m;
    } hostDrive;
    // A routine call: CALL on the Z-80, SJP on the LH5803.
    struct Call final : sharp::RoutineCall {
        explicit Call(PC1600Machine& m) : m(m) {}
        bool call(const std::string& target, std::uint16_t address) override {
            if (target == "SC7852") m.sc7852().callRoutine(address);
            else if (target == "LH5803") m.lh5803().callRoutine(address);
            else return false;
            return true;
        }
        PC1600Machine& m;
    } call;
    struct Program final : sharp::ProgramState {
        explicit Program(PC1600Machine& m) : m(m) {}
        std::vector<std::string> busy() const override {
            if (m.ce1600fMotorOn()) return {"the CE-1600F's motor"};
            return {};
        }
        bool programEnded() const override { return m.programEnded(); }
        PC1600Machine& m;
    } program;
};

PC1600InterfaceMachine::PC1600InterfaceMachine(const MachineDescriptor& d, const VariantId& v)
    : SharpMachine(d, v), m_core(std::make_unique<PC1600Machine>()), m_kinds(std::make_unique<Kinds>(*m_core)),
      m_debugging(sharp::makeSharpDebugging(*m_core)) {
    // The RS-232C peer is attached when the port is first taken (capability()).
    m_core->setCE158SerialLink(&m_ce158Port);
}

PC1600InterfaceMachine::~PC1600InterfaceMachine() = default;

Capability* PC1600InterfaceMachine::capability(const CapabilityId& id) {
    Capability* c = SharpMachine::capability(id);
    // The TC8576F gets its peer when the serial port is first taken, as a
    // host link does right after a build, before the cold boot. Until then
    // the UART runs standalone, as the bare core of the golden runs did. The
    // peer's DCD is off until a program opens the serial port file.
    if (c == &m_rs232c.port() && !m_core->serialLink()) {
        m_rs232c.port().setPeerLines(calcu::peripherals::PeerLines{true, true, false, false});
        m_core->setSerialLink(&m_rs232c);
    }
    return c;
}

void PC1600InterfaceMachine::coreKey(const std::string& id, bool down) {
    if (down) m_core->pressKey(id);
    else m_core->releaseKey(id);
}

DisplayOutput PC1600InterfaceMachine::readDisplay() {
    const PC1600DisplaySnapshot snap = m_core->displaySnapshot();
    std::vector<std::uint8_t> dots(static_cast<std::size_t>(PC1600Display::kWidth) * PC1600Display::kHeight, 0);
    for (int row = 0; row < PC1600Display::kHeight; ++row)
        for (int col = 0; col < PC1600Display::kWidth; ++col)
            dots[static_cast<std::size_t>(row) * PC1600Display::kWidth + col] = snap.pixels[row][col] ? 1 : 0;
    // The display is on while CK0 runs and the system has power: the sub-CPU
    // cutting power leaves CK0 as it was.
    const bool on = snap.clockEnabled && !m_core->isPoweredOff();
    return sharp::displayOutput(descriptor(), on, PC1600Display::kWidth, PC1600Display::kHeight,
                                std::move(dots), snap.statusLine);
}

ExpansionCard* PC1600InterfaceMachine::moduleIn(const PlaceName& p) {
    if (p.value == sharp::kSlot1) return m_core->memory().slotCard(1);
    if (p.value == sharp::kSlot2) return m_core->memory().slotCard(2);
    return nullptr;
}

std::optional<std::string> PC1600InterfaceMachine::modelRefusal(const PeripheralRequest& req,
                                                                const std::optional<PeripheralId>& replacing) const {
    // The CE-1600P and the CE-158 cannot be used together: refused, rather
    // than the core removing the other one without a word.
    const char* other = req.peripheral.value == sharp::kCE1600P ? sharp::kCE158
                        : req.peripheral.value == sharp::kCE158 ? sharp::kCE1600P
                                                                 : nullptr;
    if (!other) return std::nullopt;
    for (const PlugEntry& e : plugged().byPlace)
        if (e.peripheral.value == other && !(replacing && *replacing == e.peripheral))
            return std::string("The CE-1600P and the CE-158 cannot be used together.");
    return std::nullopt;
}

void PC1600InterfaceMachine::corePlug(const PeripheralRequest& req, const PeripheralDescriptor& d) {
    const std::string& place = req.place.steps.at(0).value;
    if (place == sharp::kSlot1 || place == sharp::kSlot2) {
        auto card = sharp::moduleCardOf(req, d);
        if (!card) return;
        if (place == sharp::kSlot1) m_core->attachSlot1Card(std::move(card));
        else m_core->attachSlot2Card(std::move(card));
        return;
    }
    if (const auto* b = dynamic_cast<const sharp::BusRomRequest*>(req.content.get())) {
        if (b->bank >= 0)
            m_core->attachBusRom(std::make_unique<PC1600BusRomCard>(b->bytes, static_cast<std::uint8_t>(b->bank), b->name));
        else
            m_core->attachBusRom(std::make_unique<BusRomCard>(b->bytes, b->address, b->me1, b->pv, b->pu, b->name));
        return;
    }
    const std::vector<std::uint8_t>* rom = sharp::romImageOf(req, d, 0);
    if (!rom) return;
    const std::string& id = req.peripheral.value;
    if (id == sharp::kCE150) {
        m_core->attachCE150(rom->data(), rom->size());
    } else if (id == sharp::kCE1600P) {
        if (const std::vector<std::uint8_t>* rom2 = sharp::romImageOf(req, d, 1))
            m_core->attachCE1600P(rom->data(), rom->size(), rom2->data(), rom2->size());
    } else if (id == sharp::kCE158) {
        m_core->attachCE158(rom->data(), rom->size());
    } else if (id == sharp::kHostDrive) {
        const auto* folder = dynamic_cast<const sharp::HostDriveRequest*>(req.content.get());
        m_core->attachHostDrive(rom->data(), rom->size(), calcu::files::toPath(folder ? folder->folder : std::string()));
    }
}

void PC1600InterfaceMachine::coreUnplug(const PlaceName& p, const PeripheralId& id) {
    if (p.value == sharp::kSlot1) m_core->detachSlot1();
    else if (p.value == sharp::kSlot2) m_core->detachSlot2();
    else if (id.value == sharp::kCE150) { m_core->tapeEject(); m_core->detachCE150(); }   // the cassette goes with it
    else if (id.value == sharp::kCE1600P) { m_core->tapeEject(); m_core->detachCE1600P(); }
    else if (id.value == sharp::kCE158) m_core->detachCE158();
    else if (id.value == sharp::kHostDrive) m_core->detachHostDrive();
    else if (id.value == sharp::kBusRom) m_core->detachBusRoms();
}

void PC1600InterfaceMachine::offer(std::vector<Offered>& out, const std::vector<PlugEntry>&) {
    Kinds& k = *m_kinds;
    out.push_back(byComponent("SC7852", k.memory));
    out.push_back(byComponent("SC7852", k.inspector));
    out.push_back(byComponent("SC7852", k.support));
    out.push_back(byComponent("SC7852", k.trace));
    out.push_back(byComponent("SC7852", k.call));
    out.push_back(byComponent("SC7852", k.program));
    out.push_back(byComponent("SC7852", stateCapability()));
    out.push_back(byComponent(sharp::kRtcComponent, k.clock));
    out.push_back(byComponent(sharp::kRs232cComponent, m_rs232c.port()));
    if (const PlugEntry* e = at(sharp::kPrinter)) {
        // The plotter's paper output, once its card is on the bus.
        if (e->peripheral.value == sharp::kCE150) {
            if (calcu::paper::PaperOutput* paper = m_core->ce150PaperOutput()) out.push_back(byPeripheral(*e, *paper));
        } else if (e->peripheral.value == sharp::kCE1600P) {
            if (calcu::paper::PaperOutput* paper = m_core->ce1600pPaperOutput()) out.push_back(byPeripheral(*e, *paper));
            out.push_back(byPeripheral(*e, k.floppy));
            out.push_back(byPeripheral(*e, k.drive, "floppy"));   // the CE-1600F's media place
        }
        // The cassette of either interface.
        if (e->peripheral.value == sharp::kCE150 || e->peripheral.value == sharp::kCE1600P) {
            out.push_back(byPeripheral(*e, k.cassette, "cassette-recorder"));
            out.push_back(byPeripheral(*e, k.tape));
        }
    }
    if (const PlugEntry* e = at(sharp::kInterface)) {
        out.push_back(byPeripheral(*e, m_ce158Port.port()));
        out.push_back(byPeripheral(*e, k.printerPort));
    }
    if (const PlugEntry* e = at(sharp::kHostDrivePlace)) out.push_back(byPeripheral(*e, k.hostDrive));
}

// ---- The machine state ----

template <class Io>
void PC1600InterfaceMachine::interfaceStateIo(Io& io) {
    io.begin("LINE");
    m_ce158Port.stateIo(io);
    m_rs232c.stateIo(io);
    io.end();
    io.begin("CASS");
    m_kinds->cassette.stateIo(io);
    io.end();
}

void PC1600InterfaceMachine::coreSaveState(sharp::StateWriter& w) const {
    m_core->saveState(w);
    const_cast<PC1600InterfaceMachine*>(this)->interfaceStateIo(w);   // only reads
}

void PC1600InterfaceMachine::coreRestoreState(sharp::StateReader& r) {
    m_core->restoreState(r);
    interfaceStateIo(r);
}
