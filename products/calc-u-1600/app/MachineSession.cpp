#include "app/MachineSession.hpp"

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
#include "Machine/SharpRomImages.hpp"
#include "PC1500/PC1500TypedInput.hpp"
#include "PC1600/PC1600TypedInput.hpp"
#include "app/Utf8Path.hpp"
#include "peripherals/Controls.hpp"
#include "peripherals/RealTimeClock.hpp"
#include "peripherals/SerialPort.hpp"

namespace shared {

using namespace calcu::machine;

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

const char* slotPlace(int slot) { return slot == 2 ? sharp::kSlot2 : sharp::kSlot1; }

// The key ID of a key name of the typing helpers: they give a letter as its
// capital (PC1500TypedInput.hpp, PC1600TypedInput.hpp, which the old
// classes took); the machine takes exactly its key IDs (C1600-1 AC-9, EC-1).
KeyId keyIdOf(const std::string& name) {
    if (name.size() == 1 && name[0] >= 'A' && name[0] <= 'Z') return KeyId{std::string(1, static_cast<char>(name[0] - 'A' + 'a'))};
    return KeyId{name};
}

// releaseKey()'s minimum hold for a frontend's key: a press of one frame
// is scanned on both models; two leave a margin without making a fast
// typist's keys overlap.
constexpr int kMinKeyHoldFrames = 2;

// tapShiftedKey(): 30 ms shift hold, 100 ms gap, 30 ms base-key hold.
constexpr double kTapHoldSeconds = 0.030;
constexpr double kTapGapSeconds = 0.100;

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

} // namespace

MachineSession::MachineSession(Settings& settings, const AppPaths& paths, Listener& listener, Bays* bays)
    : m_settings(settings),
      m_paths(paths),
      m_listener(listener),
      m_bays(bays),
      m_catalogue(sharp::sharpCatalogue()),
      m_hostTime([] { return std::chrono::system_clock::now(); }),
      m_hostLinks(serialFileFor, folderOrNone(paths.serialLinkDir())) {}

MachineSession::~MachineSession() = default;

void MachineSession::resetRomSelectionsToDefault() {
    m_pc1500RomRevision = PC1500RomRevision::A04;
    m_pc1600RomVersion = PC1600RomVersion::New;
    m_ce1600pRomVersion = CE1600PRomVersion::New;
}

void MachineSession::romMissing(const std::string& error) {
    m_listener.romMissing(error, m_paths.bundledResourcesDir());
    throw std::runtime_error(error);
}

void MachineSession::switchModel(Model model, bool keepPeripherals) {
    job::runToCompletion(switchModelJob(model, keepPeripherals));
}

job::Task<void> MachineSession::switchModelJob(Model model, bool keepPeripherals) {
    const Peripherals restore = keepPeripherals ? attachedPeripherals() : Peripherals{};
    busChange([this] { discardMachine(); });  // the old machine (and its disk) goes away
    m_model = model;
    // PC-1500A is A04-only (PC1500Variant.hpp) -- clamp regardless of
    // whatever revision was last picked for the plain PC-1500.
    if (model == Model::PC1500A) m_pc1500RomRevision = PC1500RomRevision::A04;
    buildMachine(model);
    wireNewMachine();
    // Attach any currently-selected memory modules before the cold boot --
    // a module's state must be visible on the very first ROM check -- then
    // whatever was attached before the rebuild.
    if (m_bays) m_bays->attachModulesToFreshMachine();
    reattach(restore);

    // Every rebuild boots flat out to the prompt (incl. a plotter's power-on
    // init); the caller restarts paced emulation after it, which sets the
    // clock from the host (see seedClockFromHost()). As in 0.8.1 only the
    // PC-1600 gets a Cold boot: a fresh PC-1500 is Reset (its RAM is a
    // freshly built machine's either way).
    co_await resetToPromptJob(/*allReset=*/model == Model::PC1600);
    discardAudio(); // whatever the flat-out boot beeped is stale

    m_settings.setLastUsedModel(static_cast<int>(m_model));
    m_listener.modelSwitched(m_model);
}

void MachineSession::setPC1600RomVersion(PC1600RomVersion version) {
    job::runToCompletion(setPC1600RomVersionJob(version));
}

job::Task<void> MachineSession::setPC1600RomVersionJob(PC1600RomVersion version) {
    m_pc1600RomVersion = version;
    if (m_model == Model::PC1600) co_await switchModelJob(m_model, /*keepPeripherals=*/true); // rebuild with the new ROM
}

void MachineSession::setPC1500RomRevision(PC1500RomRevision revision) {
    job::runToCompletion(setPC1500RomRevisionJob(revision));
}

job::Task<void> MachineSession::setPC1500RomRevisionJob(PC1500RomRevision revision) {
    m_pc1500RomRevision = revision;
    if (m_model != Model::PC1600) co_await switchModelJob(m_model, /*keepPeripherals=*/true); // rebuild with the new ROM
}

std::vector<std::string> MachineSession::bundledRomDirs() const { return {m_paths.bundledResourcesDir()}; }

void MachineSession::buildMachine(Model model) {
    sharp::SharpMachineBuilder builder;
    for (;;) {
        BuildRequest req;
        req.model = ModelId{modelIdOf(model)};
        req.soundRate = sharp::kSoundRate;
        if (model == Model::PC1600) req.variant = VariantId{m_pc1600RomVersion == PC1600RomVersion::Old ? "Old" : "New"};
        else req.variant = VariantId{pc1500RomVariantName(m_pc1500RomRevision)};
        std::string err;
        bool ok = sharp::modelRomImages(m_catalogue, req.model, req.variant, bundledRomDirs(), &req.roms, &err);
        if (ok) {
            BuildResult built = build(m_catalogue, builder, req);
            if (built.machine) {
                m_machine = std::move(built.machine);
                return;
            }
            err = built.refusal;
        }
        // Only the old PC-1600 ROM set is optional (its dump may be missing
        // or incomplete): fall back to the new ROM and retry once -- a
        // new-ROM failure ends the session.
        if (model != Model::PC1600 || m_pc1600RomVersion != PC1600RomVersion::Old) romMissing(err);
        m_listener.notice(Notice::OldPC1600RomUnavailable, err);
        m_pc1600RomVersion = PC1600RomVersion::New;
    }
}

Machine& MachineSession::resetBareForPreset(Model model, PC1600RomVersion version, CE1600PRomVersion ce1600pVersion) {
    if (model == Model::PC1600) {
        m_pc1600RomVersion = version;
        m_ce1600pRomVersion = ce1600pVersion;
    }
    discardMachine();
    const PC1500RomRevision kept = m_pc1500RomRevision;
    m_pc1500RomRevision = PC1500RomRevision::A04;   // the preset loader loads the preset's own ROM
    buildMachine(model);
    m_pc1500RomRevision = kept;
    wireNewMachine(); // a preset's `interface: CE-158` gets its host link from here
    return *m_machine;
}

void MachineSession::finishPresetLoad(Model model) {
    m_model = model;
    m_settings.setLastUsedModel(static_cast<int>(m_model));
    m_listener.modelSwitched(m_model);
}

void MachineSession::refreshSerialLinkDirectory() {
    m_hostLinks.moveTo(folderOrNone(m_paths.serialLinkDir()));
    m_listener.serialLinksRelinked();
}

namespace {

std::string linkPath(const calcu::peripherals::HostLinks& links, const char* file) {
    for (const auto& s : links.status())
        if (s.fileName == file && s.open) return s.path;
    return {};
}

} // namespace

std::string MachineSession::serialLinkStatus() const { return linkPath(m_hostLinks, "calcu1600-rs232c.serial"); }

std::string MachineSession::ce158SerialLinkStatus() const { return linkPath(m_hostLinks, "calcu1600-ce158.serial"); }

void MachineSession::pumpSerialLinks() {
    if (m_machine) m_hostLinks.pump(*m_machine);
}

void MachineSession::wireNewMachine() {
    m_hostLinks.machineChanged(*m_machine);   // the PC-1600's RS-232C gets its file now, before the boot
    m_listener.machineReplaced();
}

void MachineSession::discardMachine() {
    m_listener.machineAboutToChange();
    std::string tapeError;
    if (!tapeEject(&tapeError)) m_listener.notice(Notice::TapeSaveFailed, tapeError); // a recording goes to its file
    endTrace();
    m_paste.cancel({}); // the machine it was typing into is going away
    m_liveTyping = false;
    m_timedKeys.clear();
    m_keyPressedAt.clear();
    m_machine.reset();
    m_hostLinks.machineGone();
}

void MachineSession::seedClockFromHost() {
    if (m_machine) calcu::peripherals::setClocksFromHost(*m_machine, calcu::peripherals::hostDateTime(m_hostTime()));
}

void MachineSession::resetToPrompt(bool allReset) { job::runToCompletion(resetToPromptJob(allReset)); }

job::Task<void> MachineSession::resetToPromptJob(bool allReset) {
    cancelPaste();
    if (!m_machine) co_return;
    if (allReset) m_machine->coldBoot();
    else m_machine->reset();
    co_await sharp::legacy::runToPromptJob(*m_machine);
}

void MachineSession::powerCycleAround(const std::function<void()>& change) {
    job::runToCompletion(powerCycleAroundJob(change));
}

job::Task<void> MachineSession::powerCycleAroundJob(std::function<void()> change) {
    if (!m_machine) co_return;
    // A change on a running machine asks for a power cycle; on one that is
    // off, nothing follows (LIB-2 AC-4) and it stays off, as in 0.8.1.
    m_powerCycleFollows = false;
    change();
    if (!m_powerCycleFollows || !m_machine) co_return;
    cancelPaste();
    m_machine->powerCycle();
    co_await sharp::legacy::runToPromptJob(*m_machine);
}

// ---- Keys and typing ----

void MachineSession::pressKey(const std::string& name) {
    // A release still to come (a fast typist: released for real, held on
    // for the minimum -- this key's own too, a quick double tap): this
    // press waits for it and a gap, so two keys never overlap (the ROM
    // drops one) and a double tap stays two taps.
    if (!m_timedKeys.empty()) {
        const auto at = m_timedKeys.back().due + minKeyHoldCycles();
        m_keyPressedAt[name] = at;
        scheduleKeyAt(at, true, name);
        return;
    }
    m_keyPressedAt[name] = m_budgetClock;
    machinePressKey(name);
}

std::uint64_t MachineSession::minKeyHoldCycles() const {
    return static_cast<std::uint64_t>(kMinKeyHoldFrames * clockHz() / 60.0);
}

void MachineSession::releaseKey(const std::string& name) {
    const auto it = m_keyPressedAt.find(name);
    if (it != m_keyPressedAt.end()) {
        const auto holdUntil = it->second + minKeyHoldCycles();
        m_keyPressedAt.erase(it);
        if (m_budgetClock < holdUntil) {
            scheduleKeyAt(holdUntil, false, name);
            return;
        }
    }
    machineReleaseKey(name);
}

// A key ID the model lacks is refused by the machine and changes nothing
// (C1600-1 EC-1); the frontends name only the model's keys.
void MachineSession::machinePressKey(const std::string& name) {
    if (m_machine) m_machine->keyDown(keyIdOf(name));
}

void MachineSession::machineReleaseKey(const std::string& name) {
    if (m_machine) m_machine->keyUp(keyIdOf(name));
}

void MachineSession::setOnKeyPressed(bool pressed) {
    if (pressed) machinePressKey("on");
    else machineReleaseKey("on");
}

void MachineSession::scheduleKey(double afterSeconds, bool press, const std::string& name) {
    scheduleKeyAt(m_budgetClock + static_cast<std::uint64_t>(afterSeconds * clockHz()), press, name);
}

void MachineSession::scheduleKeyAt(std::uint64_t due, bool press, const std::string& name) {
    // Keep the queue in due order (taps can overlap).
    auto it = std::upper_bound(m_timedKeys.begin(), m_timedKeys.end(), due,
                               [](std::uint64_t d, const TimedKey& k) { return d < k.due; });
    m_timedKeys.insert(it, TimedKey{due, press, name});
}

void MachineSession::fireDueKeys() {
    while (!m_timedKeys.empty() && m_timedKeys.front().due <= m_budgetClock) {
        const TimedKey key = m_timedKeys.front();
        m_timedKeys.pop_front();
        if (key.press) machinePressKey(key.name);
        else machineReleaseKey(key.name);
    }
}

void MachineSession::tapShiftedKey(const std::string& baseName) {
    machinePressKey("shift");
    scheduleKey(kTapHoldSeconds, false, "shift");
    scheduleKey(kTapHoldSeconds + kTapGapSeconds, true, baseName);
    scheduleKey(kTapHoldSeconds + kTapGapSeconds + kTapHoldSeconds, false, baseName);
}

void MachineSession::tapKey(const std::string& name) {
    if (!m_machine) return;
    if (auto* queue = kind<sharp::KeyQueue>()) {   // the PC-1500's
        queue->enqueue(keyIdOf(name).value);
    } else {
        machinePressKey(name);
        scheduleKey(kTapHoldSeconds, false, name);
    }
}

void MachineSession::enqueueKey(const std::string& name) {
    // PC-1600 buffering is out of scope for now -- silently ignored,
    // matching pressKey()/releaseKey()'s convention for a name/model
    // combination that doesn't apply.
    if (auto* queue = kind<sharp::KeyQueue>()) queue->enqueue(keyIdOf(name).value);
}

void MachineSession::enqueueShiftedKey(const std::string& baseName) {
    enqueueKey("shift");
    enqueueKey(baseName);
}

bool MachineSession::hostKeyDown(const ResolvedKey& key, bool physical) {
    // A real machine keystroke aborts a paste still typing -- an easy way
    // out of a long one, and it keeps the two from fighting over the key
    // matrix.
    if (pasteActive()) interruptPaste();

    if (m_model == Model::PC1600) {
        // An accented character is a KBII key sequence, and a key typed
        // while one is still running must wait behind it (KBII is still
        // latched) -- both go through the frame-paced queue. Everything
        // else is unbuffered.
        if (key.needsKbii || liveTypingActive()) {
            typeLiveStep(PasteStep{key.baseKey, key.needsShift, key.needsKbii});
        } else if (key.needsShift) {
            tapShiftedKey(key.baseKey);  // self-contained: nothing to release
        } else if (physical) {
            pressKey(key.baseKey);
            return true;
        } else {
            tapKey(key.baseKey);
        }
        return false;
    }

    // PC-1500: cursor keys bypass the queue so the ROM's own auto-repeat
    // can engage on a genuine physical hold; everything else (shifted keys
    // included) is queued so fast typing can't outrun the key-scan loop.
    if (key.needsShift) {
        enqueueShiftedKey(key.baseKey);
    } else if (physical && key.isPc1500RepeatKey()) {
        pressKey(key.baseKey);
        return true;
    } else {
        enqueueKey(key.baseKey);
    }
    return false;
}

void MachineSession::hostShiftTap() {
    if (pasteActive()) interruptPaste();
    if (liveTypingActive()) typeLiveStep(PasteStep{"shift"});
    else tapKey("shift");
}

void MachineSession::pasteText(const std::string& text) { enqueueTyped(text, /*pressEnter=*/false); }

void MachineSession::typeCommand(const std::string& line) { enqueueTyped(line, /*pressEnter=*/true); }

void MachineSession::enqueueTyped(const std::string& text, bool pressEnter) {
    if (!m_machine) return;
    if (!m_paste.active()) m_pasteFrameCycles = 0;
    const bool pc1600 = isPC1600();
    std::vector<PasteStep> steps = pc1600 ? buildPasteSteps(text, pc1600ResolveTypedChar, pc1600ResolveKbiiChar)
                                          : buildPasteSteps(text, pc1500ResolveTypedChar);
    if (pressEnter) steps.push_back(PasteStep{"enter", false});
    m_paste.setPacing(pc1600 ? pc1600PastePacing() : pc1500PastePacing());
    m_paste.append(steps);
    m_liveTyping = false;
}

void MachineSession::typeLiveStep(const PasteStep& step) {
    if (!isPC1600()) return;
    if (!m_paste.active()) m_pasteFrameCycles = 0;
    m_paste.setPacing(pc1600PastePacing());
    m_paste.append({step});
    m_liveTyping = true;
}

void MachineSession::cancelPaste() {
    m_paste.cancel([this](const std::string& key) { machineReleaseKey(key); });
    m_liveTyping = false;
}

void MachineSession::interruptPaste() {
    m_paste.cancel([this](const std::string& key) { machineReleaseKey(key); }, /*finishKbii=*/true);
    m_liveTyping = m_paste.active(); // what's left is a KBII sequence's closing tap
}

// ---- Running ----

void MachineSession::runActive(std::uint64_t cycles) {
    if (!m_machine) return;
    if (!m_listener.runDebuggerSlice(cycles)) m_machine->advance(cycles);
    m_hostLinks.pump(*m_machine);
}

void MachineSession::pasteOnFrame() {
    m_paste.onFrame([this](const std::string& key) { machinePressKey(key); },
                    [this](const std::string& key) { machineReleaseKey(key); },
                    [this] { return m_machine ? sharp::legacy::readLatches(*m_machine) : KeyLatches{}; });
}

void MachineSession::advance(std::uint64_t cyclesBudget) {
    m_budgetClock += cyclesBudget;
    // While a paste is typing, run in 60 Hz emulated frames and let the
    // feeder act at each frame boundary (its cadence is counted in frames,
    // so it holds under wall-clock pacing and turbo alike).
    const auto frameCycles = static_cast<std::uint64_t>(clockHz() / 60.0);
    while (m_paste.active() && cyclesBudget > 0 && frameCycles > 0) {
        const std::uint64_t chunk = std::min(cyclesBudget, frameCycles - m_pasteFrameCycles);
        runActive(chunk);
        cyclesBudget -= chunk;
        m_pasteFrameCycles += chunk;
        if (m_pasteFrameCycles >= frameCycles) {
            m_pasteFrameCycles = 0;
            pasteOnFrame();
        }
    }
    if (cyclesBudget > 0) runActive(cyclesBudget);
    fireDueKeys();
}

double MachineSession::clockHz() const {
    if (m_machine) return static_cast<double>(m_machine->descriptor().tickRate);
    return static_cast<double>(sharp::sharpCatalogue().model(ModelId{modelIdOf(m_model)})->tickRate);
}

bool MachineSession::isMachinePoweredOn() const { return m_machine && m_machine->readDisplay().on; }

// ---- Output ----

DisplayFrame MachineSession::currentDisplay() const {
    DisplayFrame frame;
    if (!m_machine) return frame;
    // The display output as today's frame (adapter until M-49, LIB-14): every
    // status symbol in kStatusSymbolNames order, the ones the model lacks off.
    const DisplayOutput out = m_machine->readDisplay();
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

bool MachineSession::lcdText(LcdText* out) const {
    if (!m_machine) return false;
    *out = displayText(*m_machine);
    return true;
}

GrayImage MachineSession::currentScreenImage() const {
    if (!m_machine) return GrayImage{};
    return renderLcdImage(lcdBitmapOf(m_machine->readDisplay()), isPC1600() ? kPC1600ScreenMm : kPC1500ScreenMm);
}

std::size_t MachineSession::drainAudio(std::int16_t* out, std::size_t max) {
    return m_machine ? m_machine->takeSound(std::span<std::int16_t>(out, max)) : 0;
}

void MachineSession::discardAudio() {
    if (!m_machine) return;
    std::int16_t scratch[4096];
    while (m_machine->takeSound(scratch) > 0) {
    }
}

// ---- Debug panel ----

std::vector<std::string> MachineSession::inspectorView(inspect::View view) const {
    auto* k = kind<sharp::InspectorViews>();
    return k ? k->view(view) : std::vector<std::string>{};
}

std::vector<inspect::MenuEntry> MachineSession::inspectorMenu(bool dumps) const {
    auto* k = kind<sharp::InspectorViews>();
    return k ? k->menu(dumps) : std::vector<inspect::MenuEntry>{};
}

std::vector<std::uint8_t> MachineSession::debugSlotCardImage(int slot, int* bankCount) const {
    *bankCount = -1;
    auto* k = kind<sharp::InspectorViews>();
    return k ? k->slotImage(slot, bankCount) : std::vector<std::uint8_t>{};
}

// ---- Expansion slots ----

int MachineSession::slotCount() const {
    if (!m_machine) return 0;
    int n = 0;
    for (const auto& p : m_machine->descriptor().places)
        if (p.name.value == sharp::kSlot1 || p.name.value == sharp::kSlot2) ++n;
    return n;
}

const ExpansionCard* MachineSession::slotCard(int slot) const {
    return m_machine ? sharp::legacy::moduleIn(*m_machine, slot) : nullptr;
}

void MachineSession::setSlotWriteProtected(int slot, bool protect) {
    if (!m_machine) return;
    const Owner owner = PeripheralOwner{PeripheralId{pluggedAt(slotPlace(slot))}, PlacePath{{PlaceName{slotPlace(slot)}}}};
    if (auto* controls = calcu::peripherals::controlsCapability(*m_machine, owner))
        controls->set(ControlId{"write-protect"}, protect);
}

void MachineSession::attachSlotCard(int slot, std::unique_ptr<ExpansionCard> card) {
    if (!m_machine || !card || slot < 1 || slot > slotCount()) return;
    const std::string name = card->moduleName();
    if (!m_catalogue.peripheral(PeripheralId{name})) {
        if (auto* sd = dynamic_cast<const SoftwareDefinedCard*>(card.get())) {
            m_catalogue.addPeripheral(sharp::moduleDescriptor(sd->definition()));
        } else {
            // A card without a definition: it fits the slot it was made for.
            PeripheralDescriptor d;
            d.id = PeripheralId{name};
            d.name = name;
            d.fits = {m_machine->descriptor().place(PlaceName{slotPlace(slot)})->pinOut};
            m_catalogue.addPeripheral(std::move(d));
        }
    }
    attachAt(slotPlace(slot), name.c_str(), std::nullopt, std::make_shared<sharp::ModuleCard>(std::move(card)), nullptr);
}

void MachineSession::detachSlot(int slot) {
    if (!m_machine || slot < 1 || slot > slotCount()) return;
    const std::string id = pluggedAt(slotPlace(slot));
    if (!id.empty()) detachAt(slotPlace(slot), id.c_str());
}

void MachineSession::detachSlots() {
    for (int slot = 1; slot <= slotCount(); ++slot) detachSlot(slot);
}

bool MachineSession::beginTrace(const std::string& path) {
    auto* trace = kind<sharp::CpuTrace>();
    if (!trace) return false;
    std::FILE* handle = std::fopen(toPath(path).string().c_str(), "wb");
    if (!handle) return false;
    const bool ok = trace->begin(handle, TRACE_FULL);
    if (!ok) std::fclose(handle); // a capture was already active; ownership passes only on success
    return ok;
}

void MachineSession::endTrace() {
    if (auto* trace = kind<sharp::CpuTrace>()) trace->end();
}

std::uint64_t MachineSession::traceBytes() const {
    auto* trace = kind<sharp::CpuTrace>();
    return trace ? trace->bytes() : 0;
}

bool MachineSession::traceActive() const {
    auto* trace = kind<sharp::CpuTrace>();
    return trace && trace->active();
}

// ---- Cassette recorder ----

bool MachineSession::tapeInterfaceAttached() const {
    const std::string printer = pluggedAt(sharp::kPrinter);
    return isPC1600() ? printer == sharp::kCE1600P : printer == sharp::kCE150;
}

bool MachineSession::tapePlay(const std::string& path, std::string* error) {
    auto* tape = kind<sharp::TapeDeckAccess>();
    std::string err;
    const bool ok = tape && tape->play(path, err);
    if (!ok && error) *error = err;
    return ok;
}

void MachineSession::tapeRecord(const std::string& path) {
    if (auto* tape = kind<sharp::TapeDeckAccess>()) tape->record(path);
}

bool MachineSession::tapeEject(std::string* error) {
    auto* tape = kind<sharp::TapeDeckAccess>();
    if (!tape) return true;
    std::string err;
    const bool ok = tape->eject(&err);
    if (!ok && error) *error = err;
    return ok;
}

TapeDeck::Status MachineSession::tapeStatus() const {
    auto* tape = kind<sharp::TapeDeckAccess>();
    return tape ? tape->status() : TapeDeck::Status{};
}

std::string MachineSession::tapeTakeLastError() {
    auto* tape = kind<sharp::TapeDeckAccess>();
    return tape ? tape->takeLastError() : std::string();
}

// ---- Peripherals ----

std::string MachineSession::pluggedAt(const char* place) const {
    if (!m_machine) return {};
    for (const auto& e : m_machine->plugged().byPlace)
        if (e.place.steps.size() == 1 && e.place.steps[0].value == place) return e.peripheral.value;
    return {};
}

template <class K> K* MachineSession::kindOf(const char* peripheral) const {
    if (!m_machine) return nullptr;
    for (const auto& e : m_machine->capabilities(K::kKind))
        if (const auto* p = std::get_if<PeripheralOwner>(&e.owner); p && p->peripheral.value == peripheral)
            return capabilityAs<K>(*m_machine, e.id);
    return nullptr;
}

sharp::FloppyDriveAccess* MachineSession::floppyDrive() const { return kind<sharp::FloppyDriveAccess>(); }

void MachineSession::noteChange(const ChangeResult& r) {
    if (r.follows == AfterChange::PowerCycle) m_powerCycleFollows = true;
}

bool MachineSession::attachAt(const char* place, const char* peripheral, std::optional<VariantId> variant,
                              std::shared_ptr<const ProductData> content, std::string* error) {
    if (!m_machine) return false;
    PeripheralRequest req{PlacePath{{PlaceName{place}}}, PeripheralId{peripheral}, std::move(variant), {}, std::move(content), {}};
    std::string err;
    if (m_catalogue.peripheral(req.peripheral) && !m_catalogue.peripheral(req.peripheral)->romsFor(req.variant).empty() &&
        !sharp::peripheralRomImages(m_catalogue, req.peripheral, req.variant, bundledRomDirs(), &req.roms, &err)) {
        if (error) *error = err;
        return false;
    }
    const ChangeResult r = m_machine->attach(std::move(req), m_catalogue);
    if (r.refused) {
        if (error) *error = r.refused->reason;
        return false;
    }
    noteChange(r);
    m_hostLinks.machineChanged(*m_machine);
    return true;
}

void MachineSession::detachAt(const char* place, const char* peripheral) {
    if (!m_machine) return;
    noteChange(m_machine->detach(PlacePath{{PlaceName{place}}}, PeripheralId{peripheral}, m_catalogue));
    m_hostLinks.machineChanged(*m_machine);
}

MachineSession::Peripherals MachineSession::attachedPeripherals() const {
    return {ce150Attached(), ce1600pAttached(), ce158Attached(), hostDriveDirectory()};
}

void MachineSession::reattach(const Peripherals& p) {
    // The order the user can build it up in; what the model can't take is
    // refused (CE-1600P and host drive: PC-1600 only).
    if (p.ce150) attachCE150();
    if (p.ce1600p) attachCE1600P();
    if (p.ce158) attachCE158();
    if (!p.hostDriveDir.empty()) attachHostDrive(p.hostDriveDir);
}

bool MachineSession::attachCE150(std::string* error) {
    return attachAt(sharp::kPrinter, sharp::kCE150, std::nullopt, nullptr, error);
}

void MachineSession::detachCE150() { detachAt(sharp::kPrinter, sharp::kCE150); }

bool MachineSession::ce150Attached() const { return pluggedAt(sharp::kPrinter) == sharp::kCE150; }

bool MachineSession::attachCE158(std::string* error) {
    return attachAt(sharp::kInterface, sharp::kCE158, std::nullopt, nullptr, error);
}

void MachineSession::detachCE158() { detachAt(sharp::kInterface, sharp::kCE158); }

bool MachineSession::ce158Attached() const { return pluggedAt(sharp::kInterface) == sharp::kCE158; }

std::vector<std::uint8_t> MachineSession::drainCE158PrinterOutput() {
    auto* port = kind<sharp::Ce158PrinterPort>();
    return port ? port->drain() : std::vector<std::uint8_t>{};
}

namespace {

VariantId ce1600pVariant(CE1600PRomVersion v) { return VariantId{v == CE1600PRomVersion::Old ? "Old" : "New"}; }

} // namespace

bool MachineSession::attachCE1600P(std::string* error) {
    if (!isPC1600()) return false;
    std::string err;
    std::vector<RomImage> probe;
    if (!sharp::peripheralRomImages(m_catalogue, PeripheralId{sharp::kCE1600P}, ce1600pVariant(m_ce1600pRomVersion),
                                    bundledRomDirs(), &probe, &err)) {
        // Like the PC-1600's own old ROM: the old set is optional (its dump
        // may be missing), so fall back to the new one. A failing new set
        // just leaves the plotter detached.
        if (m_ce1600pRomVersion != CE1600PRomVersion::Old) {
            if (error) *error = err;
            return false;
        }
        m_listener.notice(Notice::OldCE1600PRomUnavailable, err);
        m_ce1600pRomVersion = CE1600PRomVersion::New;
    }
    if (!attachAt(sharp::kPrinter, sharp::kCE1600P, ce1600pVariant(m_ce1600pRomVersion), nullptr, error)) return false;
    // The disk goes in when the CE-1600F is on the bus: at the off point of
    // the power cycle that follows, as in 0.8.1 (adapter, M-47).
    sharp::legacy::atOffPoint(*m_machine, [this] {
        if (m_bays) m_bays->insertSelectedDisk();
    });
    return true;
}

void MachineSession::detachCE1600P() {
    if (isPC1600()) busChange([this] { detachAt(sharp::kPrinter, sharp::kCE1600P); });
}

bool MachineSession::swapCE1600PRom(CE1600PRomVersion version, std::string* error) {
    if (!ce1600pAttached()) return false;
    sharp::FloppyDriveAccess* drive = floppyDrive();
    const int side = drive ? drive->side() : 0;
    if (m_bays) m_bays->flushFloppy();
    setCE1600PRomVersion(version);
    PeripheralRequest req{PlacePath{{PlaceName{sharp::kPrinter}}}, PeripheralId{sharp::kCE1600P}, ce1600pVariant(version), {},
                          nullptr, {}};
    std::string err;
    if (!sharp::peripheralRomImages(m_catalogue, req.peripheral, req.variant, bundledRomDirs(), &req.roms, &err)) {
        if (version != CE1600PRomVersion::Old) {
            if (error) *error = err;
            return false;
        }
        m_listener.notice(Notice::OldCE1600PRomUnavailable, err);
        m_ce1600pRomVersion = CE1600PRomVersion::New;
        req.variant = ce1600pVariant(CE1600PRomVersion::New);
        req.roms.clear();
        if (!sharp::peripheralRomImages(m_catalogue, req.peripheral, req.variant, bundledRomDirs(), &req.roms, &err)) {
            if (error) *error = err;
            return false;
        }
    }
    // One replace: the other variant in the same place, one power cycle
    // (C1600-1 AC-18, design D-5).
    const ChangeResult r = m_machine->replace(PeripheralId{sharp::kCE1600P}, std::move(req), m_catalogue);
    if (r.refused) {
        if (error) *error = r.refused->reason;
        return false;
    }
    noteChange(r);
    // The disk moves over with the same side up (adapter, M-47).
    sharp::legacy::atOffPoint(*m_machine, [this, side] {
        if (m_bays) m_bays->insertSelectedDisk();  // re-inserts the selected disk, side A up
        if (sharp::FloppyDriveAccess* d = floppyDrive(); d && side != 0 && d->hasDisk()) d->setSide(side);
    });
    return true;
}

bool MachineSession::ce1600pAttached() const { return pluggedAt(sharp::kPrinter) == sharp::kCE1600P; }

bool MachineSession::attachHostDrive(const std::string& dir, std::string* error) {
    if (!isPC1600()) return false;
    if (auto* folder = kind<sharp::HostDriveFolder>()) {
        folder->setFolder(dir);   // only the directory changes, live
        return true;
    }
    return attachAt(sharp::kHostDrivePlace, sharp::kHostDrive, std::nullopt, std::make_shared<sharp::HostDriveRequest>(dir),
                    error);
}

std::string MachineSession::hostDriveDirectory() const {
    auto* folder = kind<sharp::HostDriveFolder>();
    return folder ? folder->folder() : std::string();
}

void MachineSession::detachHostDrive() { detachAt(sharp::kHostDrivePlace, sharp::kHostDrive); }

bool MachineSession::hostDriveAttached() const { return pluggedAt(sharp::kHostDrivePlace) == sharp::kHostDrive; }

MachineSession::NoticeText MachineSession::noticeText(Notice notice, const std::string& detail) {
    const std::string why = detail.empty() ? "" : ":\n\n" + detail;
    switch (notice) {
        case Notice::OldPC1600RomUnavailable:
            return {"Old ROM unavailable", "The old PC-1600 ROM could not be loaded" + why + "\n\nUsing the new ROM instead."};
        case Notice::OldCE1600PRomUnavailable:
            return {"Old CE-1600P ROM unavailable",
                    "The old CE-1600P ROM could not be loaded" + why + "\n\nUsing the new ROM instead."};
        case Notice::TapeSaveFailed: return {"Tape", "The tape recording could not be saved" + why};
    }
    return {};
}

namespace {

const char* paperOwner(PaperKind kind) {
    switch (kind) {
        case PaperKind::CE150: return sharp::kCE150;
        case PaperKind::CE1600P: return sharp::kCE1600P;
        default: return nullptr;
    }
}

} // namespace

std::vector<AlpsPlotterMechanism::FlatPoint> MachineSession::plotPoints(PaperKind kind) const {
    const char* owner = paperOwner(kind);
    auto* paper = owner ? kindOf<sharp::PlotterPaper>(owner) : nullptr;
    return paper ? paper->points() : std::vector<AlpsPlotterMechanism::FlatPoint>{};
}

std::uint64_t MachineSession::plotRevision(PaperKind kind) const {
    const char* owner = paperOwner(kind);
    auto* paper = owner ? kindOf<sharp::PlotterPaper>(owner) : nullptr;
    return paper ? paper->revision() : 0;
}

void MachineSession::clearPaper(PaperKind kind) {
    const char* owner = paperOwner(kind);
    if (auto* paper = owner ? kindOf<sharp::PlotterPaper>(owner) : nullptr) paper->clear();
}

} // namespace shared
