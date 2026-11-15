#include "PC1600/PC1600InterfaceMachine.hpp"

#include <filesystem>

#include "../Debug/Inspect/PC1600Inspector.hpp"
#include "../Machine/SharpCatalogue.hpp"
#include "../Machine/SharpDisplayOutput.hpp"
#include "peripherals/RealTimeClock.hpp"

using namespace calcu::machine;

namespace {

std::filesystem::path pathOf(const std::string& utf8) { return std::filesystem::path(std::u8string(utf8.begin(), utf8.end())); }
std::string utf8Of(const std::filesystem::path& p) {
    const std::u8string s = p.generic_u8string();
    return std::string(s.begin(), s.end());
}

} // namespace

struct PC1600InterfaceMachine::Kinds {
    explicit Kinds(PC1600Machine& m)
        : memory(m), inspector(m), trace(m), tape(m), clock(m), ce150Paper(m), ce1600pPaper(m), floppy(m), printerPort(m),
          hostDrive(m) {}

    struct Memory final : sharp::LoaderMemory {
        explicit Memory(PC1600Machine& m) : m(m) {}
        std::uint8_t peek(std::uint16_t a) const override { return m.memory().peek(a); }
        bool poke(std::uint16_t a, const std::uint8_t* data, std::size_t n) override { return m.pokeMemory(a, data, n); }
        std::vector<std::uint8_t> internalRam() const override {
            std::vector<std::uint8_t> ram(PC1600Machine::kInternalRamSize);
            m.debugCopyInternalRam(ram.data());
            return ram;
        }
        PC1600Machine& m;
    } memory;
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
    struct Trace final : sharp::CpuTrace {
        explicit Trace(PC1600Machine& m) : m(m) {}
        bool begin(std::FILE* f, std::uint32_t flags) override { return m.beginCpuTrace(f, flags); }
        void end() override { m.endCpuTrace(); }
        bool active() const override { return m.cpuTraceActive(); }
        std::uint64_t bytes() const override { return m.cpuTraceBytes(); }
        PC1600Machine& m;
    } trace;
    struct Tape final : sharp::TapeDeckAccess {
        explicit Tape(PC1600Machine& m) : m(m) {}
        bool play(const std::string& p, std::string& e) override { return m.tapePlay(p, e); }
        void record(const std::string& p) override { m.tapeRecord(p); }
        bool eject(std::string* e) override { return m.tapeEject(e); }
        TapeDeck::Status status() const override { return m.tapeStatus(); }
        std::string takeLastError() override { return m.tapeTakeLastError(); }
        PC1600Machine& m;
    } tape;
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
    struct Ce150Paper final : sharp::PlotterPaper {
        explicit Ce150Paper(PC1600Machine& m) : m(m) {}
        std::vector<AlpsPlotterMechanism::FlatPoint> points() const override { return m.ce150PlotPoints(); }
        std::uint64_t revision() const override { return m.ce150PlotRevision(); }
        std::vector<std::string> drainEvents() override { return m.drainCE150Events(); }
        void clear() override { m.clearCE150Paper(); }
        PC1600Machine& m;
    } ce150Paper;
    struct Ce1600pPaper final : sharp::PlotterPaper {
        explicit Ce1600pPaper(PC1600Machine& m) : m(m) {}
        std::vector<AlpsPlotterMechanism::FlatPoint> points() const override { return m.ce1600pPlotPoints(); }
        std::uint64_t revision() const override { return m.ce1600pPlotRevision(); }
        std::vector<std::string> drainEvents() override { return m.drainCE1600PEvents(); }
        void clear() override { m.clearCE1600PPaper(); }
        PC1600Machine& m;
    } ce1600pPaper;
    struct Floppy final : sharp::FloppyDriveAccess {
        explicit Floppy(PC1600Machine& m) : m(m) {}
        std::vector<std::uint8_t> diskImage() const override { return m.ce1600fDiskImage(); }
        std::uint64_t revision() const override { return m.ce1600fRevision(); }
        void eject() override { m.ce1600fEject(); }
        bool hasDisk() const override { return m.ce1600fHasDisk(); }
        bool loadImage(const std::uint8_t* d, std::size_t n) override { return m.ce1600fLoadImage(d, n); }
        int side() const override { return m.ce1600fSide(); }
        void setSide(int s) override { m.ce1600fSetSide(s); }
        bool motorOn() const override { return m.ce1600fMotorOn(); }
        PC1600Machine& m;
    } floppy;
    struct PrinterPort final : sharp::Ce158PrinterPort {
        explicit PrinterPort(PC1600Machine& m) : m(m) {}
        std::vector<std::uint8_t> drain() override { return m.drainCE158ParallelOutput(); }
        PC1600Machine& m;
    } printerPort;
    struct HostDrive final : sharp::HostDriveFolder {
        explicit HostDrive(PC1600Machine& m) : m(m) {}
        std::string folder() const override { return utf8Of(m.hostDriveDirectory()); }
        void setFolder(const std::string& f) override { m.setHostDriveDirectory(pathOf(f)); }
        PC1600Machine& m;
    } hostDrive;
};

PC1600InterfaceMachine::PC1600InterfaceMachine(const MachineDescriptor& d, const VariantId& v)
    : SharpMachine(d, v), m_core(std::make_unique<PC1600Machine>()), m_kinds(std::make_unique<Kinds>(*m_core)) {
    // The RS-232C peer is attached when the port is first taken (capability()).
    m_core->setCE158SerialLink(&m_ce158Port);
}

PC1600InterfaceMachine::~PC1600InterfaceMachine() = default;

Capability* PC1600InterfaceMachine::capability(const CapabilityId& id) {
    Capability* c = SharpMachine::capability(id);
    // The TC8576F gets its peer when the serial port is first taken, as a
    // host link does right after a build (today's session attached its PTY
    // there, before the cold boot). Until then the UART runs standalone, as
    // the bare core of the golden runs did. The peer's DCD is off until a
    // program opens the serial port file, as with today's PTY.
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
    return sharp::displayOutput(descriptor(), snap.clockEnabled, PC1600Display::kWidth, PC1600Display::kHeight,
                                std::move(dots), snap.statusLine);
}

ExpansionCard* PC1600InterfaceMachine::moduleIn(const PlaceName& p) {
    if (p.value == sharp::kSlot1) return m_core->memory().slotCard(1);
    if (p.value == sharp::kSlot2) return m_core->memory().slotCard(2);
    return nullptr;
}

std::optional<std::string> PC1600InterfaceMachine::modelRefusal(const PeripheralRequest& req,
                                                                const std::optional<PeripheralId>& replacing) const {
    // The CE-1600P and the CE-158 cannot be used together (LIB-2 AC-29,
    // D-6, D-7; today the core removed the other one without a word).
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
    const std::vector<std::uint8_t>* rom = sharp::romImageOf(req, d, 0);
    if (!rom) return;
    const std::string& id = req.peripheral.value;
    if (id == sharp::kCE150) {
        m_core->attachCE150(rom->data(), rom->size());
    } else if (id == sharp::kCE1600P) {
        if (const std::vector<std::uint8_t>* rom2 = sharp::romImageOf(req, d, 1))
            m_core->attachCE1600P(rom->data(), rom->size(), rom2->data(), rom2->size());
        if (req.variant) m_presetCe1600pVariant = *req.variant;
    } else if (id == sharp::kCE158) {
        m_core->attachCE158(rom->data(), rom->size());
    } else if (id == sharp::kHostDrive) {
        const auto* folder = dynamic_cast<const sharp::HostDriveRequest*>(req.content.get());
        m_core->attachHostDrive(rom->data(), rom->size(), pathOf(folder ? folder->folder : std::string()));
    }
}

void PC1600InterfaceMachine::coreUnplug(const PlaceName& p, const PeripheralId& id) {
    if (p.value == sharp::kSlot1) m_core->detachSlot1();
    else if (p.value == sharp::kSlot2) m_core->detachSlot2();
    else if (id.value == sharp::kCE150) m_core->detachCE150();
    else if (id.value == sharp::kCE1600P) m_core->detachCE1600P();
    else if (id.value == sharp::kCE158) m_core->detachCE158();
    else if (id.value == sharp::kHostDrive) m_core->detachHostDrive();
}

std::vector<PlugEntry> PC1600InterfaceMachine::coreHasPlugged() const {
    std::vector<PlugEntry> out;
    auto add = [&](const char* place, const std::string& id, std::optional<VariantId> v = std::nullopt) {
        out.push_back({PlacePath{{PlaceName{place}}}, PeripheralId{id}, std::move(v)});
    };
    if (const ExpansionCard* c = m_core->memory().slotCard(1)) add(sharp::kSlot1, c->moduleName());
    if (const ExpansionCard* c = m_core->memory().slotCard(2)) add(sharp::kSlot2, c->moduleName());
    if (m_core->ce150Attached()) add(sharp::kPrinter, sharp::kCE150);
    if (m_core->ce1600pAttached()) add(sharp::kPrinter, sharp::kCE1600P, m_presetCe1600pVariant);
    if (m_core->ce158Attached()) add(sharp::kInterface, sharp::kCE158);
    if (m_core->hostDriveAttached()) add(sharp::kHostDrivePlace, sharp::kHostDrive);
    return out;
}

void PC1600InterfaceMachine::offer(std::vector<Offered>& out, const std::vector<PlugEntry>&) {
    Kinds& k = *m_kinds;
    out.push_back(byComponent("SC7852", k.memory));
    out.push_back(byComponent("SC7852", k.inspector));
    out.push_back(byComponent("SC7852", k.trace));
    out.push_back(byComponent("SC7852", k.tape));
    out.push_back(byComponent(sharp::kRtcComponent, k.clock));
    out.push_back(byComponent(sharp::kRs232cComponent, m_rs232c.port()));
    if (const PlugEntry* e = at(sharp::kPrinter)) {
        if (e->peripheral.value == sharp::kCE150) {
            out.push_back(byPeripheral(*e, k.ce150Paper));
        } else if (e->peripheral.value == sharp::kCE1600P) {
            out.push_back(byPeripheral(*e, k.ce1600pPaper));
            out.push_back(byPeripheral(*e, k.floppy));
        }
    }
    if (const PlugEntry* e = at(sharp::kInterface)) {
        out.push_back(byPeripheral(*e, m_ce158Port.port()));
        out.push_back(byPeripheral(*e, k.printerPort));
    }
    if (const PlugEntry* e = at(sharp::kHostDrivePlace)) out.push_back(byPeripheral(*e, k.hostDrive));
}
