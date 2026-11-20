#include "PC1600Machine.hpp"

#include <cstring>
#include <filesystem>
#include <vector>

#include "../Connector/CardState.hpp"
#include "../Debug/Inspect/PC1600Inspector.hpp"
#include "../Machine/SharpCatalogue.hpp"
#include "../Machine/SharpDisplayOutput.hpp"
#include "../Machine/SharpLoaderMemory.hpp"
#include "../Machine/SharpProductKinds.hpp"
#include "PC1600TypedInput.hpp"
#include "files/Files.hpp"
#include "peripherals/RealTimeClock.hpp"
#include "typing/TypingSupport.hpp"

using namespace calcu::machine;

struct PC1600Machine::Kinds {
    explicit Kinds(PC1600Machine& m)
        : memory(m), inspector(m), support(m), trace(m), tape(m), cassette(m.tapeDeck()), clock(m), floppy(m),
          drive(driveOf(m)), printerPort(m), hostDrive(m), call(m) {}

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
};

PC1600Machine::PC1600Machine(const MachineDescriptor& d, const VariantId& v)
    : SharpMachine(d, v), m_z80Mem(m_bank), m_sc7852(m_z80Mem), m_lh5803Mem(m_z80Mem), m_lh5803(m_lh5803Mem),
      m_kinds(std::make_unique<Kinds>(*this)), m_debugging(sharp::makeSharpDebugging(*this)) {
    m_z80Mem.setBusArbiter(&m_arbiter);
    m_z80Mem.setCPU(&m_sc7852);
    m_lh5803Mem.setBusArbiter(&m_arbiter);
    m_lh5803.setCpuIdTag(CPU_ID_LH5803);
    // The RS-232C peer is attached when the port is first taken (capability()).
    setCE158SerialLink(&m_ce158Port);
}

PC1600Machine::PC1600Machine()
    : PC1600Machine(*sharp::sharpCatalogue().model(ModelId{sharp::kPC1600}),
                    *sharp::sharpCatalogue().model(ModelId{sharp::kPC1600})->defaultVariant) {}

PC1600Machine::~PC1600Machine() = default;

Capability* PC1600Machine::capability(const CapabilityId& id) {
    Capability* c = SharpMachine::capability(id);
    // The TC8576F gets its peer when the serial port is first taken, as a
    // host link does right after a build, before the cold boot. Until then
    // the UART runs standalone, as the bare core of the golden runs did. The
    // peer's DCD is off until a program opens the serial port file.
    if (c == &m_rs232c.port() && !serialLink()) {
        m_rs232c.port().setPeerLines(calcu::peripherals::PeerLines{true, true, false, false});
        setSerialLink(&m_rs232c);
    }
    return c;
}

void PC1600Machine::coreKey(const std::string& id, bool down) {
    if (down) pressKey(id);
    else releaseKey(id);
}

DisplayOutput PC1600Machine::readDisplay() {
    const PC1600DisplaySnapshot snap = displaySnapshot();
    std::vector<std::uint8_t> dots(static_cast<std::size_t>(PC1600Display::kWidth) * PC1600Display::kHeight, 0);
    for (int row = 0; row < PC1600Display::kHeight; ++row)
        for (int col = 0; col < PC1600Display::kWidth; ++col)
            dots[static_cast<std::size_t>(row) * PC1600Display::kWidth + col] = snap.pixels[row][col] ? 1 : 0;
    // The display is on while CK0 runs and the system has power: the sub-CPU
    // cutting power leaves CK0 as it was.
    const bool on = snap.clockEnabled && !isPoweredOff();
    return sharp::displayOutput(descriptor(), on, PC1600Display::kWidth, PC1600Display::kHeight,
                                std::move(dots), snap.statusLine);
}

ExpansionCard* PC1600Machine::moduleIn(const PlaceName& p) {
    if (p.value == sharp::kSlot1) return m_z80Mem.slotCard(1);
    if (p.value == sharp::kSlot2) return m_z80Mem.slotCard(2);
    return nullptr;
}

std::optional<std::string> PC1600Machine::modelRefusal(const PeripheralRequest& req,
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

void PC1600Machine::corePlug(const PeripheralRequest& req, const PeripheralDescriptor& d) {
    const std::string& place = req.place.steps.at(0).value;
    if (place == sharp::kSlot1 || place == sharp::kSlot2) {
        auto card = sharp::moduleCardOf(req, d);
        if (!card) return;
        if (place == sharp::kSlot1) attachSlot1Card(std::move(card));
        else attachSlot2Card(std::move(card));
        return;
    }
    if (const auto* b = dynamic_cast<const sharp::BusRomRequest*>(req.content.get())) {
        if (b->bank >= 0)
            attachBusRom(std::make_unique<PC1600BusRomCard>(b->bytes, static_cast<std::uint8_t>(b->bank), b->name));
        else
            attachBusRom(std::make_unique<BusRomCard>(b->bytes, b->address, b->me1, b->pv, b->pu, b->name));
        return;
    }
    const std::vector<std::uint8_t>* rom = sharp::romImageOf(req, d, 0);
    if (!rom) return;
    const std::string& id = req.peripheral.value;
    if (id == sharp::kCE150) {
        attachCE150(rom->data(), rom->size());
    } else if (id == sharp::kCE1600P) {
        if (const std::vector<std::uint8_t>* rom2 = sharp::romImageOf(req, d, 1))
            attachCE1600P(rom->data(), rom->size(), rom2->data(), rom2->size());
    } else if (id == sharp::kCE158) {
        attachCE158(rom->data(), rom->size());
    } else if (id == sharp::kHostDrive) {
        const auto* folder = dynamic_cast<const sharp::HostDriveRequest*>(req.content.get());
        attachHostDrive(rom->data(), rom->size(), calcu::files::toPath(folder ? folder->folder : std::string()));
    }
}

void PC1600Machine::coreUnplug(const PlaceName& p, const PeripheralId& id) {
    if (p.value == sharp::kSlot1) detachSlot1();
    else if (p.value == sharp::kSlot2) detachSlot2();
    else if (id.value == sharp::kCE150) detachCE150();
    else if (id.value == sharp::kCE1600P) { tapeEject(); detachCE1600P(); }   // the cassette goes with it
    else if (id.value == sharp::kCE158) detachCE158();
    else if (id.value == sharp::kHostDrive) detachHostDrive();
    else if (id.value == sharp::kBusRom) detachBusRoms();
}

void PC1600Machine::offer(std::vector<Offered>& out, const std::vector<PlugEntry>&) {
    Kinds& k = *m_kinds;
    out.push_back(byComponent("SC7852", k.memory));
    out.push_back(byComponent("SC7852", k.inspector));
    out.push_back(byComponent("SC7852", k.support));
    out.push_back(byComponent("SC7852", k.trace));
    out.push_back(byComponent("SC7852", k.call));
    out.push_back(byComponent("SC7852", stateCapability()));
    out.push_back(byComponent(sharp::kRtcComponent, k.clock));
    out.push_back(byComponent(sharp::kRs232cComponent, m_rs232c.port()));
    if (const PlugEntry* e = at(sharp::kPrinter)) {
        // The plotter's paper output, once its card is on the bus.
        if (e->peripheral.value == sharp::kCE150) {
            if (calcu::paper::PaperOutput* paper = ce150PaperOutput()) out.push_back(byPeripheral(*e, *paper));
        } else if (e->peripheral.value == sharp::kCE1600P) {
            if (calcu::paper::PaperOutput* paper = ce1600pPaperOutput()) out.push_back(byPeripheral(*e, *paper));
            out.push_back(byPeripheral(*e, k.floppy));
            out.push_back(byPeripheral(*e, k.drive, "floppy"));   // the CE-1600F's media place
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


void PC1600Machine::resetLocked() {
    m_readyFrames = 0;  // key readiness is counted again from here (keyReady())
    m_keyReadInFrame = false;
    m_mediumWaits = m_lastMediumWaits = MediumWaits{};
    m_bank.reset();
    m_z80Mem.reset();
    m_z80Mem.keyboard().releaseAll();
    m_sc7852.reset();
    m_lh5803.reset();
    m_arbiter.reset();
    m_debugStop.clear();
    m_timer64Accum = 0;
    m_timer64State = false;
    m_timer64EdgeCount = 0;
    // m_rtcAccum is the sub-CPU's divider, which runs on the always-on rail:
    // no reset or power cycle touches it (seedClock() sets its phase).
    m_z80Mem.setTimer64Bit(false);
    m_lh5803Mem.updatePUPV(false, false); // match the just-reset LH5803 CPU
    if (m_ce150Card) m_ce150Card->reset(); // re-anchor, keep it attached (like the CE-1600P)
    m_ce158.reset();
    // Any reset happens with power present.
    m_z80Mem.subCpu().markSystemOn();
}

void PC1600Machine::powerOnLocked() {
    // The sub-CPU switches VCC on and holds the SC-7852 in reset for 30 ms
    // (Service Manual §6-3). RAM and the LCD's RAM kept their contents on
    // VGG; the boot ROM finds its FA08H signature and resumes, or runs the
    // WAKE$ command string, depending on the cause the sub-CPU reports
    // (takePowerOnCause() has set it).
    resetLocked();
}

void PC1600Machine::coreReset() {
    resetLocked();
    // Tell the boot ROM this was the lighter reset level: it keeps the
    // calendar clock, the internal-RAM work area and every setting.
    m_z80Mem.subCpu().setResetCauseSimple();
}

void PC1600Machine::coreColdBoot() {
    // Two things make this the ALL RESET: the wiped RAM, and the reset
    // cause the sub-CPU reports. The ROM then runs full cold init --
    // clock back to 1 Jan 00:00:00 (the caller re-seeds it), work area
    // zeroed, settings to default.
    m_z80Mem.clearInternalRam();
    resetLocked();
    m_z80Mem.subCpu().aclReset(); // ALL RESET is the sub-CPU's ACL pin too
    m_z80Mem.subCpu().setResetCauseAllReset();
}

void PC1600Machine::seedClock(int year, int month, int day, int hour, int minute, int second, int millisecond) {
    PC1600SubCpu::DateTime dt;
    dt.month  = static_cast<uint8_t>(month); // plain 1-12, as the protocol carries it
    dt.day    = PC1600SubCpu::packBcd(day);
    dt.hour   = PC1600SubCpu::packBcd(hour);
    dt.minute = PC1600SubCpu::packBcd(minute);
    dt.second = PC1600SubCpu::packBcd(second);
    m_z80Mem.subCpu().setDateTime(dt);
    (void)year; // the sub-CPU keeps no year; a 29 Feb is passed as is (see PC1600SubCpu::carryIntoHour)
    // Boot re-inits the calendar to its cold-start default once; keep the
    // seeded time through that write.
    m_z80Mem.subCpu().armHostSeedGuard();
    // Align the next tick with the host's next whole second.
    m_rtcAccum = static_cast<int>(static_cast<int64_t>(kRtcPeriodTStates) * millisecond / 1000);
}

bool PC1600Machine::attachCE1600P(const uint8_t* rom1, size_t rom1Size,
                                   const uint8_t* rom2, size_t rom2Size) {
    if (rom1Size != CE1600PCard::kRomHalfSize || rom2Size != CE1600PCard::kRomHalfSize)
        return false;
    auto card = std::make_unique<CE1600PCard>();
    if (!card->loadRom(rom1, rom1Size, rom2, rom2Size)) return false;
    auto floppy = std::make_unique<CE1600FCard>();  // drive starts empty
    detachCE1600PLocked();
    detachCE150Locked(); // one plotter on the bus at a time
    detachCE158Locked(); // CE-158 + CE-1600P not supported yet (TODO.md, IO7N)
    card->connectRecorder(&m_tapeDeck);
    m_z80Mem.systemBus().attach(card.get());
    m_z80Mem.systemBus().attach(floppy.get());
    m_ce1600pCard = std::move(card);
    m_ce1600fCard = std::move(floppy);
    return true;
}

void PC1600Machine::detachCE1600P() {
    detachCE1600PLocked();
}

void PC1600Machine::detachCE1600PLocked() {
    if (m_ce1600fCard) {
        m_z80Mem.systemBus().detach(m_ce1600fCard.get());
        m_ce1600fCard.reset();
    }
    if (!m_ce1600pCard) return;
    m_z80Mem.systemBus().detach(m_ce1600pCard.get());
    m_ce1600pCard.reset();
}

bool PC1600Machine::attachHostDrive(const uint8_t* rom, size_t romSize, const std::filesystem::path& dir) {
    auto card = std::make_unique<PC1600HostDriveCard>();
    if (!card->loadRom(rom, romSize)) return false;
    card->drive().setDirectory(dir);
    detachHostDriveLocked();
    m_z80Mem.systemBus().attach(card.get());
    m_hostDriveCard = std::move(card);
    return true;
}

void PC1600Machine::attachBusRom(std::unique_ptr<SystemBusCard> card) {
    m_z80Mem.systemBus().attachFirst(card.get());
    m_busRoms.push_back(std::move(card));
}

void PC1600Machine::detachBusRoms() {
    for (auto& c : m_busRoms) m_z80Mem.systemBus().detach(c.get());
    m_busRoms.clear();
}

void PC1600Machine::detachHostDrive() {
    detachHostDriveLocked();
}

void PC1600Machine::detachHostDriveLocked() {
    if (!m_hostDriveCard) return;
    m_z80Mem.systemBus().detach(m_hostDriveCard.get());
    m_hostDriveCard.reset();
}

bool PC1600Machine::hostDriveAttached() const {
    return m_hostDriveCard != nullptr;
}

void PC1600Machine::setHostDriveDirectory(const std::filesystem::path& dir) {
    if (m_hostDriveCard) m_hostDriveCard->drive().setDirectory(dir);
}

std::filesystem::path PC1600Machine::hostDriveDirectory() const {
    return m_hostDriveCard ? m_hostDriveCard->drive().directory() : std::filesystem::path{};
}

sharp::PlotterPaperOutput* PC1600Machine::ce1600pPaperOutput() {
    return m_ce1600pCard ? &m_ce1600pCard->paperOutput() : nullptr;
}

// Reads and clears the plotter mechanism's event log, which step() fills on
// every pen change.
std::vector<std::string> PC1600Machine::drainCE1600PEvents() {
    if (!m_ce1600pCard) return {};
    return m_ce1600pCard->mechanism().drainEvents();
}

// ── CE-1600F floppy (union-attached with CE-1600P, above) ─────────────

std::vector<uint8_t> PC1600Machine::ce1600fDiskImage() const {
    if (!m_ce1600fCard) return {};
    return m_ce1600fCard->imageForSave();
}

uint64_t PC1600Machine::ce1600fRevision() const {
    return m_ce1600fCard ? m_ce1600fCard->revision() : 0;
}

void PC1600Machine::ce1600fEject() {
    if (m_ce1600fCard) m_ce1600fCard->ejectDisk();
}

bool PC1600Machine::ce1600fHasDisk() const {
    return m_ce1600fCard && m_ce1600fCard->hasDisk();
}

bool PC1600Machine::ce1600fLoadImage(const uint8_t* data, size_t size) {
    if (!m_ce1600fCard) return false;
    return m_ce1600fCard->loadImage(data, size);
}

int PC1600Machine::ce1600fSide() const {
    return m_ce1600fCard ? m_ce1600fCard->side() : 0;
}

void PC1600Machine::ce1600fSetSide(int side) {
    if (m_ce1600fCard) m_ce1600fCard->setSide(side);
}

bool PC1600Machine::ce1600fMotorOn() const {
    return m_ce1600fCard && m_ce1600fCard->motorOn();
}

// ── Cassette recorder ─────────────────────────────────────────────────

bool PC1600Machine::tapePlay(const std::string& path, std::string& error) {
    return m_tapeDeck.loadForPlay(path, error);
}

void PC1600Machine::tapeRecord(const std::string& path) {
    m_tapeDeck.armRecord(path);
}

bool PC1600Machine::tapeEject(std::string* error) {
    return m_tapeDeck.eject(error);
}

TapeDeck::Status PC1600Machine::tapeStatus() const {
    return m_tapeDeck.status();
}

std::string PC1600Machine::tapeTakeLastError() {
    return m_tapeDeck.takeLastError();
}

// ── CE-150 plotter (LH5803 side) ──────────────────────────────────────

bool PC1600Machine::attachCE150(const uint8_t* rom, size_t romSize) {
    if (romSize != Ce150Card::kRomSize) return false;
    auto card = std::make_unique<Ce150Card>();
    if (!card->loadRom(rom, romSize)) return false;
    detachCE150Locked();
    detachCE1600PLocked(); // one plotter on the bus at a time
    card->reset();
    m_z80Mem.systemBus().attach(card.get());
    m_ce150Card = std::move(card);
    return true;
}

void PC1600Machine::detachCE150() {
    detachCE150Locked();
}

void PC1600Machine::detachCE150Locked() {
    if (!m_ce150Card) return;
    m_z80Mem.systemBus().detach(m_ce150Card.get());
    m_ce150Card.reset();
}

// ── CE-158 interface (LH5803 side) ─────────────────────────────────────

bool PC1600Machine::attachCE158(const uint8_t* rom, size_t romSize) {
    auto card = m_ce158.build(rom, romSize, kTStateHz); // ticked with SC7852 T-states, see step()
    if (!card) return false;
    detachCE158Locked();
    detachCE1600PLocked(); // not usable together with the CE-1600P
    m_z80Mem.systemBus().attach(card.get());
    m_ce158.install(std::move(card));
    return true;
}

void PC1600Machine::detachCE158() {
    detachCE158Locked();
}

void PC1600Machine::detachCE158Locked() {
    if (!m_ce158.attached()) return;
    m_z80Mem.systemBus().detach(m_ce158.card());
    m_ce158.remove();
}

void PC1600Machine::setCE158SerialLink(SerialLink* link) {
    m_ce158.setSerialLink(link);
}

std::vector<uint8_t> PC1600Machine::drainCE158ParallelOutput() {
    return m_ce158.drainParallelOutput();
}

sharp::PlotterPaperOutput* PC1600Machine::ce150PaperOutput() {
    return m_ce150Card ? &m_ce150Card->paperOutput() : nullptr;
}

std::vector<std::string> PC1600Machine::drainCE150Events() {
    if (!m_ce150Card) return {};
    return m_ce150Card->mechanism().drainEvents();
}


int PC1600Machine::step() {
    uint64_t tstates = 0;
    return stepLocked(&tstates);
}

int PC1600Machine::stepLocked(uint64_t* tstates) {
    *tstates = 0; // a step parked on a breakpoint takes no time
    if (!m_z80Mem.subCpu().systemOn()) {
        // No CPU has power. The sub-CPU's clock, timers and the peripherals
        // with their own supply keep going; a power-on source ends it.
        advanceSharedClocks(kOffSliceTStates);
        *tstates = kOffSliceTStates;
        if (m_z80Mem.subCpu().takePowerOnCause()) powerOnLocked();
        return kOffSliceTStates;
    }
    // The sub-CPU cuts power once it has the OFF command (EAH) and PCTRL ->
    // Q0 confirms. The CPU side of that is not documented; the ROM's OFF
    // routine halts the LH-5803 right after sending EAH (rom1500 E553H),
    // so a halted bus owner stands for it.
    if (m_z80Mem.subCpu().powerOffCommanded() &&
        (m_arbiter.sc7852Owns() ? m_sc7852.halted() : m_lh5803.halted())) {
        m_z80Mem.subCpu().switchSystemOff();
        advanceSharedClocks(kOffSliceTStates);
        *tstates = kOffSliceTStates;
        return kOffSliceTStates;
    }
    if (m_arbiter.sc7852Owns()) {
        // KEYGETR (0169H) and KEYDIRECT (0175H, INKEY$): a program reads a key.
        const uint16_t pc = m_sc7852.pc();
        if (pc == 0x0169 || pc == 0x0175) m_keyReadInFrame = true;
        else if (pc >= 0x6149 && pc <= 0x6302) noteTapeWait(pc);
        else if (pc >= 0x9FF4 && pc <= 0xA060) noteReceiveWait(pc);
        int c = m_sc7852.step();
        // Parked on a breakpoint: nothing executed, so no time passes.
        if (c == 0 && m_sc7852.breakpointsEnabled() && m_sc7852.consumeBreakpointHit()) {
            m_debugStop.latch(::DebugStop::Breakpoint, 1);
            return 0;
        }
        if (m_z80Watches && m_z80Watches->hitPending()) m_debugStop.latch(::DebugStop::Watch, 1);
        // A halted SC7852::step() returns 0 (this core's convention for
        // "made no forward progress"), but real HALT still burns 4 T-states
        // per internal NOP cycle -- without crediting that, a CPU parked in
        // HALT would never accumulate enough T-states to see the timer's
        // falling edge and would wait forever for the very interrupt meant
        // to wake it: the boot reaches a genuine HALT waiting on this
        // interrupt, so the timer must keep advancing once it gets there.
        const int cost = (c > 0 ? c : SC7852::kHaltTickCycles);
        advanceTimer64(cost);
        advanceSharedClocks(cost);
        *tstates = static_cast<uint64_t>(cost);
        if (m_ce1600fCard) m_ce1600fCard->advance(static_cast<uint32_t>(cost));
        // The documented handoff is OUT (38H),A then HALT -- the write
        // sets the pending flag (PC1600Memory::writeIO), but the actual
        // switch only happens once the SC7852 has also reached HALT, so
        // the parked CPU is left in a stopped, resumable state (see
        // resumeFromHalt()) rather than switching mid-instruction-stream.
        if (m_arbiter.switchRequestedBySC7852() && m_sc7852.halted()) {
            m_arbiter.switchToLH5803();
        }
        maybeDrainTrace();
        return c;
    }
    // SC7852 LHNMIO (pin 92): the opcode fetch at 9400H with PU = PV = 1
    // -- inside the CE-158 high bank's display shift (93F9H-9439H) --
    // raises the LH5803's NMI. Its handler (C440H) runs a copy of that
    // routine, redraws the LCD once, acknowledges at port 36H and resumes
    // the CE-158 at 943AH. Raised before the fetch: the handler redoes
    // 9400H-9403H itself. The Service Manual says "94**H"; the CE-158 runs
    // on through 943AH-94xxH with the trap re-armed, so only the 9400H
    // fetch is decoded here (that and whether port 30H b0 gates the trap
    // are open). Ref/PC-1600/PC-1600-CPU-LH5803-Compat.md §6.1.
    if (m_lh5803.pc() == 0x9400 && m_lh5803.pu() && m_lh5803.pv() && !m_lh5803.halted() &&
        !m_z80Mem.lhNmiLatched()) {
        m_z80Mem.latchLhNmi();
        m_lh5803.requestNonMaskableInterrupt();
    }
    int c = m_lh5803.step();
    if (c == 0 && m_lh5803.breakpointsEnabled() && m_lh5803.consumeBreakpointHit()) {
        m_debugStop.latch(::DebugStop::Breakpoint, 2);
        return 0;
    }
    if (m_lh5803Watches && m_lh5803Watches->hitPending()) m_debugStop.latch(::DebugStop::Watch, 2);
    // Push the LH5803's post-instruction PU/PV so the next LH5803-side bus
    // access sees it (PV gates the CE-150 ROM window). Mirrors
    // PC1500Machine::step()'s updatePUPV for the LH5801.
    m_lh5803Mem.updatePUPV(m_lh5803.pu(), m_lh5803.pv());
    // The LH-5803 also drives the UART / sub-CPU handshake (the OFF-path
    // clock save, rom1500 E538), and the calendar clock and buzzer keep
    // running while it owns the bus -- see advanceSharedClocks(). A halted
    // step returns 0 and is charged LH5801::kHaltTickCycles.
    *tstates = toTStates(static_cast<uint64_t>(c > 0 ? c : LH5801::kHaltTickCycles), /*sc7852Owned=*/false);
    advanceTimer64(static_cast<int>(*tstates));
    advanceSharedClocks(static_cast<int>(*tstates));
    // LH5803->SC7852: the STA #(0A038H) store is the whole handoff, so the
    // switch happens immediately after this step(). It raises cause bit 3,
    // and that INT ends the SC7852's HALT through the ROM's own handler
    // (the park at P1-B3 5C0E-5C22 unmasks only bit 3, then EI;HALT). A
    // SC7852 that parked unable to take it (IFF1 clear or bit 3 masked --
    // hand-written code; a real machine would hang there) is resumed
    // directly instead, without latching the cause.
    if (m_arbiter.switchRequestedByLH5803()) {
        m_arbiter.switchToSC7852();
        if (m_sc7852.iff1() && (m_z80Mem.intMask() & 0x08)) {
            m_z80Mem.latchLh5803InterruptCause();
        } else {
            m_sc7852.resumeFromHalt();
        }
    }
    maybeDrainTrace();
    return c;
}

void PC1600Machine::advanceTimer64(int tstates) {
    // The sub-CPU's 64 Hz square wave runs whichever CPU owns the bus: the
    // LH5803 ROM counts it too (DELAY64, EEB2H/EEECH, polls PB5 at
    // #(F00FH) -- the CE-158 terminal's menu waits there).
    m_timer64Accum += tstates * kTimer64AccumScale;
    while (m_timer64Accum >= kTimer64HalfPeriodScaled) {
        m_timer64Accum -= kTimer64HalfPeriodScaled;
        m_timer64State = !m_timer64State;
        m_z80Mem.setTimer64Bit(m_timer64State); // PB5 raw level only, see its own comment
        // TRM pin table (INT4, pin 83): "an interrupt is sent to the
        // CPU on a falling edge." Each high->low transition latches
        // port 32H bit 4, separately from PB5's raw level (the real ISR
        // reads 32H right after waking and needs the bit still set --
        // see latchTimer64InterruptCause()). It latches whatever 35H
        // says (the ROM's ISR filters the 32H byte with 35H itself,
        // P1-B3 4102H/4112H); the mask only gates the INT level -- see
        // PC1600Memory::interruptLevel().
        if (!m_timer64State) m_z80Mem.latchTimer64InterruptCause();
        // The sub-CPU's 0.5 s tick comes from the same divider as this
        // 64 Hz signal, see kTimer64EdgesPerHalfSecond. It raises SRIRQ
        // bit 1, and INT6 when the mask enables it.
        if (++m_timer64EdgeCount == kTimer64EdgesPerHalfSecond) {
            m_timer64EdgeCount = 0;
            m_z80Mem.subCpu().halfSecondTick();
        }
    }
}

void PC1600Machine::advanceSharedClocks(int tstates) {
    // Every T-state step() charges passes here, whichever CPU ran or none.
    m_elapsedTStates += static_cast<uint64_t>(tstates);
    m_readyFrameAccum += tstates;
    if (m_readyFrameAccum >= kReadyFrameTStates) {
        m_readyFrameAccum -= kReadyFrameTStates;
        // A frame takes keys (docs/developer/Idle.md) when the LCD clock runs and
        // the ROM waits in KEYGET (KEYWK2 F07AH bit 2) or a program read a key
        // in it. The 1/64 s interrupt reads the key matrix whatever runs, so a
        // matrix read says nothing.
        const bool keyWait = (debugPeek(0xF07A) & 0x04) != 0;
        if ((keyWait || m_keyReadInFrame) && m_z80Mem.display().clockEnabled()) ++m_readyFrames;
        else m_readyFrames = 0;
        m_keyReadInFrame = false;
        m_lastMediumWaits = m_mediumWaits;
        m_mediumWaits = MediumWaits{};
    }
    // LU-57813P calendar clock: one tick per emulated second, fed from
    // both bus-ownership branches of step() -- see kRtcPeriodTStates. The
    // OFF-key/auto-power-off shutdown hands the bus to the LH5803 and can
    // leave it there for the machine's whole "powered off" span; the
    // clock sits on the always-powered VGG rail and must not lag by it.
    m_rtcAccum += tstates;
    m_z80Mem.advanceBuzzer(static_cast<uint32_t>(tstates));
    while (m_rtcAccum >= kRtcPeriodTStates) {
        m_rtcAccum -= kRtcPeriodTStates;
        m_z80Mem.subCpu().tickOneSecond();
    }
    // TC8576F UART + the sub-CPU parallel-port BUSY window: both are
    // paced in SC-7852 T-states, the domain the firmware's status polls
    // observe them in.
    m_z80Mem.uart().tick(tstates);
    m_z80Mem.subCpu().tickByTStates(tstates);
    m_z80Mem.display().tick(tstates);
    m_ce158.tick(static_cast<uint64_t>(tstates)); // the CE-158's own UART clock
    m_z80Mem.systemBus().advanceCassette(static_cast<uint32_t>(tstates)); // the cassette recorder
}

uint64_t PC1600Machine::runCycles(uint64_t maxCycles) {
    uint64_t consumed = 0;
    while (consumed < maxCycles) {
        // The T-states step() charged the shared clocks, whichever CPU ran
        // (or none, while powered off).
        uint64_t tstates = 0;
        {
            stepLocked(&tstates);
        }
        if (m_debugStop.pending() == ::DebugStop::Breakpoint) break; // parked on a breakpoint: no time passed
        consumed += tstates;
        if (m_debugStop.pending() == ::DebugStop::Watch) break; // the watched access's instruction has completed
    }
    return consumed;
}

void PC1600Machine::pressKey(const std::string& name) {
    PC1600Keyboard::Key key = PC1600Keyboard::keyFromName(name);
    if (key == PC1600Keyboard::Key::Unknown) return;
    m_z80Mem.keyboard().setKeyState(key, true);
}
void PC1600Machine::releaseKey(const std::string& name) {
    PC1600Keyboard::Key key = PC1600Keyboard::keyFromName(name);
    if (key == PC1600Keyboard::Key::Unknown) return;
    m_z80Mem.keyboard().setKeyState(key, false);
}
void PC1600Machine::setOnKeyPressed(bool pressed) {
    // The ON/BREAK line: the SC-7852 sees its debounced level on PB7 and
    // the latch at 1BH (BREAK), and the sub-CPU sees it on KH, where a
    // rising edge powers a switched-off system on (Service Manual §9-3).
    // step() does the power-on at its next call.
    const bool risingEdge = m_z80Mem.setOnKeyPressed(pressed);
    if (risingEdge) m_z80Mem.subCpu().onKeyPressed(); // ignored while the system is on
}

bool PC1600Machine::pokeMemory(uint16_t address, const uint8_t* data, size_t size) {
    // All-or-nothing: reject ROM / open bus up front, then write through
    // the host path (poke(): flash takes the byte directly instead of via
    // its command decoder). isWritable() can't see inside a card -- any
    // region that answers a read passes it, even one that drops the write
    // (mask ROM, read-only RAM) -- so a byte poke() reports as not stored
    // puts the bytes written so far back and fails the call.
    std::vector<uint8_t> old(size);
    for (size_t i = 0; i < size; i++) {
        uint16_t addr = static_cast<uint16_t>(address + i); // wraps at 0xFFFF, matching real Z-80 address arithmetic
        if (!m_z80Mem.isWritable(addr)) return false;
        old[i] = m_z80Mem.read(addr);
    }
    for (size_t i = 0; i < size; i++) {
        if (m_z80Mem.poke(static_cast<uint16_t>(address + i), data[i])) continue;
        for (size_t j = 0; j < i; j++) m_z80Mem.poke(static_cast<uint16_t>(address + j), old[j]);
        return false;
    }
    return true;
}

uint8_t PC1600Machine::debugPeek(uint16_t addr) {
    return m_z80Mem.peek(addr);
}

bool PC1600Machine::mode1() { return (debugPeek(0xF1BC) & 0x40) != 0; }

// docs/developer/Idle.md, "A medium awaited": the CE-1600P's tape loaders
// (bank 5 in page B) in an edge wait -- MODE 0 CMBITIN L6149-6152, MODE 1
// CM1500BIT 62CD-62D8 and 62F7-6302 -- and CRCVA's receive loop (bank 6 in
// page C), NEW 9FF4-A018, OLD A03C-A060.
void PC1600Machine::noteTapeWait(uint16_t pc) {
    if (!m_ce1600pCard || m_bank.pageBBank() != 5) return;
    if ((pc >= 0x6149 && pc <= 0x6152) || (pc >= 0x62CD && pc <= 0x62D8) || (pc >= 0x62F7 && pc <= 0x6302))
        m_mediumWaits.tape = true;
}

void PC1600Machine::noteReceiveWait(uint16_t pc) {
    if (m_bank.pageCBank() != 6) return;
    if (pc <= 0xA018) m_mediumWaits.receiveNew = true;
    if (pc >= 0xA03C && pc <= 0xA060) m_mediumWaits.receiveOld = true;
}

bool PC1600Machine::tapeAwaited() const {
    if (!(m_mediumWaits.tape || m_lastMediumWaits.tape)) return false;
    const TapeDeck::Status st = m_tapeDeck.status();
    return st.mode != TapeDeck::Mode::Play || !st.motor || m_tapeDeck.atEnd();
}

bool PC1600Machine::receiveWaitSeen(bool oldRom) const {
    return oldRom ? m_mediumWaits.receiveOld || m_lastMediumWaits.receiveOld
                  : m_mediumWaits.receiveNew || m_lastMediumWaits.receiveNew;
}

bool PC1600Machine::programEnded() const {
    const auto peek = [this](std::uint16_t a) { return m_z80Mem.peek(a); };
    const bool busyOff = (peek(0xF64E) & 0x01) == 0;              // BUSY, SYMB0 bit 0
    const bool direct = peek(0xF89C) == 0 && peek(0xF89D) == 0;   // CURLINE
    const bool noStop = (peek(0xF88A) & 0x60) == 0;               // INTFLG: no PRINT stop, no INPUT
    const bool keyWait = (peek(0xF07A) & 0x04) != 0;              // KEYWK2: KEYGET waits
    return busyOff && direct && noStop && keyWait;
}

// The tape, or COM1:/COM2: with nothing on its way; the variant Old has
// CRCVA's loop at other addresses.
bool PC1600Machine::awaitsMedium() const {
    const bool oldRom = variant() && variant()->value == "Old";
    return tapeAwaited() || (receiveWaitSeen(oldRom) && !m_rs232c.port().incoming());
}

std::vector<std::string> PC1600Machine::busyWith() const {
    if (ce1600fMotorOn()) return {"the CE-1600F's motor"};
    return {};
}

int PC1600Machine::programAreaTitle() { return debugPeek(0xF1D5); }

bool PC1600Machine::debugSc7852Owns() {
    return m_arbiter.sc7852Owns();
}

void PC1600Machine::setSerialLink(SerialLink* link) {
    m_z80Mem.uart().setSerialLink(link);
}

void PC1600Machine::debugCopyInternalRam(uint8_t* out) {
    m_z80Mem.debugCopyInternalRam(out);
}

bool PC1600Machine::debugWriteInternalRam(size_t off, const uint8_t* data, size_t n) {
    return m_z80Mem.debugWriteInternalRam(off, data, n);
}

bool PC1600Machine::debugWriteSlotBus(int bank, uint16_t addr, const uint8_t* data, size_t n) {
    return m_z80Mem.slotBusWrite(bank, addr, data, n);
}

bool PC1600Machine::debugSlotBusWritable(int bank, uint16_t addr, size_t n) {
    return m_z80Mem.slotBusWritable(bank, addr, n);
}

std::vector<uint8_t> PC1600Machine::debugSlotImage(int slot) {
    return slot == 1 ? m_z80Mem.slot1CardImage() : m_z80Mem.slot2CardImage();
}

PC1600Machine::DebugBankState PC1600Machine::debugBankState() {
    return inspectBankState();
}

PC1600Machine::DebugBankState PC1600Machine::inspectBankState() const {
    DebugBankState s;
    s.port31 = m_bank.readPort31();
    s.port28 = m_bank.slot2VerticalBank();
    s.port3c = m_bank.readPort3C();
    s.pageABank = m_bank.pageABank();
    s.pageBBank = m_bank.pageBBank();
    s.pageCBank = m_bank.pageCBank();
    s.pageDBank = m_bank.pageDBank();
    s.slot2MapMode = m_bank.slot2MapMode();
    s.slot1MapRemapped = m_bank.slot1MapActive(); // SLOT1MAP bit -- now modelled, see PC1600Memory::slot1MapTarget()
    s.hiddenBasicRom = m_bank.hiddenBasicRomSelected();
    s.slot1CardBank = m_z80Mem.slot1CardBank();
    s.slot2CardBank = m_z80Mem.slot2CardBank();
    s.slot1CardBankCount = m_z80Mem.slot1CardBankCount();
    s.slot2CardBankCount = m_z80Mem.slot2CardBankCount();

    // Live address-map resolution -- mirrors PC1600Memory::resolveConst()'s
    // branch set + the two slot windows + slot2MapTarget()'s/slot1MapTarget()'s
    // gates. A SLOTMAP redirect only counts as live when it is both armed
    // (Port 3CH mode/bit) and triggered (the page's bank field == 1) AND the
    // relevant card is present -- an empty slot lets the access fall through
    // to its ordinary meaning (read()'s own comment).
    using PT = PageTarget;
    const bool s1 = m_z80Mem.slot1Attached();
    const bool s2 = m_z80Mem.slot2Attached();
    const uint8_t mode = s.slot2MapMode;

    // Page A (0000-3FFF): always Bank 0 lower, unless SLOT2MAP mode 2 steals
    // bank 1 for Slot 2's high 16 KB.
    if (mode == 2 && s.pageABank == 1 && s2) {
        s.target[0] = PT::Slot2; s.slotmapRedirect[0] = true;
    } else {
        s.target[0] = PT::SystemRomLo;
    }

    // Page B (4000-7FFF): SLOT1MAP's beta mirror and SLOT2MAP mode 2's low
    // half both live here -- the only real collision, arbitrated by the
    // same PC1600Bank::resolveSlotCollision() the live read/write decode
    // uses (PC1600Memory::resolveSlotRemap()).
    bool slot1B = s.slot1MapRemapped && s.pageBBank == 1 && s1;
    bool slot2B = mode == 2 && s.pageBBank == 1 && s2;
    m_bank.resolveSlotCollision(&slot1B, &slot2B);
    if (slot1B || slot2B) {
        s.target[1] = slot1B ? PT::Slot1 : PT::Slot2;
        s.slotmapRedirect[1] = true;
    } else {
        switch (s.pageBBank) {
            case 0:  s.target[1] = PT::SystemRomHi; break;
            case 3:  s.target[1] = s.hiddenBasicRom ? PT::Bank3bRom : PT::Bank3Rom; break;
            case 1:
            case 2:  s.target[1] = PT::OpenBus; break;
            default: {
                // Banks 4-7 go out on the 60-pin bus (PC1600Memory::read()):
                // whichever card answers the page's first byte owns it.
                const SystemBusCard* card = m_z80Mem.systemBus().debugResponder(
                    PC1600BusDrive::z80MemPins(0x4000, /*forWrite=*/false, s.pageBBank));
                s.target[1] = card ? PT::BusCard : PT::OpenBus;
                if (card) s.busCard = card->moduleName();
                break;
            }
        }
    }

    // Page C (8000-BFFF).
    if (mode == 1 && s.pageCBank == 1 && s2) {
        s.target[2] = PT::Slot2; s.slotmapRedirect[2] = true;
    } else {
        switch (s.pageCBank) {
            case 0:
            case 1:  s.target[2] = s1 ? PT::Slot1 : PT::OpenBus; break;
            case 2:
            case 3:  s.target[2] = s2 ? PT::Slot2 : PT::OpenBus; break;
            case 6:  s.target[2] = PT::Bank6Rom; break;
            default: s.target[2] = PT::OpenBus; break; // banks 4/5/7
        }
    }

    // Page D (C000-FFFF): bank 0 internal RAM, bank 1 open bus (unmodelled).
    s.target[3] = (s.pageDBank == 0) ? PT::InternalRam : PT::OpenBus;

    return s;
}

bool PC1600Machine::beginTraceRecords(uint32_t flags) {
    if (m_traceFile) return false;
    m_traceFile = std::make_unique<PC1500TraceFile>();
    m_traceDrainCounter = 0;
    m_sc7852.setTraceFlags(flags);
    m_lh5803.setTraceFlags(flags);
    uint32_t staleLost = 0;
    m_sc7852.drainTraceEvents(m_z80TraceDrainBuf, kTraceDrainBufFrames, &staleLost);
    m_lh5803.drainTraceEvents(m_lhTraceDrainBuf, kTraceDrainBufFrames, &staleLost);
    return true;
}

void PC1600Machine::endTraceRecord() {
    if (!m_traceFile) return;
    pumpTraceFile();
    m_traceFile->finish();
    m_sc7852.setTraceFlags(TRACE_NONE);
    m_lh5803.setTraceFlags(TRACE_NONE);
}

void PC1600Machine::endTraceRecords() {
    m_traceFile.reset();
    m_sc7852.setTraceFlags(TRACE_NONE);
    m_lh5803.setTraceFlags(TRACE_NONE);
    m_traceDrainCounter = 0;
}

void PC1600Machine::pumpTraceFile() {
    // Each CPU's drainTraceEvents() takes that core's
    // own independent ring mutex -- no lock-ordering concern.
    uint32_t lost = 0;
    uint32_t n = m_sc7852.drainTraceEvents(m_z80TraceDrainBuf, kTraceDrainBufFrames, &lost);
    if (lost) m_traceFile->writeGap(lost);
    for (uint32_t i = 0; i < n; i++) {
        // SC7852::recordTraceFrame() leaves cpuId unset (the file's 0x04
        // record type already marks a Z80 frame); stamp it so the merged
        // stream is self-describing, matching the LH5803 frames the core
        // tags in its ctor.
        m_z80TraceDrainBuf[i].cpuId = CPU_ID_SC7852;
        m_traceFile->writeFrame(m_z80TraceDrainBuf[i]);
    }

    lost = 0;
    n = m_lh5803.drainTraceEvents(m_lhTraceDrainBuf, kTraceDrainBufFrames, &lost);
    if (lost) m_traceFile->writeGap(lost);
    for (uint32_t i = 0; i < n; i++) m_traceFile->writeFrame(m_lhTraceDrainBuf[i]);
}

PC1600DisplaySnapshot PC1600Machine::displaySnapshot() const {
    PC1600DisplaySnapshot snap;
    const PC1600Display& disp = m_z80Mem.display();
    snap.clockEnabled = disp.clockEnabled();
    for (int y = 0; y < PC1600Display::kHeight; y++) {
        for (int x = 0; x < PC1600Display::kWidth; x++) {
            snap.pixels[y][x] = disp.pixel(x, y);
        }
    }
    snap.statusLine = disp.statusLine();
    return snap;
}

// ── Machine state ────────────────────────────────────────

template <class Io>
void PC1600Machine::stateIo(Io& io) {
    io.begin("Z80C");
    m_sc7852.stateIo(io);
    io.end();
    io.begin("5803");
    m_lh5803.stateIo(io);
    m_lh5803Mem.stateIo(io);
    io.end();
    io.begin("BANK");
    m_bank.stateIo(io);
    io.end();
    io.begin("ARBI");
    m_arbiter.stateIo(io);
    io.end();
    m_z80Mem.stateIo(io);
    io.begin("TIME");
    io(m_elapsedTStates)(m_readyFrameAccum)(m_readyFrames);
    io(m_timer64Accum)(m_timer64State)(m_timer64EdgeCount)(m_rtcAccum);
    io.end();
    io.begin("TAPE");
    m_tapeDeck.stateIo(io);
    io.end();
    sharp::cardStateIo(io, "SLT1", m_z80Mem.slotCard(1));
    sharp::cardStateIo(io, "SLT2", m_z80Mem.slotCard(2));
    sharp::cardStateIo(io, "C16P", m_ce1600pCard.get());
    sharp::cardStateIo(io, "C16F", m_ce1600fCard.get());
    sharp::cardStateIo(io, "HOST", m_hostDriveCard.get());
    sharp::cardStateIo(io, "C150", m_ce150Card.get());
    sharp::cardStateIo(io, "C158", m_ce158.card());
    io.begin("LINE");
    m_ce158Port.stateIo(io);
    m_rs232c.stateIo(io);
    io.end();
    io.begin("CASS");
    m_kinds->cassette.stateIo(io);
    io.end();
}

void PC1600Machine::coreSaveState(sharp::StateWriter& w) const { const_cast<PC1600Machine*>(this)->stateIo(w); }   // only reads
void PC1600Machine::coreRestoreState(sharp::StateReader& r) { stateIo(r); }
