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
#include "Preset/PresetFile.hpp"
#include "app/FloppyBay.hpp"
#include "app/ModuleBay.hpp"
#include "app/SharpDebuggerSetup.hpp"
#include "app/SharpPrinters.hpp"
#include "app/TapeBay.hpp"
#include "app/Utf8Path.hpp"
#include "peripherals/Controls.hpp"
#include "peripherals/RealTimeClock.hpp"
#include "peripherals/SerialPort.hpp"
#include "sessions/LibraryTopics.hpp"
#include "sessions/Args.hpp"

namespace shared {

using namespace calcu::machine;
namespace sessions = calcu::sessions;

namespace {

const char* pc1500RomVariantName(PC1500RomRevision revision) {
    switch (revision) {
        case PC1500RomRevision::A01: return "A01";
        case PC1500RomRevision::A03: return "A03";
        case PC1500RomRevision::A04: return "A04";
    }
    return "A04";
}

const char* modelIdOf(Model model) {
    switch (model) {
        case Model::PC1500: return sharp::kPC1500;
        case Model::PC1500A: return sharp::kPC1500A;
        case Model::PC1600: return sharp::kPC1600;
    }
    return sharp::kPC1500A;
}

std::optional<Model> modelOfId(const std::string& id) {
    if (id == sharp::kPC1500) return Model::PC1500;
    if (id == sharp::kPC1500A) return Model::PC1500A;
    if (id == sharp::kPC1600) return Model::PC1600;
    return std::nullopt;
}

const char* slotPlace(int slot) { return slot == 2 ? sharp::kSlot2 : sharp::kSlot1; }

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

Model modelForPreset(const PresetFile& preset) {
    if (preset.isPC1600()) return Model::PC1600;
    return preset.variant == PC1500Variant::PC1500A ? Model::PC1500A : Model::PC1500;
}

const char* peripheralName(Peripheral p) {
    switch (p) {
        case Peripheral::CE150: return "CE-150";
        case Peripheral::CE1600P: return "CE-1600P";
        case Peripheral::CE158: return "CE-158";
    }
    return "";
}

std::string attachFailedSentence(const std::string& device, const std::string& reason) {
    return reason.empty() ? "The " + device + " could not be attached."
                          : "The " + device + " could not be attached: " + reason;
}

double steadySeconds() {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

// ---- Command arguments ----

using sessions::bytesArg;
using sessions::flagArg;
using sessions::integerArg;
using sessions::textArg;

std::optional<Model> modelArg(const sessions::Value& args) {
    const std::string name = textArg(args, "model");
    if (auto m = modelFromSettingsName(name)) return m;
    return modelOfId(name);
}

const char* romVersionName(bool old) { return old ? "old" : "new"; }

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
      m_paper(m_catalogue, sharpPrinterPapers()),
      m_debugger(sharpDebuggerSetup(&paths, &settings, /*hasTraceFolder=*/true)),
      m_peripherals(serialFileFor, folderOrNone(paths.serialLinkDir())) {
    // The one lookup, at the start (LIB-12 AC-4, AC-13): the whole full set
    // or nothing. The host drive's ROM is bundled beside it (roms.md Q-1).
    calcu::roms::LookupResult found = sharp::lookUpRoms(calcu::roms::DirectoryBundle(toPath(paths.bundledResourcesDir())));
    m_roms = std::move(found.set);
    m_romsMissing = std::move(found.missing);
    if (m_roms) sharp::addModuleDefinitions(m_catalogue, *m_roms);
    if (auto rom = sharp::readHostDriveRom({paths.bundledResourcesDir()}))
        m_hostDriveRom = std::make_shared<const std::vector<std::uint8_t>>(std::move(*rom));

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
    return {&m_typingPart, &m_peripherals, &m_paper, &m_debugger, this};
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

    using sessions::Done;
    using sessions::Refused;
    using sessions::SessionContext;
    using sessions::Value;
    namespace c = commands;

    // The start (LIB-15 N3; LIB-18 AC-5 until M-35): the startup model of
    // the settings, or the one named, cold booted; then a preset if named,
    // else the model's default preset. It ends a running operation.
    setup.command(
        c::kStart,
        [this](const Value& args, SessionContext&) -> CommandOutcome {
            if (!m_roms) {
                m_romsMissingTopic->set(std::make_shared<RomsMissingPayload>(m_romsMissing));
                return Refused{calcu::roms::describe(m_romsMissing)};
            }
            std::string model;
            if (auto m = modelArg(args)) model = settingsModelName(*m);
            else if (!textArg(args, "model").empty()) notice("There is no model \"" + textArg(args, "model") + "\"; the startup model is used.");
            const std::string preset = textArg(args, "preset");
            return operation([this, model, preset] { return startJob(model, preset); });
        },
        /*endsRunningOperation=*/true);
    setup.command(c::kLoadPreset, [this](const Value& args, SessionContext&) -> CommandOutcome {
        const std::string path = textArg(args, "path");
        const bool armOnly = flagArg(args, "armOnly");
        return operation([this, path, armOnly] { return loadPresetJob(path, armOnly); });
    });
    setup.command(c::kLoadDefaultPreset, [this](const Value& args, SessionContext&) -> CommandOutcome {
        const std::string path = textArg(args, "path");
        const auto model = modelArg(args);
        if (!model) return Refused{"The load-default-preset command needs a model."};
        return operation([this, path, m = *model] { return loadDefaultPresetJob(path, m); });
    });
    setup.command(c::kDefaultPreset, [this](const Value&, SessionContext&) -> CommandOutcome {
        return operation([this] { return defaultPresetJob(); });
    });
    setup.command(
        c::kReset,
        [this](const Value& args, SessionContext&) -> CommandOutcome {
            const bool all = flagArg(args, "all");
            return operation([this, all] { return resetJob(all); });
        },
        /*endsRunningOperation=*/true);
    setup.command(
        c::kPickModel,
        [this](const Value& args, SessionContext&) -> CommandOutcome {
            const auto model = modelArg(args);
            if (!model) return Refused{"There is no model \"" + textArg(args, "model") + "\"."};
            return operation([this, m = *model] { return pickModelJob(m); });
        },
        /*endsRunningOperation=*/true);
    setup.command(c::kSelectModule, [this](const Value& args, SessionContext&) -> CommandOutcome {
        const int slot = static_cast<int>(integerArg(args, "slot", 1));
        const std::string name = textArg(args, "name");
        return operation([this, slot, name] { return selectModuleJob(slot, name); });
    });
    setup.command(c::kPC1500Rom, [this](const Value& args, SessionContext&) -> CommandOutcome {
        const std::string r = textArg(args, "revision");
        PC1500RomRevision rev = PC1500RomRevision::A04;
        if (r == "A01") rev = PC1500RomRevision::A01;
        else if (r == "A03") rev = PC1500RomRevision::A03;
        else if (r != "A04") return Refused{"There is no PC-1500 ROM \"" + r + "\"."};
        return operation([this, rev] { return selectPC1500RomJob(rev); });
    });
    setup.command(c::kPC1600Rom, [this](const Value& args, SessionContext&) -> CommandOutcome {
        const PC1600RomVersion v = textArg(args, "version") == "old" ? PC1600RomVersion::Old : PC1600RomVersion::New;
        return operation([this, v] { return selectPC1600RomJob(v); });
    });
    setup.command(c::kCE1600PRom, [this](const Value& args, SessionContext&) -> CommandOutcome {
        const CE1600PRomVersion v = textArg(args, "version") == "old" ? CE1600PRomVersion::Old : CE1600PRomVersion::New;
        return operation([this, v] { return selectCE1600PRomJob(v); });
    });
    setup.command(c::kTogglePeripheral, [this](const Value& args, SessionContext&) -> CommandOutcome {
        const std::string name = textArg(args, "peripheral");
        Peripheral p;
        if (name == "CE-150") p = Peripheral::CE150;
        else if (name == "CE-1600P") p = Peripheral::CE1600P;
        else if (name == "CE-158") p = Peripheral::CE158;
        else return Refused{"There is no peripheral \"" + name + "\"."};
        return operation([this, p] { return togglePeripheralJob(p); });
    });
    setup.command(c::kMountHostDrive, [this](const Value& args, SessionContext&) -> CommandOutcome {
        const std::string dir = textArg(args, "dir");
        return operation([this, dir] { return mountHostDriveJob(dir); });
    });
    setup.command(c::kLoadBasic, [this](const Value& args, SessionContext&) -> CommandOutcome {
        const OpResult r = loadBasicProgram(bytesArg(args, "bytes").value_or(std::vector<std::uint8_t>{}), textArg(args, "path"));
        refreshTopics();
        if (!r.ok) return Refused{r.error};
        return Done{};
    });
    setup.command(c::kLoadMachineCode, [this](const Value& args, SessionContext&) -> CommandOutcome {
        MachineCodeLoadRequest req;
        req.payload = bytesArg(args, "bytes").value_or(std::vector<std::uint8_t>{});
        req.addr = static_cast<std::uint32_t>(integerArg(args, "addr"));
        req.slot = static_cast<int>(integerArg(args, "slot"));
        req.bank = static_cast<int>(integerArg(args, "bank"));
        const OpResult r = loadMachineCode(req);
        refreshTopics();
        if (!r.ok) return Refused{r.error};
        return Done{};
    });
}

void SharpAppPart::afterCall(sessions::SessionContext&, calcu::pacing::WallTime) {
    frameDuties();
    refreshTopics();
}

void SharpAppPart::machineChanged(sessions::SessionContext&) {
    // A machine the host built itself (the harness's start): this part
    // follows its model.
    if (const Machine* m = machine())
        if (auto model = modelOfId(m->descriptor().model.value)) m_model = *model;
    refreshTopics();
}

void SharpAppPart::operationEnded(sessions::SessionContext&) {
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

// ── Operations ──────────────────────────────────────────────────────────

sessions::CommandOutcome SharpAppPart::operation(std::function<job::Task<OpResult>()> make) {
    return sessions::StartOperation{[this, make](sessions::OperationContext&) { return runOperation(this, make); }};
}

sessions::Task<sessions::OperationResult> SharpAppPart::runOperation(SharpAppPart* self,
                                                                    std::function<job::Task<OpResult>()> make) {
    // Before an operation: pending card and disk writes go first.
    self->m_modules->flushPendingPersist();
    self->m_floppy->flushPendingPersist();
    const OpResult r = co_await make();
    co_return sessions::OperationResult{r.ok, r.error};
}

job::Task<OpResult> SharpAppPart::startJob(std::string modelOrEmpty, std::string presetOrEmpty) {
    const Model model = modelOrEmpty.empty() ? m_settings.startupModel()
                                             : modelFromSettingsName(modelOrEmpty).value_or(m_settings.startupModel());
    co_await switchModelJob(model);
    if (!presetOrEmpty.empty()) co_return co_await loadPresetJob(presetOrEmpty);
    co_return co_await defaultPresetJob();   // the model's default preset, if one is set
}

job::Task<OpResult> SharpAppPart::loadPresetJob(std::string ref, bool armOnly, bool userLoad) {
    const std::string path = m_paths.resolvePresetRef(ref);
    PresetFile preset;
    std::string parseError;
    if (!parsePresetFile(path, &preset, &parseError)) co_return OpResult::fail(parseError);
    preset.armOnly = armOnly;
    // L-A3 (LIB-10 §10.9, until LIB-6 1600 in M-46): a version 1 preset's
    // `trace:` through the library's trace session, limited and ended with
    // the operation (AC-43).
    preset.traceHook = [this](const std::string& name, std::string* error) {
        if (!m_ctx) return false;
        calcu::debugger::TraceSession& t = m_debugger.traceSession();
        if (name.empty()) {
            t.end(*m_ctx, calcu::debugger::TraceSession::Ending::Off);
            return true;
        }
        if (auto why = t.start(*m_ctx, name, /*button=*/false, /*ofOperation=*/true)) {
            *error = *why;
            return false;
        }
        return true;
    };

    // The ROM modules of the set (the CE-502B), then the bundled catalog,
    // then the save folder -- the same order the module bay uses.
    const std::string bundled = m_paths.bundledResourcesDir();
    const std::vector<std::string> extraModuleDirs = {m_paths.instanceDir()};
    // The folder of `screenshot:` steps (LIB-6 1600 moves them in M-46);
    // `trace:` steps go to the trace session (L-A3, below).
    const std::string traceDir = m_paths.traceDir().empty() ? m_paths.instanceDir() : m_paths.traceDir();
    const PresetLogFn presetLog = [this](const std::string& line) { log(line); };
    const PresetSaveAsFn onSaveAs = [this](const PresetSaveAsRequest& request, std::string* error) {
        switch (request.target) {
            case PresetStep::SaveAsTarget::Slot1:
                return m_modules->saveAsFromPreset(1, request.name, request.path, request.isTemplate, error,
                                                   request.writeProtect);
            case PresetStep::SaveAsTarget::Slot2:
                return m_modules->saveAsFromPreset(2, request.name, request.path, request.isTemplate, error,
                                                   request.writeProtect);
            case PresetStep::SaveAsTarget::Floppy:
                return m_floppy->saveAsFromPreset(request.name, request.path, request.isTemplate, error);
        }
        return false;
    };

    // P-A2 (LIB-8 AC-36, A-3): a preset the user loads starts the paper of
    // the plotter it plugs in empty; the other paper stays.
    if (userLoad && !preset.plotter.empty() && m_ctx) {
        const std::string owner = preset.plotter == "ce150" ? sharp::kCE150 : sharp::kCE1600P;
        if (calcu::paper::Paper* paper = m_paper.find(*m_ctx, owner).paper) paper->cut();
    }

    // The model switch is announced before the preset runs, so the armed
    // machine shows its model while it is still off. Core fires onArmed on
    // every arming outcome, so the bays are synced exactly once. The armed
    // state goes out with the call it happens in (LIB-3 AC-15, AC-38).
    const Model model = modelForPreset(preset);
    Machine& m = resetBareForPreset(
        model, preset.romVariant == "old" ? PC1600RomVersion::Old : PC1600RomVersion::New,
        preset.ce1600pRomVariant == "old" ? CE1600PRomVersion::Old : CE1600PRomVersion::New);
    finishPresetLoad(model);
    const auto onArmed = [this, pc1600 = preset.isPC1600()](const PresetLoadResult& armedSoFar) {
        m_modules->syncFromPresetLoad(1, armedSoFar.slot1ResolvedPath, armedSoFar.slot1RomFile);
        if (pc1600) {
            m_modules->syncFromPresetLoad(2, armedSoFar.slot2ResolvedPath, armedSoFar.slot2RomFile);
            m_floppy->syncFromPresetLoad(armedSoFar.floppyImageLabel, armedSoFar.floppyResolvedPath);
        } else {
            m_modules->syncFromPresetLoad(2);
        }
        if (Machine* armed = machine()) sharp::legacy::syncPlugged(*armed);   // the armed peripherals are seen now
        refreshTopics();
    };
    // The model's preset loader on the old class (adapter until M-46).
    const PresetLoadResult result = co_await sharp::legacy::applyPresetJob(
        m, preset, presetLog, traceDir, bundled, [this] { seedClockFromHost(); }, presetRoms(), extraModuleDirs, onArmed,
        onSaveAs);
    if (!result.ok) co_return OpResult::fail(result.error);
    co_return OpResult{};
}

job::Task<OpResult> SharpAppPart::loadDefaultPresetJob(std::string ref, Model model) {
    const std::string path = m_paths.resolvePresetRef(ref);
    PresetFile preset;
    std::string parseError;
    if (!parsePresetFile(path, &preset, &parseError)) co_return OpResult::fail(parseError);
    if (modelForPreset(preset) != model) {
        co_return OpResult::fail("The default preset \"" + presetDisplayName(preset.presetName, path) +
                                 "\" is for a different model -- change it in Settings.");
    }
    co_return co_await loadPresetJob(path, false, /*userLoad=*/false);
}

job::Task<OpResult> SharpAppPart::resetJob(bool allReset) {
    if (!machine()) co_return OpResult::fail("No machine is running.");
    co_await resetToPromptJob(allReset);
    co_return OpResult{};
}

job::Task<OpResult> SharpAppPart::selectModelJob(Model model) {
    if (model == m_model && machine()) co_return OpResult{};
    m_modules->flushPendingPersist();
    m_floppy->flushPendingPersist();
    m_modules->onModelChanged();
    // A model switch is a fresh default machine: A04 / new ROM, no plotter,
    // no modules (onModelChanged above), no floppy.
    resetRomSelectionsToDefault();
    co_await switchModelJob(model);
    m_floppy->selectDisk({});
    co_return OpResult{};
}

job::Task<OpResult> SharpAppPart::defaultPresetJob() {
    const Model model = m_model;
    const std::string path = m_settings.defaultPresetPath(model);
    if (path.empty()) co_return OpResult{};
    co_return co_await loadDefaultPresetJob(path, model);
}

job::Task<OpResult> SharpAppPart::pickModelJob(Model model) {
    if (model == m_model && machine()) co_return OpResult{};
    co_await selectModelJob(model);
    co_return co_await defaultPresetJob();
}

job::Task<OpResult> SharpAppPart::selectModuleJob(int slot, std::string moduleNameOrEmpty) {
    m_modules->selectModule(slot, moduleNameOrEmpty);
    // A module change is a detach and an attach in the slot, and the power
    // cycle that follows it (behaviour change 1600-4; LIB-2 AC-28).
    co_await powerCycleAroundJob([this, slot] {
        detachSlot(slot);
        m_modules->attachSlot(slot);
    });
    co_return OpResult{};
}

job::Task<OpResult> SharpAppPart::selectPC1500RomJob(PC1500RomRevision revision) {
    if (revision == m_pc1500RomRevision) co_return OpResult{};
    co_await setPC1500RomRevisionJob(revision);   // rebuilds a PC-1500
    co_return OpResult{};
}

job::Task<OpResult> SharpAppPart::selectPC1600RomJob(PC1600RomVersion version) {
    if (version == m_pc1600RomVersion) co_return OpResult{};
    co_await setPC1600RomVersionJob(version);   // rebuilds a PC-1600
    co_return OpResult{};
}

job::Task<OpResult> SharpAppPart::selectCE1600PRomJob(CE1600PRomVersion version) {
    if (version == m_ce1600pRomVersion) co_return OpResult{};
    if (!ce1600pAttached()) {
        setCE1600PRomVersion(version);
        co_return OpResult{};
    }
    co_await powerCycleAroundJob([this, version] {
        std::string error;
        if (!swapCE1600PRom(version, &error)) notice(attachFailedSentence("CE-1600P", error));
    });
    co_return OpResult{};
}

job::Task<OpResult> SharpAppPart::togglePeripheralJob(Peripheral p) {
    co_await powerCycleAroundJob([this, p] {
        std::string error;
        bool ok = true;
        switch (p) {
            case Peripheral::CE150:
                if (ce150Attached()) detachCE150();
                else ok = attachCE150(&error);
                break;
            case Peripheral::CE1600P:
                if (ce1600pAttached()) detachCE1600P();
                else ok = attachCE1600P(&error);
                break;
            case Peripheral::CE158:
                if (ce158Attached()) detachCE158();
                else ok = attachCE158(&error);
                break;
        }
        if (!ok) notice(attachFailedSentence(peripheralName(p), error));
    });
    co_return OpResult{};
}

job::Task<OpResult> SharpAppPart::mountHostDriveJob(std::string dirOrEmpty) {
    if (dirOrEmpty.empty()) {
        if (!hostDriveAttached()) co_return OpResult{};
        co_await powerCycleAroundJob([this] { detachHostDrive(); });
        co_return OpResult{};
    }
    std::string error;
    if (hostDriveAttached()) {
        if (!attachHostDrive(dirOrEmpty, &error)) co_return OpResult::fail(error);   // only the directory changes, live
        co_return OpResult{};
    }
    bool attached = false;
    co_await powerCycleAroundJob(
        [this, &attached, &error, dirOrEmpty] { attached = attachHostDrive(dirOrEmpty, &error); });
    if (!attached) co_return OpResult::fail(error);
    co_return OpResult{};
}

OpResult SharpAppPart::loadBasicProgram(const std::vector<std::uint8_t>& file, const std::string& path) {
    // Live-machine LOAD, like LOAD on the real machine: no reset, MODE
    // change or NEW0.
    Machine* m = machine();
    if (!m) return OpResult::fail("No machine is running.");
    const BasicLoadResult loaded = sharp::legacy::loadBasicProgram(*m, file);
    if (!loaded.ok) return OpResult::fail(path + ": " + loaded.error);
    return OpResult{};
}

OpResult SharpAppPart::loadMachineCode(const MachineCodeLoadRequest& request) {
    Machine* m = machine();
    if (!m) {
        return OpResult::fail(m_model == Model::PC1600 ? "No PC-1600 machine is running."
                                                       : "No PC-1500 machine is running.");
    }
    std::string error;
    const bool ok = sharp::legacy::loadMachineCode(*m, request.slot, request.addr, request.payload.data(),
                                                   request.payload.size(), &error, request.bank);
    return ok ? OpResult{} : OpResult::fail(error);
}

// ── The machine ─────────────────────────────────────────────────────────

void SharpAppPart::resetRomSelectionsToDefault() {
    m_pc1500RomRevision = PC1500RomRevision::A04;
    m_pc1600RomVersion = PC1600RomVersion::New;
    m_ce1600pRomVersion = CE1600PRomVersion::New;
}

void SharpAppPart::romMissing() {
    if (m_romsMissingTopic) m_romsMissingTopic->set(std::make_shared<RomsMissingPayload>(m_romsMissing));
    throw std::runtime_error(calcu::roms::describe(m_romsMissing));
}

job::Task<void> SharpAppPart::switchModelJob(Model model, bool keepPeripherals) {
    const Peripherals restore = keepPeripherals ? attachedPeripherals() : Peripherals{};
    busChange([this] { discardMachine(); });   // the old machine (and its disk) goes away
    m_model = model;
    // PC-1500A is A04-only (PC1500Variant.hpp).
    if (model == Model::PC1500A) m_pc1500RomRevision = PC1500RomRevision::A04;
    buildMachine(model);
    // The selected memory modules before the cold boot -- a module's state
    // must be visible on the very first ROM check -- then whatever was
    // attached before the rebuild.
    m_modules->attachAllToFreshMachine();
    reattach(restore);

    // Every rebuild boots flat out to the prompt (incl. a plotter's power-on
    // init). As in 0.8.1 only the PC-1600 gets a Cold boot.
    co_await resetToPromptJob(/*allReset=*/model == Model::PC1600);

    m_settings.setLastUsedModel(m_model);
    ++m_machineState.modelSwitches;
}

job::Task<void> SharpAppPart::setPC1600RomVersionJob(PC1600RomVersion version) {
    m_pc1600RomVersion = version;
    if (m_model == Model::PC1600) co_await switchModelJob(m_model, /*keepPeripherals=*/true);
}

job::Task<void> SharpAppPart::setPC1500RomRevisionJob(PC1500RomRevision revision) {
    m_pc1500RomRevision = revision;
    if (m_model != Model::PC1600) co_await switchModelJob(m_model, /*keepPeripherals=*/true);
}

void SharpAppPart::buildMachine(Model model) {
    if (!m_roms) romMissing();   // no machine without the full set (AC-5)
    BuildRequest req;
    req.model = ModelId{modelIdOf(model)};
    req.soundRate = sharp::kSoundRate;
    if (model == Model::PC1600) req.variant = VariantId{m_pc1600RomVersion == PC1600RomVersion::Old ? "Old" : "New"};
    else req.variant = VariantId{pc1500RomVariantName(m_pc1500RomRevision)};
    req.roms = m_roms->imagesFor(*m_catalogue.model(req.model), req.variant);
    sharp::SharpMachineBuilder builder;
    BuildResult built = build(m_catalogue, builder, req);
    // The set holds every file of every variant (AC-6): a refusal here is a
    // programming error.
    if (!built.machine) throw std::logic_error("the ROM set did not build " + req.model.value + ": " + built.refusal);
    installMachine(std::move(built.machine));
}

void SharpAppPart::installMachine(std::unique_ptr<Machine> m) {
    m_ms->setMachine(std::move(m), pacingProfile(m_model), sharp::kSoundRate);
    m_peripherals.hostLinks().machineChanged(*machine());   // the PC-1600's RS-232C gets its file now, before the boot
    ++m_machineState.builds;
}

bool SharpAppPart::peripheralImages(const PeripheralRequest& req, std::vector<RomImage>* out, std::string* error) const {
    const PeripheralDescriptor* d = m_catalogue.peripheral(req.peripheral);
    if (!d || d->romsFor(req.variant).empty()) return true;
    if (req.peripheral.value == sharp::kHostDrive) {
        if (!m_hostDriveRom) {
            if (error) *error = "the host drive's ROM " + d->roms.at(0).fileName + " is not bundled";
            return false;
        }
        out->push_back({d->roms.at(0).fileName, *m_hostDriveRom});
        return true;
    }
    if (!m_roms) {
        if (error) *error = calcu::roms::describe(m_romsMissing);
        return false;
    }
    *out = m_roms->imagesFor(*d, req.variant);
    return true;
}

Machine& SharpAppPart::resetBareForPreset(Model model, PC1600RomVersion version, CE1600PRomVersion ce1600pVersion) {
    if (model == Model::PC1600) {
        m_pc1600RomVersion = version;
        m_ce1600pRomVersion = ce1600pVersion;
    }
    discardMachine();
    const PC1500RomRevision kept = m_pc1500RomRevision;
    m_pc1500RomRevision = PC1500RomRevision::A04;   // the preset loader loads the preset's own ROM
    m_model = model;
    buildMachine(model);
    m_pc1500RomRevision = kept;
    return *machine();
}

void SharpAppPart::finishPresetLoad(Model model) {
    m_model = model;
    m_settings.setLastUsedModel(m_model);
    ++m_machineState.modelSwitches;
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

void SharpAppPart::discardMachine() {
    if (!m_ms || !m_ms->machine()) return;
    std::string tapeError;
    if (!tapeEject(&tapeError)) log("Tape: the tape recording could not be saved: " + tapeError);   // a recording goes to its file
    // A running trace ends at the new machine (the library's trace session, G11).
    m_ms->dropInput();   // EC-5: the keys it held come up, nothing waits
    m_ms->takeMachine();
    m_peripherals.hostLinks().machineGone();
}

void SharpAppPart::seedClockFromHost() {
    if (Machine* m = machine())
        calcu::peripherals::setClocksFromHost(*m, calcu::peripherals::hostDateTime(
                                                      m_ctx ? m_ctx->hostTime() : std::chrono::system_clock::now()));
}

job::Task<void> SharpAppPart::resetToPromptJob(bool allReset) {
    if (m_ms) m_ms->dropInput();   // EC-5
    Machine* m = machine();
    if (!m) co_return;
    if (allReset) m->coldBoot();
    else m->reset();
    // L-A2 (LIB-10 §10.9, until LIB-18 1600 in M-46): the restart is told to
    // the parts, so the run control resumes and an armed start looks afresh.
    if (m_ctx) m_ctx->tellRestarted(allReset ? sessions::Restart::ColdBoot : sessions::Restart::Reset);
    co_await sharp::legacy::runToPromptJob(*m);
}

job::Task<void> SharpAppPart::powerCycleAroundJob(std::function<void()> change) {
    if (!machine()) co_return;
    // A change on a running machine asks for a power cycle; on one that is
    // off, nothing follows (LIB-2 AC-4) and it stays off, as in 0.8.1.
    m_powerCycleFollows = false;
    change();
    Machine* m = machine();
    if (!m_powerCycleFollows || !m) co_return;
    m_ms->dropInput();   // EC-5
    m->powerCycle();
    co_await sharp::legacy::runToPromptJob(*m);
}

double SharpAppPart::clockHz() const {
    if (const Machine* m = machine()) return static_cast<double>(m->descriptor().tickRate);
    return static_cast<double>(sharp::sharpCatalogue().model(ModelId{modelIdOf(m_model)})->tickRate);
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

void SharpAppPart::attachSlotCard(int slot, std::unique_ptr<ExpansionCard> card) {
    Machine* m = machine();
    if (!m || !card || slot < 1 || slot > slotCount()) return;
    const std::string name = card->moduleName();
    if (!m_catalogue.peripheral(PeripheralId{name})) {
        if (auto* sd = dynamic_cast<const SoftwareDefinedCard*>(card.get())) {
            m_catalogue.addPeripheral(sharp::moduleDescriptor(sd->definition()));
        } else {
            // A card without a definition: it fits the slot it was made for.
            PeripheralDescriptor d;
            d.id = PeripheralId{name};
            d.name = name;
            d.fits = {m->descriptor().place(PlaceName{slotPlace(slot)})->pinOut};
            m_catalogue.addPeripheral(std::move(d));
        }
    }
    attachAt(slotPlace(slot), name.c_str(), std::nullopt, std::make_shared<sharp::ModuleCard>(std::move(card)), nullptr);
}

void SharpAppPart::detachSlot(int slot) {
    if (!machine() || slot < 1 || slot > slotCount()) return;
    const std::string id = pluggedAt(slotPlace(slot));
    if (!id.empty()) detachAt(slotPlace(slot), id.c_str());
}

void SharpAppPart::detachSlots() {
    for (int slot = 1; slot <= slotCount(); ++slot) detachSlot(slot);
}


// ---- Cassette recorder ----

bool SharpAppPart::tapeInterfaceAttached() const {
    const std::string printer = pluggedAt(sharp::kPrinter);
    return isPC1600() ? printer == sharp::kCE1600P : printer == sharp::kCE150;
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

template <class K> K* SharpAppPart::kindOf(const char* peripheral) const {
    Machine* m = machine();
    if (!m) return nullptr;
    for (const auto& e : m->capabilities(K::kKind))
        if (const auto* p = std::get_if<PeripheralOwner>(&e.owner); p && p->peripheral.value == peripheral)
            return capabilityAs<K>(*m, e.id);
    return nullptr;
}

sharp::FloppyDriveAccess* SharpAppPart::floppyDrive() const { return kind<sharp::FloppyDriveAccess>(); }

void SharpAppPart::flushFloppyBay() {
    if (m_floppy) m_floppy->flushPendingPersist();
}

void SharpAppPart::insertSelectedDisk() {
    if (m_floppy) m_floppy->insertSelectedDisk();
}

void SharpAppPart::noteChange(const ChangeResult& r) {
    if (r.follows == AfterChange::PowerCycle) m_powerCycleFollows = true;
}

bool SharpAppPart::attachAt(const char* place, const char* peripheral, std::optional<VariantId> variant,
                            std::shared_ptr<const ProductData> content, std::string* error) {
    Machine* m = machine();
    if (!m) return false;
    PeripheralRequest req{PlacePath{{PlaceName{place}}}, PeripheralId{peripheral}, std::move(variant), {}, std::move(content), {}};
    if (!peripheralImages(req, &req.roms, error)) return false;
    const ChangeResult r = m->attach(std::move(req), m_catalogue);
    if (r.refused) {
        if (error) *error = r.refused->reason;
        return false;
    }
    noteChange(r);
    m_peripherals.hostLinks().machineChanged(*m);
    return true;
}

void SharpAppPart::detachAt(const char* place, const char* peripheral) {
    Machine* m = machine();
    if (!m) return;
    noteChange(m->detach(PlacePath{{PlaceName{place}}}, PeripheralId{peripheral}, m_catalogue));
    m_peripherals.hostLinks().machineChanged(*m);
}

SharpAppPart::Peripherals SharpAppPart::attachedPeripherals() const {
    return {ce150Attached(), ce1600pAttached(), ce158Attached(), hostDriveDirectory()};
}

void SharpAppPart::reattach(const Peripherals& p) {
    // The order the user can build it up in; what the model can't take is
    // refused (CE-1600P and host drive: PC-1600 only).
    if (p.ce150) attachCE150();
    if (p.ce1600p) attachCE1600P();
    if (p.ce158) attachCE158();
    if (!p.hostDriveDir.empty()) attachHostDrive(p.hostDriveDir);
}

bool SharpAppPart::attachCE150(std::string* error) { return attachAt(sharp::kPrinter, sharp::kCE150, std::nullopt, nullptr, error); }

void SharpAppPart::detachCE150() { detachAt(sharp::kPrinter, sharp::kCE150); }

bool SharpAppPart::ce150Attached() const { return pluggedAt(sharp::kPrinter) == sharp::kCE150; }

bool SharpAppPart::attachCE158(std::string* error) {
    return attachAt(sharp::kInterface, sharp::kCE158, std::nullopt, nullptr, error);
}

void SharpAppPart::detachCE158() { detachAt(sharp::kInterface, sharp::kCE158); }

bool SharpAppPart::ce158Attached() const { return pluggedAt(sharp::kInterface) == sharp::kCE158; }

std::vector<std::uint8_t> SharpAppPart::drainCE158PrinterOutput() {
    auto* port = kind<sharp::Ce158PrinterPort>();
    return port ? port->drain() : std::vector<std::uint8_t>{};
}

namespace {

VariantId ce1600pVariant(CE1600PRomVersion v) { return VariantId{v == CE1600PRomVersion::Old ? "Old" : "New"}; }

} // namespace

bool SharpAppPart::attachCE1600P(std::string* error) {
    if (!isPC1600()) return false;
    if (!attachAt(sharp::kPrinter, sharp::kCE1600P, ce1600pVariant(m_ce1600pRomVersion), nullptr, error)) return false;
    // The disk goes in when the CE-1600F is on the bus: at the off point of
    // the power cycle that follows, as in 0.8.1 (adapter, M-47).
    sharp::legacy::atOffPoint(*machine(), [this] { insertSelectedDisk(); });
    return true;
}

void SharpAppPart::detachCE1600P() {
    if (isPC1600()) busChange([this] { detachAt(sharp::kPrinter, sharp::kCE1600P); });
}

bool SharpAppPart::swapCE1600PRom(CE1600PRomVersion version, std::string* error) {
    if (!ce1600pAttached()) return false;
    sharp::FloppyDriveAccess* drive = floppyDrive();
    const int side = drive ? drive->side() : 0;
    flushFloppyBay();
    setCE1600PRomVersion(version);
    PeripheralRequest req{PlacePath{{PlaceName{sharp::kPrinter}}}, PeripheralId{sharp::kCE1600P}, ce1600pVariant(version), {},
                          nullptr, {}};
    if (!peripheralImages(req, &req.roms, error)) return false;
    // One replace: the other variant in the same place, one power cycle
    // (C1600-1 AC-18, design D-5).
    Machine* m = machine();
    const ChangeResult r = m->replace(PeripheralId{sharp::kCE1600P}, std::move(req), m_catalogue);
    if (r.refused) {
        if (error) *error = r.refused->reason;
        return false;
    }
    noteChange(r);
    // The disk moves over with the same side up (adapter, M-47).
    sharp::legacy::atOffPoint(*m, [this, side] {
        insertSelectedDisk();   // re-inserts the selected disk, side A up
        if (sharp::FloppyDriveAccess* d = floppyDrive(); d && side != 0 && d->hasDisk()) d->setSide(side);
    });
    return true;
}

bool SharpAppPart::ce1600pAttached() const { return pluggedAt(sharp::kPrinter) == sharp::kCE1600P; }

bool SharpAppPart::attachHostDrive(const std::string& dir, std::string* error) {
    if (!isPC1600()) return false;
    if (auto* folder = kind<sharp::HostDriveFolder>()) {
        folder->setFolder(dir);   // only the directory changes, live
        return true;
    }
    return attachAt(sharp::kHostDrivePlace, sharp::kHostDrive, std::nullopt, std::make_shared<sharp::HostDriveRequest>(dir),
                    error);
}

std::string SharpAppPart::hostDriveDirectory() const {
    auto* folder = kind<sharp::HostDriveFolder>();
    return folder ? folder->folder() : std::string();
}

void SharpAppPart::detachHostDrive() { detachAt(sharp::kHostDrivePlace, sharp::kHostDrive); }

bool SharpAppPart::hostDriveAttached() const { return pluggedAt(sharp::kHostDrivePlace) == sharp::kHostDrive; }

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
