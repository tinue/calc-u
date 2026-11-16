#include "app/SharpAppPart.hpp"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <stdexcept>

#include "Connector/SoftwareDefinedCard.hpp"
#include "Display/DisplayOutputText.hpp"
#include "Machine/LegacyCalls.hpp"
#include "Machine/SharpBuilder.hpp"
#include "Machine/SharpCatalogue.hpp"
#include "Machine/SharpMachine.hpp"
#include "Machine/SharpRoms.hpp"
#include "app/FloppyBay.hpp"
#include "app/ModuleBay.hpp"
#include "app/SharpDebuggerSetup.hpp"
#include "app/SharpPrinters.hpp"
#include "app/TapeBay.hpp"
#include "app/Utf8Path.hpp"
#include "peripherals/Controls.hpp"
#include "peripherals/RealTimeClock.hpp"
#include "peripherals/SerialPort.hpp"
#include "sessions/Args.hpp"
#include "sessions/LibraryTopics.hpp"

namespace shared {

using namespace calcu::machine;
namespace sessions = calcu::sessions;
namespace startup = calcu::startup;

namespace {

std::optional<Model> modelOfId(const std::string& id) {
    if (id == sharp::kPC1500) return Model::PC1500;
    if (id == sharp::kPC1500A) return Model::PC1500A;
    if (id == sharp::kPC1600) return Model::PC1600;
    return std::nullopt;
}

const char* slotPlace(int slot) { return slot == 2 ? sharp::kSlot2 : sharp::kSlot1; }

PlacePath placeOf(const char* place) { return PlacePath{{PlaceName{place}}}; }

// Which serial port gets which serial port file (LIB-2 AC-33): the PC-1600's
// RS-232C at once, the CE-158's when it is first used.
std::optional<calcu::peripherals::HostLinkRule> serialFileFor(const CapabilityEntry& e) {
    if (const auto* c = std::get_if<ComponentOwner>(&e.owner); c && c->component.value == sharp::kRs232cComponent)
        return calcu::peripherals::HostLinkRule{"calcu1600-rs232c.serial", true};
    if (const auto* p = std::get_if<PeripheralOwner>(&e.owner); p && p->peripheral.value == sharp::kCE158)
        return calcu::peripherals::HostLinkRule{"calcu1600-ce158.serial", false};
    return std::nullopt;
}

std::optional<std::string> folderOrNone(const std::string& dir) {
    return dir.empty() ? std::nullopt : std::optional<std::string>(dir);
}

double steadySeconds() {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

VariantId ce1600pVariant(CE1600PRomVersion v) { return VariantId{v == CE1600PRomVersion::Old ? "Old" : "New"}; }

calcu::roms::LookupResult lookUp(const AppPaths& paths) {
    return sharp::lookUpRoms(calcu::roms::DirectoryBundle(toPath(paths.bundledResourcesDir())));
}

std::shared_ptr<const std::vector<std::uint8_t>> hostDriveRomOf(const AppPaths& paths) {
    auto rom = sharp::readHostDriveRom({paths.bundledResourcesDir()});
    return rom ? std::make_shared<const std::vector<std::uint8_t>>(std::move(*rom)) : nullptr;
}

using sessions::integerArg;
using sessions::textArg;

} // namespace

DisplayFrame displayFrameOf(const DisplayOutput& out) {
    DisplayFrame frame;
    const LcdBitmap bitmap = lcdBitmapOf(out);
    frame.cols = bitmap.cols;
    frame.rows = bitmap.rows;
    frame.pixels = bitmap.pixels;
    frame.poweredOn = bitmap.poweredOn;
    for (std::size_t i = 0; i < StatusLine::kCount; ++i) {
        bool on = false;
        for (const auto& ind : out.indicators)
            if (ind.name == kStatusSymbolNames[i]) on = ind.on;
        frame.statusSymbols.emplace_back(kStatusSymbolNames[i], on);
    }
    return frame;
}

SharpAppPart::SharpAppPart(Settings& settings, AppPaths& paths)
    : m_settings(settings),
      m_paths(paths),
      m_catalogue(sharp::sharpCatalogue()),
      m_found(lookUp(paths)),
      m_roms(m_found.set),
      m_romsMissing(m_found.missing),
      m_hostDriveRom(hostDriveRomOf(paths)),
      m_paper(m_catalogue, sharpPrinterPapers()),
      m_debugger(sharpDebuggerSetup(&paths, &settings, /*hasTraceFolder=*/true)),
      m_peripherals(serialFileFor, folderOrNone(paths.serialLinkDir())),
      m_machines(m_catalogue, m_hostDriveRom),
      m_rules(SharpKeywordSetup{[this] { return hostFolders(); }}),
      m_bundle(toPath(paths.bundledExamplesDir())),
      m_presets(calcu::presets::PresetsSetup{&m_catalogue, &m_rules, nullptr, &m_typingPart, &m_bundle,
                                             paths.homeDir(),
                                             [&paths] { return paths.places().screenshotFolder(); }}),
      m_startup(calcu::startup::StartupSetup{m_machines, m_roms, settings.file(), &m_presets,
                                             m_roms ? std::string() : calcu::roms::describe(m_romsMissing)}) {
    // The module definitions: the bundled ones and the ROM modules of the set
    // (the CE-502B; LIB-12 AC-14).
    sharp::addModuleDefinitions(m_catalogue, paths.bundledResourcesDir());
    if (m_roms) sharp::addModuleDefinitions(m_catalogue, *m_roms);
    m_presets.setControl(&m_startup.control());
    m_paper.setControl(&m_startup.control());   // the origin of a build (LIB-8 A-3)
    calcu::paper::addPaperStep(m_presets.vocabulary(), m_paper);
    calcu::debugger::addDebuggerSteps(m_presets.vocabulary(), m_debugger);
    // A variant switch carries the module a bay holds (until M-47) and
    // remembers which side of a disk was up.
    m_machines.setModuleCarry([this](const PlugEntry& e, Machine&) -> std::shared_ptr<const ProductData> {
        const int slot = e.place.steps[0].value == sharp::kSlot2 ? 2 : 1;
        std::unique_ptr<ExpansionCard> card = m_modules->cardFor(slot);
        return card ? std::make_shared<sharp::ModuleCard>(std::move(card)) : nullptr;
    });

    ModuleBay::Events moduleEvents;
    moduleEvents.moduleChanged = [this](int slot) {
        if (slot >= 0 && static_cast<std::size_t>(slot) < m_machineState.moduleChanges.size())
            ++m_machineState.moduleChanges[static_cast<std::size_t>(slot)];
    };
    moduleEvents.error = [this](const std::string& text) { notice(text); };
    m_modules = std::make_unique<ModuleBay>(*this, m_paths, std::move(moduleEvents));
    FloppyBay::Events floppyEvents;
    floppyEvents.error = [this](const std::string& text) { notice(text); };
    m_floppy = std::make_unique<FloppyBay>(*this, m_paths, std::move(floppyEvents));
    TapeBay::Events tapeEvents;
    tapeEvents.tapesChanged = [this] { ++m_tapeListChanges; };
    tapeEvents.error = [this](const std::string& text) { notice(text); };
    m_tapes = std::make_unique<TapeBay>(*this, m_paths, std::move(tapeEvents));
}

SharpAppPart::~SharpAppPart() {
    // Save what's pending; the bays refer to this part.
    if (m_modules) m_modules->flushPendingPersist();
    if (m_floppy) m_floppy->flushPendingPersist();
    m_tapes.reset();
    m_floppy.reset();
    m_modules.reset();
    if (m_ms) {
        std::string ignored;
        tapeEject(&ignored);   // a recording goes to its file
    }
}

std::vector<sessions::SessionPart*> SharpAppPart::sessionParts() {
    // PeripheralsPart before StartupPart (the clocks at a build, LIB-18
    // §3.5), PresetsPart before StartupPart (LIB-6 §3.8).
    return {&m_typingPart, &m_peripherals, &m_presets, &m_startup, &m_loaderPart, &m_paper, &m_debugger, this};
}

void SharpAppPart::notice(const std::string& sentence) {
    if (m_ctx) m_ctx->notice(sentence);
}

void SharpAppPart::log(const std::string& line) {
    if (m_ctx) m_ctx->log(line);
}

// ── SessionPart ─────────────────────────────────────────────────────────

void SharpAppPart::attach(sessions::SessionSetup& setup) {
    m_ctx = &setup.context();
    m_ms = &m_ctx->machineSession();

    m_controlTopic = &setup.stateTopic(topics::kControlState);
    m_machineTopic = &setup.stateTopic(topics::kMachine);
    m_tapesTopic = &setup.stateTopic(topics::kTapes);
    m_serialTopic = &setup.stateTopic(topics::kSerialLinks);
    m_romsMissingTopic = &setup.stateTopic(topics::kRomsMissing);
    // LIB-12 AC-5: without the full set no machine is built; the frontends
    // show the files.
    if (!m_roms) m_romsMissingTopic->set(std::make_shared<RomsMissingPayload>(m_romsMissing));

    using sessions::Done;
    using sessions::Refused;
    using sessions::SessionContext;
    using sessions::Value;
    namespace c = commands;
    const char* busy = sessions::kAnotherOperationRuns;

    // AC-29 (until M-47 over the module bay): a module in a slot, or none.
    setup.command(c::kSelectModule, [this, busy](const Value& args, SessionContext& s) -> CommandOutcome {
        if (s.busy()) return Refused{busy};
        Machine* m = machine();
        if (!m) return Refused{sessions::kNoMachine};
        const int slot = static_cast<int>(integerArg(args, "slot", 1));
        if (slot < 1 || slot > slotCount()) return Refused{"There is no slot " + std::to_string(slot) + "."};
        const std::string name = textArg(args, "name");
        const std::string there = pluggedAt(slotPlace(slot));
        m_modules->selectModule(slot, name);
        startup::PlugChange ch;
        ch.place = placeOf(slotPlace(slot));
        if (name.empty()) {
            if (there.empty()) return Done{};
            ch.kind = startup::PlugChange::Kind::Detach;
        } else {
            std::unique_ptr<ExpansionCard> card = m_modules->cardFor(slot);
            if (!card) return Refused{"The module " + name + " could not be read."};
            ch.kind = there.empty() ? startup::PlugChange::Kind::Attach : startup::PlugChange::Kind::Replace;
            ch.request = moduleRequest(slot, std::move(card));
        }
        if (auto refused = machineControl().check(*m, ch)) return Refused{refused->reason};
        return change(std::move(ch), [this, slot] { m_modules->followMachine(slot); });
    });
    // AC-30: the host drive with its folder (a second folder: a replace), or none.
    setup.command(c::kMountHostDrive, [this, busy](const Value& args, SessionContext& s) -> CommandOutcome {
        if (s.busy()) return Refused{busy};
        Machine* m = machine();
        if (!m) return Refused{sessions::kNoMachine};
        const std::string dir = textArg(args, "dir");
        startup::PlugChange ch;
        ch.place = placeOf(sharp::kHostDrivePlace);
        if (dir.empty()) {
            if (!hostDriveAttached()) return Done{};
            ch.kind = startup::PlugChange::Kind::Detach;
        } else {
            ch.kind = hostDriveAttached() ? startup::PlugChange::Kind::Replace : startup::PlugChange::Kind::Attach;
            ch.request = machineControl().requestFor(ch.place, PeripheralId{sharp::kHostDrive}, std::nullopt);
            ch.request.content = std::make_shared<sharp::HostDriveRequest>(dir);
        }
        if (auto refused = machineControl().check(*m, ch)) return Refused{refused->reason};
        return change(std::move(ch));
    });
    // AC-31, D-21: the CE-1600P's ROM: a replace by the other variant, or the
    // choice for the next attach.
    setup.command(c::kCE1600PRom, [this, busy](const Value& args, SessionContext& s) -> CommandOutcome {
        if (s.busy()) return Refused{busy};
        const CE1600PRomVersion v = textArg(args, "version") == "old" ? CE1600PRomVersion::Old : CE1600PRomVersion::New;
        Machine* m = machine();
        if (!m || !ce1600pAttached()) {
            m_ce1600pChoice = v;
            refreshTopics();
            return Done{};
        }
        if (v == ce1600pRomVersion()) return Done{};
        m_ce1600pChoice = v;
        startup::PlugChange ch;
        ch.kind = startup::PlugChange::Kind::Replace;
        ch.place = placeOf(sharp::kPrinter);
        ch.request = machineControl().requestFor(ch.place, PeripheralId{sharp::kCE1600P}, ce1600pVariant(v));
        if (auto refused = machineControl().check(*m, ch)) return Refused{refused->reason};
        return change(std::move(ch));
    });
}

sessions::CommandOutcome SharpAppPart::change(startup::PlugChange ch, std::function<void()> after) {
    return sessions::StartOperation{[this, ch, after](sessions::OperationContext& o) -> sessions::Task<sessions::OperationResult> {
        co_await machineControl().change(o, ch);
        if (after) after();
        co_return sessions::OperationResult{};
    }};
}

void SharpAppPart::afterCall(sessions::SessionContext&, calcu::pacing::WallTime) {
    frameDuties();
    refreshTopics();
}

void SharpAppPart::beforeOperation(sessions::SessionContext&) {
    // Pending card and disk writes go first.
    m_modules->flushPendingPersist();
    m_floppy->flushPendingPersist();
}

void SharpAppPart::beforeMachineChange(sessions::SessionContext&) {
    m_modules->flushPendingPersist();
    m_floppy->flushPendingPersist();
    if (sharp::FloppyDriveAccess* d = floppyDrive(); d && d->hasDisk()) m_floppySide = d->side();
    std::string tapeError;
    if (!tapeEject(&tapeError)) log("Tape: the tape recording could not be saved: " + tapeError);
}

void SharpAppPart::machineChanged(sessions::SessionContext&) {
    if (!m_ms || m_ms->generation() == m_seenGeneration) return;
    m_seenGeneration = m_ms->generation();
    followNewMachine();
    refreshTopics();
}

void SharpAppPart::followNewMachine() {
    const Machine* m = machine();
    if (!m) return;
    const Model before = m_model;
    if (auto model = modelOfId(m->descriptor().model.value)) m_model = *model;
    ++m_machineState.builds;
    const auto& built = machineControl().lastBuild();
    const startup::BuildOrigin origin = built ? built->origin : startup::BuildOrigin::Start;
    switch (origin) {
        case startup::BuildOrigin::VariantSwitch:
            // AC-28: the plugs went along; the bays keep what they hold, and
            // the disk goes into the new drive with the same side up.
            m_modules->followCarriedMachine();
            m_floppy->insertSelectedDisk();
            if (sharp::FloppyDriveAccess* d = floppyDrive(); d && d->hasDisk() && m_floppySide != 0) d->setSide(m_floppySide);
            return;
        case startup::BuildOrigin::Preset:
        case startup::BuildOrigin::StartupPreset:
            // What the preset plugged in (adapter "bays on machineChanged",
            // until M-47): the modules by name are templates; a slot ROM
            // makes its slot unsaveable.
            if (before != m_model) m_ce1600pChoice = CE1600PRomVersion::New;
            for (int slot = 1; slot <= 2; ++slot) {
                const std::vector<std::uint8_t>* rom = nullptr;
                if (built)
                    for (const PeripheralRequest& r : built->plan.peripherals)
                        if (r.place == placeOf(slotPlace(slot)))
                            if (const auto* s = dynamic_cast<const sharp::SlotRomRequest*>(r.content.get())) rom = &s->bytes;
                m_modules->followPreset(slot, rom);
            }
            m_floppy->syncFromPresetLoad({}, {});
            ++m_machineState.modelSwitches;
            return;
        case startup::BuildOrigin::Start:
        case startup::BuildOrigin::ModelSwitch:
            // AC-27: nothing of the old machine is kept.
            m_modules->onModelChanged();
            m_floppy->selectDisk({});
            m_ce1600pChoice = CE1600PRomVersion::New;
            ++m_machineState.modelSwitches;
            return;
    }
}

void SharpAppPart::operationEnded(sessions::SessionContext&) {
    if (Machine* m = machine()) m_peripherals.hostLinks().machineChanged(*m);   // a CE-158 plugged in
    // A CE-1600F plugged in now gets the disk the bay holds.
    if (sharp::FloppyDriveAccess* d = floppyDrive(); d && !d->hasDisk() && !m_floppy->selectedDiskName().empty())
        m_floppy->insertSelectedDisk();
    frameDuties();   // the bays catch up with whatever the operation changed
    refreshTopics();
}

void SharpAppPart::frameDuties() {
    const double now = steadySeconds();
    m_modules->tick(now);
    m_floppy->tick(now);
    m_tapeStatus = tapeStatus();
    m_tapes->onFrameTick(m_tapeStatus);
}

void SharpAppPart::refreshTopics() {
    if (!m_controlTopic) return;
    m_controlTopic->set(std::make_shared<ControlStatePayload>(controlState()));
    m_machineState.model = m_model;
    m_machineTopic->set(std::make_shared<MachinePayload>(m_machineState));
    auto tapes = std::make_shared<TapesPayload>();
    tapes->status = m_tapeStatus;
    tapes->listChanges = m_tapeListChanges;
    m_tapesTopic->set(std::move(tapes));
    auto links = std::make_shared<SerialLinksPayload>();
    links->rs232c = serialLinkStatus();
    links->ce158 = ce158SerialLinkStatus();
    links->moves = m_serialMoves;
    m_serialTopic->set(std::move(links));
}

// ── The machine ─────────────────────────────────────────────────────────

PC1500RomRevision SharpAppPart::pc1500RomRevision() const {
    const Machine* m = machine();
    const std::string v = m && m->variant() ? m->variant()->value : std::string("A04");
    if (v == "A01") return PC1500RomRevision::A01;
    if (v == "A03") return PC1500RomRevision::A03;
    return PC1500RomRevision::A04;
}

PC1600RomVersion SharpAppPart::pc1600RomVersion() const {
    const Machine* m = machine();
    return m && m->variant() && m->variant()->value == "Old" ? PC1600RomVersion::Old : PC1600RomVersion::New;
}

CE1600PRomVersion SharpAppPart::ce1600pRomVersion() const {
    if (const Machine* m = machine())
        for (const auto& e : m->plugged().byPlace)
            if (e.peripheral.value == sharp::kCE1600P)
                return e.variant && e.variant->value == "Old" ? CE1600PRomVersion::Old : CE1600PRomVersion::New;
    return m_ce1600pChoice;
}

void SharpAppPart::refreshSerialLinkDirectory() {
    m_peripherals.hostLinks().moveTo(folderOrNone(m_paths.serialLinkDir()));
    ++m_serialMoves;
    refreshTopics();
}

namespace {

std::string linkPath(const calcu::peripherals::HostLinks& links, const char* file) {
    for (const auto& s : links.status())
        if (s.fileName == file && s.open) return s.path;
    return {};
}

} // namespace

std::string SharpAppPart::serialLinkStatus() const {
    return linkPath(const_cast<SharpAppPart*>(this)->m_peripherals.hostLinks(), "calcu1600-rs232c.serial");
}

std::string SharpAppPart::ce158SerialLinkStatus() const {
    return linkPath(const_cast<SharpAppPart*>(this)->m_peripherals.hostLinks(), "calcu1600-ce158.serial");
}

double SharpAppPart::clockHz() const {
    if (const Machine* m = machine()) return static_cast<double>(m->descriptor().tickRate);
    const char* id = m_model == Model::PC1500 ? sharp::kPC1500 : m_model == Model::PC1600 ? sharp::kPC1600 : sharp::kPC1500A;
    return static_cast<double>(sharp::sharpCatalogue().model(ModelId{id})->tickRate);
}

// ---- Output ----

bool SharpAppPart::lcdText(const DisplayOutput& output, LcdText* out) const {
    Machine* m = machine();
    if (!m) return false;
    *out = displayText(*m, output);
    return true;
}

GrayImage SharpAppPart::screenImage(const DisplayOutput& output) const {
    if (!machine()) return GrayImage{};
    return renderLcdImage(lcdBitmapOf(output), m_model == Model::PC1600 ? kPC1600ScreenMm : kPC1500ScreenMm);
}

// ---- Debug panel ----

std::vector<std::string> SharpAppPart::inspectorView(inspect::View view) const {
    auto* k = kind<sharp::InspectorViews>();
    return k ? k->view(view) : std::vector<std::string>{};
}

std::vector<inspect::MenuEntry> SharpAppPart::inspectorMenu(bool dumps) const {
    auto* k = kind<sharp::InspectorViews>();
    return k ? k->menu(dumps) : std::vector<inspect::MenuEntry>{};
}

std::vector<std::uint8_t> SharpAppPart::debugSlotCardImage(int slot, int* bankCount) const {
    *bankCount = -1;
    auto* k = kind<sharp::InspectorViews>();
    return k ? k->slotImage(slot, bankCount) : std::vector<std::uint8_t>{};
}

// ---- Expansion slots ----

int SharpAppPart::slotCount() const {
    const Machine* m = machine();
    if (!m) return 0;
    int n = 0;
    for (const auto& p : m->descriptor().places)
        if (p.name.value == sharp::kSlot1 || p.name.value == sharp::kSlot2) ++n;
    return n;
}

const ExpansionCard* SharpAppPart::slotCard(int slot) const {
    Machine* m = machine();
    return m ? sharp::legacy::moduleIn(*m, slot) : nullptr;
}

void SharpAppPart::setSlotWriteProtected(int slot, bool protect) {
    Machine* m = machine();
    if (!m) return;
    const Owner owner = PeripheralOwner{PeripheralId{pluggedAt(slotPlace(slot))}, PlacePath{{PlaceName{slotPlace(slot)}}}};
    if (auto* controls = calcu::peripherals::controlsCapability(*m, owner)) controls->set(ControlId{"write-protect"}, protect);
}


PeripheralRequest SharpAppPart::moduleRequest(int slot, std::unique_ptr<ExpansionCard> card) {
    const std::string name = card->moduleName();
    if (!m_catalogue.peripheral(PeripheralId{name})) {
        if (auto* sd = dynamic_cast<const SoftwareDefinedCard*>(card.get())) {
            m_catalogue.addPeripheral(sharp::moduleDescriptor(sd->definition()));
        } else {
            // A card without a definition: it fits the slot it was made for.
            PeripheralDescriptor d;
            d.id = PeripheralId{name};
            d.name = name;
            if (const Machine* m = machine()) d.fits = {m->descriptor().place(PlaceName{slotPlace(slot)})->pinOut};
            m_catalogue.addPeripheral(std::move(d));
        }
    }
    PeripheralRequest r = machineControl().requestFor(placeOf(slotPlace(slot)), PeripheralId{name}, std::nullopt);
    r.content = std::make_shared<sharp::ModuleCard>(std::move(card));
    return r;
}

// ---- Cassette recorder ----

bool SharpAppPart::tapeInterfaceAttached() const {
    const std::string printer = pluggedAt(sharp::kPrinter);
    return m_model == Model::PC1600 ? printer == sharp::kCE1600P : printer == sharp::kCE150;
}

bool SharpAppPart::tapePlay(const std::string& path, std::string* error) {
    auto* tape = kind<sharp::TapeDeckAccess>();
    std::string err;
    const bool ok = tape && tape->play(path, err);
    if (!ok && error) *error = err;
    return ok;
}

void SharpAppPart::tapeRecord(const std::string& path) {
    if (auto* tape = kind<sharp::TapeDeckAccess>()) tape->record(path);
}

bool SharpAppPart::tapeEject(std::string* error) {
    auto* tape = kind<sharp::TapeDeckAccess>();
    if (!tape) return true;
    std::string err;
    const bool ok = tape->eject(&err);
    if (!ok && error) *error = err;
    return ok;
}

TapeDeck::Status SharpAppPart::tapeStatus() const {
    auto* tape = kind<sharp::TapeDeckAccess>();
    return tape ? tape->status() : TapeDeck::Status{};
}

std::string SharpAppPart::tapeTakeLastError() {
    auto* tape = kind<sharp::TapeDeckAccess>();
    return tape ? tape->takeLastError() : std::string();
}

// ---- Peripherals ----

std::string SharpAppPart::pluggedAt(const char* place) const {
    const Machine* m = machine();
    if (!m) return {};
    for (const auto& e : m->plugged().byPlace)
        if (e.place.steps.size() == 1 && e.place.steps[0].value == place) return e.peripheral.value;
    return {};
}

sharp::FloppyDriveAccess* SharpAppPart::floppyDrive() const { return kind<sharp::FloppyDriveAccess>(); }

bool SharpAppPart::ce150Attached() const { return pluggedAt(sharp::kPrinter) == sharp::kCE150; }

bool SharpAppPart::ce1600pAttached() const { return pluggedAt(sharp::kPrinter) == sharp::kCE1600P; }

bool SharpAppPart::ce158Attached() const { return pluggedAt(sharp::kInterface) == sharp::kCE158; }

std::vector<std::uint8_t> SharpAppPart::drainCE158PrinterOutput() {
    auto* port = kind<sharp::Ce158PrinterPort>();
    return port ? port->drain() : std::vector<std::uint8_t>{};
}

bool SharpAppPart::hostDriveAttached() const { return pluggedAt(sharp::kHostDrivePlace) == sharp::kHostDrive; }

std::string SharpAppPart::hostDriveDirectory() const {
    auto* folder = kind<sharp::HostDriveFolder>();
    return folder ? folder->folder() : std::string();
}

// ---- Payloads ----

sessions::Value ControlStatePayload::toValue() const {
    auto peripheral = [](const PeripheralState& p) {
        return sessions::Value::object({{"offered", p.offered},
                                        {"onControlBar", p.onControlBar},
                                        {"attached", p.attached},
                                        {"enabled", p.enabled}});
    };
    const ControlState& s = state;
    return sessions::Value::object({{"model", settingsModelName(s.model)},
                                    {"slotCount", s.slotCount},
                                    {"ce150", peripheral(s.ce150)},
                                    {"ce1600p", peripheral(s.ce1600p)},
                                    {"ce158", peripheral(s.ce158)},
                                    {"floppyShown", s.floppyShown},
                                    {"floppyEnabled", s.floppyEnabled},
                                    {"tapeShown", s.tapeShown},
                                    {"pc1500RomMenu", s.pc1500RomMenu},
                                    {"pc1600RomMenu", s.pc1600RomMenu},
                                    {"ce1600pRomMenu", s.ce1600pRomMenu},
                                    {"hostDriveOffered", s.hostDriveOffered},
                                    {"hostDriveDir", s.hostDriveDir},
                                    {"paper", s.paperOwner ? s.paperOwner->value : std::string()},
                                    {"printerShown", s.printerShown}});
}

sessions::Value MachinePayload::toValue() const {
    sessions::Value::Array modules;
    for (auto n : moduleChanges) modules.emplace_back(static_cast<std::int64_t>(n));
    return sessions::Value::object({{"model", settingsModelName(model)},
                                    {"builds", static_cast<std::int64_t>(builds)},
                                    {"modelSwitches", static_cast<std::int64_t>(modelSwitches)},
                                    {"moduleChanges", std::move(modules)}});
}

sessions::Value TapesPayload::toValue() const {
    return sessions::Value::object({{"mode", static_cast<int>(status.mode)},
                                    {"motor", status.motor},
                                    {"position", status.position},
                                    {"length", status.length},
                                    {"path", status.path},
                                    {"listChanges", static_cast<std::int64_t>(listChanges)}});
}

sessions::Value SerialLinksPayload::toValue() const {
    return sessions::Value::object({{"rs232c", rs232c}, {"ce158", ce158}, {"moves", static_cast<std::int64_t>(moves)}});
}

sessions::Value RomsMissingPayload::toValue() const {
    return sessions::Value::object({{"text", calcu::roms::describe(missing)}});
}

} // namespace shared
