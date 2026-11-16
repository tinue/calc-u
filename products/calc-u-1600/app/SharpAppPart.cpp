#include "app/SharpAppPart.hpp"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <stdexcept>

#include "Connector/SoftwareDefinedCard.hpp"
#include "Display/DisplayOutputText.hpp"
#include "Machine/SharpBuilder.hpp"
#include "Machine/SharpCatalogue.hpp"
#include "Machine/SharpMachine.hpp"
#include "Machine/SharpRoms.hpp"
#include "app/SharpDebuggerSetup.hpp"
#include "app/SharpMediaKinds.hpp"
#include "app/SharpPrinters.hpp"
#include "app/Utf8Path.hpp"
#ifndef __EMSCRIPTEN__
#include "app/dap/DapPart.hpp"
#endif
#include "media/PresetMedia.hpp"
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



calcu::roms::LookupResult lookUp(const AppPaths& paths) {
    return sharp::lookUpRoms(calcu::roms::DirectoryBundle(toPath(paths.bundledRomsDir())));
}

std::shared_ptr<const std::vector<std::uint8_t>> hostDriveRomOf(const AppPaths& paths) {
    auto rom = sharp::readHostDriveRom({paths.bundledHostDriveRomDir()});
    return rom ? std::make_shared<const std::vector<std::uint8_t>>(std::move(*rom)) : nullptr;
}

using sessions::textArg;

} // namespace

SharpAppPart::SharpAppPart(Settings& settings, AppPaths& paths, SharpAppPartOptions options)
    : m_settings(settings),
      m_paths(paths),
      m_options(options),
      m_catalogue(sharp::sharpCatalogue()),
      m_found(lookUp(paths)),
      m_roms(m_found.set),
      m_romsMissing(m_found.missing),
      m_hostDriveRom(hostDriveRomOf(paths)),
      m_paper(m_catalogue, sharpPrinterPapers()),
      m_debugger(sharpDebuggerSetup(&paths, &settings, /*hasTraceFolder=*/true)),
      m_peripherals(
          [this](const CapabilityEntry& e) -> std::optional<calcu::peripherals::HostLinkRule> {
              std::optional<calcu::peripherals::HostLinkRule> rule = serialFileFor(e);
              if (!rule || !m_options.serialLinksOnRequest) return rule;
              // C1600-5 AC-9: only a file asked for, made at once.
              if (std::find(m_requestedLinks.begin(), m_requestedLinks.end(), rule->fileName) == m_requestedLinks.end())
                  return std::nullopt;
              rule->openAtOnce = true;
              return rule;
          },
          folderOrNone(paths.serialLinkDir())),
      m_typingPart(typingProduct(), options.hosting),
      m_machines(m_catalogue, m_hostDriveRom),
      m_rules(SharpKeywordSetup{[this] { return canMountHostFolder(); }}),
      m_bundle(toPath(paths.bundledExamplesDir())),
      m_presets(calcu::presets::PresetsSetup{&m_catalogue, &m_rules, nullptr, &m_typingPart, &m_bundle,
                                             paths.homeDir(),
                                             [this, &paths]() -> std::optional<std::string> {
                                                 // LIB-6 AC-25: the save folder, made when a step needs it.
                                                 auto folder = m_options.screenshotFolder ? m_options.screenshotFolder()
                                                                                          : paths.places().screenshotFolder();
                                                 std::error_code ec;
                                                 if (folder) std::filesystem::create_directories(toPath(*folder), ec);
                                                 return folder;
                                             }}),
      m_startup(calcu::startup::StartupSetup{m_machines, m_roms, settings.file(), &m_presets,
                                             m_roms ? std::string() : calcu::roms::describe(m_romsMissing)}),
      m_controlRules(SharpControlRules::Inputs{options.canMountHostFolder, [this] { return ce1600pVariant(); }}),
      m_control(sharpControlSetup(m_catalogue, m_controlRules)) {
    // The module definitions: the bundled ones and the ROM modules of the set
    // (the CE-502B; LIB-12 AC-14).
    sharp::addModuleDefinitions(m_catalogue, paths.bundledModulesDir());
    if (m_roms) sharp::addModuleDefinitions(m_catalogue, *m_roms);
    m_presets.setControl(&m_startup.control());
    m_paper.setControl(&m_startup.control());   // the origin of a build (LIB-8 A-3)
    calcu::paper::addPaperStep(m_presets.vocabulary(), m_paper);
    calcu::debugger::addDebuggerSteps(m_presets.vocabulary(), m_debugger);

    // LIB-7 (§10.2): the media part on the save folder and the tape folder,
    // the bundled templates beside the module definitions; a variant switch
    // carries a module's content over (S-7a), the disk and the tape too.
    calcu::media::MediaSetup media;
    media.kinds = mediaKinds(paths.bundledModulesDir(), m_catalogue);
    media.paths = &paths.places();
    media.listing = moduleListing();
    media.catalogue = &m_catalogue;
    m_media = std::make_unique<calcu::media::MediaPart>(std::move(media));
    m_media->setControl(&m_startup.control());
    calcu::media::addMediaToPresets(m_presets.vocabulary(), *m_media);

    // LIB-13 (Part D, D3): the product's open setup.
    m_presetsOpener = std::make_unique<calcu::opening::PresetsOpener>(m_presets, presetExtensions());
    m_mediaOpener = std::make_unique<calcu::opening::MediaOpener>(*m_media);
    calcu::opening::OpenSetup open;
    open.productName = "Calc-U-1600";
    open.presets = m_presetsOpener.get();
    open.media = m_mediaOpener.get();
    open.kinds = {&m_basicKind, &m_codeKind};   // AC-17: BASIC first
    open.maxBytes = openLimits();
    m_open = std::make_unique<calcu::opening::OpenPart>(std::move(open));

#ifndef __EMSCRIPTEN__
    // C1600-3 (design §3.3): the DAP server on the library's parts.
    dap::DapDeps deps;
    deps.debugger = &m_debugger;
    deps.typing = &m_typingPart;
    deps.presets = &m_presets;
    deps.control = &m_startup.control();
    deps.settings = &settings.file();
    deps.bundle = &m_bundle;
    m_dap = std::make_shared<dap::DapPart>(deps);
#endif
}

SharpAppPart::~SharpAppPart() = default;

std::vector<sessions::SessionPart*> SharpAppPart::sessionParts() {
    // PeripheralsPart before StartupPart (the clocks at a build, LIB-18
    // §3.5), PresetsPart before StartupPart (LIB-6 §3.8), MediaPart after
    // StartupPart (LIB-7 §3.7). The host calls AppSession::close() before
    // it ends the session: the media part writes what is pending (AC-13).
    std::vector<sessions::SessionPart*> parts{&m_typingPart, &m_peripherals, &m_presets,  &m_startup, m_media.get(),
                                              &m_loaderPart, m_open.get(),   &m_paper,    &m_debugger};
    // C1600-3: the DAP server after the debugger (it reads the run control
    // after the debugger's own machineChanged).
#ifndef __EMSCRIPTEN__
    if (m_dap) parts.push_back(m_dap.get());
#endif
    parts.push_back(this);
    return parts;
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

    m_lampsTopic = &setup.stateTopic(topics::kLamps);
    m_serialTopic = &setup.stateTopic(topics::kSerialLinks);
    m_romsMissingTopic = &setup.stateTopic(topics::kRomsMissing);
    // LIB-13 D4.2: a program file opened by a way with a window.
    sessions::StreamTopic* opened = &setup.streamTopic(topics::kProgramOpened, sessions::History::None);
    m_basicKind.setTopic(opened);
    m_codeKind.setTopic(opened);
    // LIB-12 AC-5: without the full set no machine is built; the frontends
    // show the files.
    if (!m_roms) m_romsMissingTopic->set(std::make_shared<RomsMissingPayload>(m_romsMissing));

    using sessions::Done;
    using sessions::Refused;
    using sessions::SessionContext;
    using sessions::Value;
    namespace c = commands;
    const char* busy = sessions::kAnotherOperationRuns;

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
        const std::string v = textArg(args, "version") == "old" ? "Old" : "New";
        Machine* m = machine();
        if (!m || !ce1600pAttached()) {
            m_ce1600pChoice = v;
            refreshTopics();
            return Done{};
        }
        if (v == ce1600pVariant()) return Done{};
        m_ce1600pChoice = v;
        startup::PlugChange ch;
        ch.kind = startup::PlugChange::Kind::Replace;
        ch.place = placeOf(sharp::kPrinter);
        ch.request = machineControl().requestFor(ch.place, PeripheralId{sharp::kCE1600P}, VariantId{v});
        if (auto refused = machineControl().check(*m, ch)) return Refused{refused->reason};
        // The disk and the tape stay in the new CE-1600P, the disk with the
        // same side up (AC-31, as today).
        struct Held {
            std::string word;
            calcu::media::Binding binding;
            calcu::media::Content content;
        };
        auto held = std::make_shared<std::vector<Held>>();
        for (const char* word : {"floppy", "tape"}) {
            m_media->flush(s, word);
            const calcu::media::Binding* b = m_media->bindingOf(word);
            if (!b) continue;
            calcu::media::Content content = b->content;
            for (const auto& e : m->capabilities(calcu::media::MediaPlace::kKind))
                if (auto* cap = calcu::machine::capabilityAs<calcu::media::MediaPlace>(*m, e.id);
                    cap && cap->side() && (cap->id().value == "floppy") == (std::string(word) == "floppy"))
                    content = cap->content();
            held->push_back(Held{word, *b, content});
        }
        return change(std::move(ch), [this, held] {
            for (Held& h : *held)
                if (h.content && m_ctx) m_media->insert(*m_ctx, h.word, h.binding, h.content, h.binding.side.value_or(0));
        });
    });
}

sessions::CommandOutcome SharpAppPart::change(startup::PlugChange ch, std::function<void()> after) {
    return sessions::StartOperation{[this, ch, after](sessions::OperationContext& o) -> sessions::Task<sessions::OperationResult> {
        co_await machineControl().change(o, ch);
        if (after) after();
        co_return sessions::OperationResult{};
    }};
}

void SharpAppPart::afterCall(sessions::SessionContext&, calcu::pacing::WallTime) { refreshTopics(); }

void SharpAppPart::machineChanged(sessions::SessionContext&) {
    if (!m_ms || m_ms->generation() == m_seenGeneration) return;
    m_seenGeneration = m_ms->generation();
    followNewMachine();
    refreshTopics();
}

void SharpAppPart::followNewMachine() {
    const Machine* m = machine();
    if (!m) return;
    const std::string before = m_lastModel;
    m_lastModel = m->descriptor().model.value;
    const auto& built = machineControl().lastBuild();
    const startup::BuildOrigin origin = built ? built->origin : startup::BuildOrigin::Start;
    switch (origin) {
        case startup::BuildOrigin::VariantSwitch:
            // AC-28: the plugs and the media went along (the media part, S-7a).
            return;
        case startup::BuildOrigin::Preset:
        case startup::BuildOrigin::StartupPreset:
            if (before != m_lastModel) m_ce1600pChoice = "New";
            return;
        case startup::BuildOrigin::Start:
        case startup::BuildOrigin::ModelSwitch:
            // AC-27: nothing of the old machine is kept.
            m_ce1600pChoice = "New";
            return;
    }
}

void SharpAppPart::operationEnded(sessions::SessionContext&) {
    if (Machine* m = machine()) m_peripherals.hostLinks().machineChanged(*m);   // a CE-158 plugged in
    refreshTopics();
}

void SharpAppPart::refreshTopics() {
    if (!m_lampsTopic) return;
    auto lamps = std::make_shared<LampsPayload>();
    lamps->tape = tapeStatus();
    const calcu::media::Binding* tape = m_media->bindingOf("tape");
    lamps->tapeName = tape ? tape->name : std::string();
    lamps->floppyMotor = floppyMotorOn();
    m_lampsTopic->set(std::move(lamps));
    auto links = std::make_shared<SerialLinksPayload>();
    links->rs232c = serialLinkStatus();
    links->ce158 = ce158SerialLinkStatus();
    links->moves = m_serialMoves;
    m_serialTopic->set(std::move(links));
}

// ── The machine ─────────────────────────────────────────────────────────

ModelId SharpAppPart::model() const {
    if (const Machine* m = machine()) return m->descriptor().model;
    return m_settings.startupModel();
}

std::string SharpAppPart::ce1600pVariant() const {
    if (const Machine* m = machine())
        for (const auto& e : m->plugged().byPlace)
            if (e.peripheral.value == sharp::kCE1600P) return e.variant && e.variant->value == "Old" ? "Old" : "New";
    return m_ce1600pChoice;
}

std::string SharpAppPart::requestSerialLink(const std::string& fileName) {
    if (std::find(m_requestedLinks.begin(), m_requestedLinks.end(), fileName) == m_requestedLinks.end())
        m_requestedLinks.push_back(fileName);
    if (Machine* m = machine()) m_peripherals.hostLinks().machineChanged(*m);
    for (const auto& st : m_peripherals.hostLinks().status())
        if (st.fileName == fileName) return st.path;
    return {};
}

bool SharpAppPart::serialLinked(const std::string& fileName) const {
    if (m_options.serialLinksOnRequest &&
        std::find(m_requestedLinks.begin(), m_requestedLinks.end(), fileName) == m_requestedLinks.end())
        return false;
    for (const auto& st : m_peripherals.hostLinks().status())
        if (st.fileName == fileName) return st.connected;
    return false;
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
    return static_cast<double>(sharp::sharpCatalogue().model(model())->tickRate);
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
    return renderLcdImage(lcdBitmapOf(output), isPC1600() ? kPC1600ScreenMm : kPC1500ScreenMm);
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

std::string SharpAppPart::slotModuleName(int slot) const {
    // What the slot shows now (plugged(): a module still waiting for the off
    // point too); the PC-1500 has one slot.
    const Machine* m = machine();
    if (!m || slot < 1 || slot > slotCount()) return {};
    const std::string place = slot == 2 ? sharp::kSlot2 : sharp::kSlot1;
    for (const auto& e : m->plugged().byPlace)
        if (e.place.steps.size() == 1 && e.place.steps[0].value == place) return e.peripheral.value;
    return {};
}

// ---- Cassette recorder and CE-1600F ----

bool SharpAppPart::tapeInterfaceAttached() const {
    // The cassette of a CE-150 or a CE-1600P, on every model (C1600-1 AC-16,
    // LIB-7 OQ-9).
    const std::string printer = pluggedAt(sharp::kPrinter);
    return printer == sharp::kCE150 || printer == sharp::kCE1600P;
}

TapeDeck::Status SharpAppPart::tapeStatus() const {
    auto* tape = kind<sharp::TapeDeckAccess>();
    return tape ? tape->status() : TapeDeck::Status{};
}

bool SharpAppPart::floppyMotorOn() const {
    auto* drive = kind<sharp::FloppyDriveAccess>();
    return drive && drive->motorOn();
}

// ---- Peripherals ----

std::string SharpAppPart::pluggedAt(const char* place) const {
    const Machine* m = machine();
    if (!m) return {};
    for (const auto& e : m->plugged().byPlace)
        if (e.place.steps.size() == 1 && e.place.steps[0].value == place) return e.peripheral.value;
    return {};
}


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

sessions::Value LampsPayload::toValue() const {
    return sessions::Value::object(
        {{"floppyMotor", floppyMotor},
         {"tapeMode", tape.mode == TapeDeck::Mode::Play ? "play" : tape.mode == TapeDeck::Mode::Record ? "record" : ""},
         {"tapeMotor", tape.motor},
         {"tapeSeconds", static_cast<int>(tape.position)},
         {"tapeLength", static_cast<int>(tape.length)},
         {"tapeFile", tapeName}});
}
sessions::Value SerialLinksPayload::toValue() const {
    return sessions::Value::object({{"rs232c", rs232c}, {"ce158", ce158}, {"moves", static_cast<std::int64_t>(moves)}});
}

sessions::Value RomsMissingPayload::toValue() const {
    return sessions::Value::object({{"text", calcu::roms::describe(missing)}});
}

} // namespace shared
