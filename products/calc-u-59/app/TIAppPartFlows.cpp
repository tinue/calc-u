#include "TIAppPart.hpp"
#include "TIAppPartInternal.hpp"
#include "AsmOverlay.hpp"
#include "CardFile.hpp"
#include "ConstantMemory.hpp"
#include "KeyboardLayout.hpp"
#include "StateApplier.hpp"
#include "TextUtil.hpp"
#include "../core/PC100C.hpp"
#include "../core/TICatalogue.hpp"
#include "../core/PC100CPaper.hpp"
#include <algorithm>

namespace cu59 {

using namespace std::chrono_literals;

// The product's flows of cu59::TIAppPart (2.3.0's SessionController, taken
// apart in M-34). The session plumbing is TIAppPart.cpp.


void TIAppPart::later(uint64_t sessionGeneration, std::function<void()> f) {
    m_queue.push_back([this, sessionGeneration, f = std::move(f)] {
        if (sessionGeneration != m_sessionGeneration) return;
        f();
    });
}

// ── Machine ──────────────────────────────────────────────────────────────────

bool TIAppPart::startMachine(Model model) {
    persistConstantMemory();  // save TI-58C RAM before switching away
    if (m_ms) m_ms->dropInput();   // LIB-5 EC-5: the keys the engine holds come up first

    const uint64_t generation = m_sessionGeneration + 1;
    auto events = std::make_unique<Events>(*this, generation);
    // The places as the app session has them (C59-6): the module, the PC-100C
    // with TRACE where it was (AC-6), attached at the build.
    MachinePeripherals places{currentModule(), m_printerConnected, m_printerTrace};
    auto session = std::make_unique<EmulatorSession>(model, *m_setup.roms, places, *events);
    m_session = std::move(session);  // the old session goes first, then its events
    m_events = std::move(events);
    m_sessionGeneration = generation;
    m_model = model;

    // A new machine: running, nothing armed (the run control, LIB-10 G11), no
    // card; TRACE as it was (AC-6).
    m_frozenAt.reset();
    clearCardAndCue();
    m_memoryCards.clear();
    m_keystrokesPlaying = false;
    m_fullSpeed = false;
    if (m_typing) m_session->setTyping(&m_typing->engine());

    auto cards = moduleCards(m_setup.cuecards, m_moduleID);
    m_moduleCards = std::move(cards.cards);
    m_moduleMetadata = cards.metadata;
    m_debug.attach(*m_session);
    if (!m_debug.moduleImageError().empty()) diagnostic(m_debug.moduleImageError());

    {
        auto a = m_session->access();
        a.debugPanel().setDebugLevel(static_cast<uint8_t>(m_debugLevel));
        if (info(model).hasConstantMemory) {
            auto saved = loadConstantMemory();
            powerOn(a.machine(), model, saved ? &*saved : nullptr);
        }
        if (!m_asmWords.empty() && !a.debugPanel().loadDebugOverlay(m_asmWords.data(), m_asmWords.size())) {
            m_asmWords.clear();
            m_asmActive = false;
            m_asmStatus = "ASM overlay cleared (incompatible after model switch).";
        }
    }
    m_session->access().debugPanel().setPanelTrace(m_cpuDebugEnabled);
    m_debug.resetHeatmapBaseline(m_session->access().machine());
    refreshCueCard();
    emitChanged(ModelChanged | RunStateChanged | CardChanged | PrinterChanged | CueCardChanged | LiveChanged |
                CpuChanged | HeatmapChanged | AsmChanged);
    return true;
}

void TIAppPart::beginReset() {
    m_session->cancelKeystrokes();
    if (m_ms) m_ms->dropInput();   // LIB-5 EC-5 (LIB-18 §3.4): reset, cold boot, power cycle
    if (auto* rc = runControl(); rc && !rc->running()) rc->resume();   // as 2.3.0: a reset unfreezes
    m_asmActive = false;
    clearCardAndCue();
    dropPresetCards();
    refreshCueCard();
}

void TIAppPart::clearCardAndCue() {
    m_pass.reset();
    m_queuedCardInReader = false;
    m_userCueCard.reset();
    m_fitted.reset();
    m_presetCue.reset();
    m_activeProgram = 0;
}

void TIAppPart::adoptPresetCue(const std::optional<CueCardContent>& cue) {
    m_userCueCard = cue;
    m_presetCue = cue;
    m_fitted.reset();
    refreshCueCard();
    emitChanged(CardChanged);
}

void TIAppPart::resetMachine() {
    if (!m_session) return;
    beginReset();
    // TRACE survives a reset, as on the hardware (the PC-100C keeps it,
    // C59-6 AC-6). The TI-58C keeps its constant memory from the running machine
    // (C59-1 OQ-6); the TI-59/58 keep RAM and partition (AC-6, AC-19).
    resetStopped();
    m_debug.resetHeatmapBaseline(m_session->access().machine());
    log({"Calculator Reset"});
    emitChanged(ModelChanged | CardChanged | AsmChanged | HeatmapChanged);
}

void TIAppPart::resetStopped() {
    m_session->access().machine().reset();
    if (m_ctx) m_ctx->tellRestarted(calcu::sessions::Restart::Reset);   // LIB-10 adapter L-E2 (until M-54)
}

void TIAppPart::cleanResetMachine() {
    if (!m_session) return;
    beginReset();
    m_session->access().machine().coldBoot();
    if (m_ctx) m_ctx->tellRestarted(calcu::sessions::Restart::ColdBoot);   // L-E2
    m_debug.resetHeatmapBaseline(m_session->access().machine());
    persistConstantMemory();  // the zeroed state, at once
    log({"Clean Reset — all registers cleared"});
    emitChanged(ModelChanged | CardChanged | AsmChanged | HeatmapChanged);
}

// ── Places (C59-6) ───────────────────────────────────────────────────────────

std::optional<std::string> TIAppPart::currentModule() const {
    return isTIModule(m_moduleID) ? std::optional<std::string>(m_moduleID) : std::nullopt;
}

TIAppPart::PlaceChange TIAppPart::changePlacesNow(const std::optional<std::string>& module,
                                                                  bool printer) {
    using calcu::machine::AfterChange;
    PlaceChange pc;
    if (module != currentModule()) {
        auto r = m_session->setModule(module, *m_setup.roms);
        if (r.refused) {
            pc.refusal = r.refused->reason;
            return pc;
        }
        pc.moduleChanged = true;
        pc.powerCycle |= r.follows == AfterChange::PowerCycle;
    }
    if (printer != m_printerConnected) {
        auto r = m_session->setPrinter(printer, m_printerTrace);
        if (r.refused) {
            pc.refusal = r.refused->reason;
            return pc;
        }
        pc.printerChanged = true;
        pc.powerCycle |= r.follows == AfterChange::PowerCycle;
    }
    return pc;
}

void TIAppPart::adoptPlaces(const PlaceChange& pc, const std::optional<std::string>& module, bool printer) {
    if (pc.refusal) diagnostic(*pc.refusal);
    if (pc.moduleChanged) {
        m_moduleID = module.value_or(std::string());
        m_debug.moduleChanged(*m_session);
        if (!m_debug.moduleImageError().empty()) diagnostic(m_debug.moduleImageError());
        auto cards = moduleCards(m_setup.cuecards, m_moduleID);
        m_moduleCards = std::move(cards.cards);
        m_moduleMetadata = cards.metadata;
        refreshCueCard();
        emitChanged(ModelChanged);
    }
    if (pc.printerChanged) {
        m_printerConnected = printer;   // the paper stays the PaperPart's (LIB-8 AC-8, AC-29)
        emitChanged(PrinterChanged);
    }
}

bool TIAppPart::changePlaces(const std::optional<std::string>& module, bool printer) {
    if (!m_session) return false;
    if (module == currentModule() && printer == m_printerConnected) return true;
    beginReset();
    PlaceChange pc = changePlacesNow(module, printer);
    // The power cycle that follows (LIB-2 AC-24; row 59-11): the ROM finds
    // what is plugged in now (AC-11).
    if (pc.powerCycle) m_session->access().machine().powerCycle();
    adoptPlaces(pc, module, printer);
    if (pc.refusal) return false;
    m_debug.resetHeatmapBaseline(m_session->access().machine());
    if (pc.powerCycle) log({"Calculator power cycle"});
    emitChanged(ModelChanged | CardChanged | AsmChanged | HeatmapChanged);
    return true;
}

bool TIAppPart::selectModule(const std::string& moduleID) {
    if (!m_session || !isTIModule(moduleID)) return false;
    return changePlaces(moduleID, m_printerConnected);
}

void TIAppPart::changeModule(const std::string& moduleID) {
    if (moduleID == m_moduleID) return;
    if (moduleID.empty()) changePlaces(std::nullopt, m_printerConnected);
    else selectModule(moduleID);
}

// ── TI-58C constant memory ───────────────────────────────────────────────────

// Requested after every batch while running (debounced); the file is only
// written when its contents changed.
void TIAppPart::persistConstantMemory() {
    m_persistDue.reset();
    if (!m_session || !info(m_model).hasConstantMemory) return;
    std::vector<uint8_t> ram(kConstantMemoryBytes);
    m_session->access().memory().serialiseRAM(ram.data());
    std::string text = encodeConstantMemory(ram, m_userCueCard ? &*m_userCueCard : nullptr);
    if (text == m_persistedText) return;
    m_files.writeConstantMemory(text);
    m_persistedText = std::move(text);
}

std::optional<std::vector<uint8_t>> TIAppPart::loadConstantMemory() {
    auto text = m_files.readConstantMemory();
    if (!text) return std::nullopt;
    auto decoded = decodeConstantMemory(*text);
    if (!decoded) return std::nullopt;  // invalid file: the caller keeps zeroed RAM
    m_userCueCard = decoded->cueCard;
    refreshCueCard();
    return decoded->ram;
}

// ── 60 Hz frame ──────────────────────────────────────────────────────────────

void TIAppPart::frame(Clock::time_point now) {
    if (!m_session) return;
    m_session->pump(now);
    uint32_t changes = 0;
    FrameUpdate f = m_session->tick(static_cast<int>(m_debugLevel), m_pass.has_value());
    if (f.cardAutoEjected) ejectCard();  // the core ejected at CRD_OFF: save written data
    if (!m_pass) feedQueuedCard();
    if (!f.logLines.empty()) m_debug.log().writeAll(f.logLines);

    if (m_session->takeRamDirty() && info(m_model).hasConstantMemory && !m_persistDue) m_persistDue = now + 500ms;
    if (m_persistDue && now >= *m_persistDue) persistConstantMemory();

    if (f.led != m_display) {
        m_display = f.led;
        changes |= DisplayChanged;
    }
    if (f.calcIndicatorOpacity != m_calcOpacity) {
        m_calcOpacity = f.calcIndicatorOpacity;
        changes |= DisplayChanged;
    }

    DebugModel::Update u = m_debug.tick(m_session->access().machine(), m_liveDebugEnabled, m_cpuDebugEnabled,
                                        isFrozen());
    if (u.live) changes |= LiveChanged;
    if (u.listing) changes |= ListingChanged;
    if (u.cpu) changes |= CpuChanged;
    if (u.heatmap) changes |= HeatmapChanged;
    if (u.diagnostic) diagnostic(*u.diagnostic);

    if (now >= m_nextProgramCheck) {  // cue-card program detection, 2 Hz
        m_nextProgramCheck = now + 500ms;
        checkProgramNumber();
    }
    if (now >= m_nextLogRefresh) {  // LOG pane, 1 Hz
        m_nextLogRefresh = now + 1s;
        refreshLog();
    }
    emitChanged(changes);
}

void TIAppPart::checkProgramNumber() {
    int program = activeLibraryProgram(m_session->access().debugPanel().snapshotCPU().SCOM[9]);
    if (program != m_activeProgram) {
        m_activeProgram = program;
        refreshCueCard();
    }
}

void TIAppPart::refreshCueCard() {
    auto card = resolveCueCard(m_activeProgram, m_moduleCards, m_moduleMetadata.title, m_moduleMetadata.id,
                               m_fitted ? std::optional<CueCardContent>(fittedCueCard()) : m_userCueCard);
    if (card != m_cueCard) {
        m_cueCard = card;
        emitChanged(CueCardChanged);
    }
}

// ── Keys and speed ───────────────────────────────────────────────────────────


void TIAppPart::setFullSpeed(bool on) {
    if (m_fullSpeed == on) return;
    m_fullSpeed = on;
    if (m_session) m_session->setFullSpeed(on);
    emitChanged(RunStateChanged);
}

// ── Printer ──────────────────────────────────────────────────────────────────

void TIAppPart::setPrinterConnected(bool connected) {
    if (!m_session) {
        m_printerConnected = connected;
        emitChanged(PrinterChanged);
        return;
    }
    changePlaces(currentModule(), connected);
}

void TIAppPart::togglePrinterConnection() { setPrinterConnected(!m_printerConnected); }

bool TIAppPart::setPrinterControl(const std::string& control, bool on) {
    if (control == PC100C::kTrace.value) {   // C59-6 AC-6: the app session keeps TRACE
        m_printerTrace = on;
        if (m_session) m_session->printerControl(PC100C::kTrace, on);
        emitChanged(PrinterChanged);
        return true;
    }
    if (control == PC100C::kPrint.value || control == PC100C::kAdv.value) {
        if (m_session) m_session->printerControl(calcu::machine::ControlId{control}, on);
        return true;
    }
    return false;
}

// ── Card reader ──────────────────────────────────────────────────────────────

bool TIAppPart::canFit() const { return m_session && info(m_model).hasCardReader && !m_pass; }

void TIAppPart::fit(const std::string& path, const std::string& fileName, CardDocument doc, bool fresh) {
    m_fitted = FittedCard{path, std::move(doc), fresh};
    m_cardFileName = fileName;
    refreshCueCard();
    emitChanged(CardChanged);
}

bool TIAppPart::fitCard(const std::optional<std::vector<uint8_t>>& raw, const std::string& path,
                                const std::string& fileName) {
    if (!canFit()) return false;
    if (!raw) {
        error("Card load error", "Could not read card file.");
        return false;
    }
    auto decoded = decodeCardFile(*raw, fileName);
    if (!decoded.card) {
        error("Card load error", decoded.error);
        return false;
    }
    fit(path, fileName, std::move(*decoded.card), false);
    return true;
}

bool TIAppPart::fitBlankCard(const std::string& savePath, const std::string& fileName) {
    if (!canFit()) return false;
    CardDocument doc;
    doc.cueCard = freshCardCue(fileName);
    fit(savePath, fileName, std::move(doc), true);
    return true;
}

bool TIAppPart::feedSide(int side) { return feed(side, false); }

bool TIAppPart::feed(int side, bool queued) {
    if (!m_session || !m_fitted || side < 0 || side > 1 || m_pass) return false;
    startSwipe(m_fitted->doc.bank(side), {side, queued, std::nullopt});
    return true;
}

bool TIAppPart::tapCard(double nx, double ny) {
    if (!cardOnShow()) return false;
    auto side = cardCornerAt(nx, ny);
    if (!side) return false;
    feedSide(*side);
    return true;
}

std::optional<int> TIAppPart::litCardCorner() const {
    // A mem: card takes the fitted card out, so with one fitted the pass is its side.
    if (!cardOnShow() || !m_pass) return std::nullopt;
    return m_pass->side;
}

CueCardContent TIAppPart::fittedCueCard() const {
    // A card without a cue card still needs something to show; never saved.
    CueCardContent cue = m_fitted->doc.cueCard ? *m_fitted->doc.cueCard : freshCardCue(m_cardFileName);
    cue.template_ = CueCardTemplate::MagnetCard;
    if (m_fitted->doc.version == 2) {
        cue.bankLeft = m_fitted->doc.sideBank(0);
        cue.bankRight = m_fitted->doc.sideBank(1);
    }
    return cue;
}

void TIAppPart::startSwipe(const std::vector<uint8_t>& data, const CardPass& pass) {
    m_pass = pass;
    m_session->access().cardReader().insertCard(data.empty() ? nullptr : data.data(), data.size());
    emitChanged(CardChanged);
}

void TIAppPart::ejectCard() {
    if (!m_session || !m_pass) {
        m_pass.reset();
        emitChanged(CardChanged);
        return;
    }
    std::vector<uint8_t> written = m_session->access().cardReader().cardEject();
    const CardPass pass = *std::exchange(m_pass, std::nullopt);
    if (!written.empty()) {
        if (pass.memoryKey) m_memoryCards[*pass.memoryKey] = {written, m_userCueCard};
        else if (m_fitted) writeFittedSide(pass.side, written, pass.queued);
    }
    queuedCardDone();
    emitChanged(CardChanged);
}

void TIAppPart::feedQueuedCard() {
    if (!m_session) return;
    auto card = m_session->takeQueuedCard();
    if (!card) return;
    m_queuedCardInReader = card->insertNow;
    if (isMemoryCardName(card->name)) {
        // A bank written to it earlier in the preset, else its CARD: block, else blank.
        std::string key(memoryCardKey(card->name));
        std::vector<uint8_t> bank;
        std::optional<CueCardContent> cueCard;
        if (auto it = m_memoryCards.find(key); it != m_memoryCards.end()) {
            bank = it->second.bank;
            cueCard = it->second.cueCard;
        } else if (card->definition) {
            bank = buildCardBank(*card->definition, m_session->access().memory().partitionProgramRegs());
            cueCard = card->definition->cueCard;
        }
        m_fitted.reset();  // a memory card has no file behind it: it is not fitted
        if (cueCard) m_userCueCard = cueCard;
        refreshCueCard();
        m_cardFileName = card->name;
        startSwipe(bank, {0, false, key});
        return;
    }
    std::string fileName = withDefaultCardExtension(card->name);
    const std::string path = m_files.cardFilePath(fileName);
    FileIO::Read file = m_files.readCard(path);
    bool fitted;
    if (file.unreadable) {
        error("Card load error", "Could not read card file " + fileName + ".");
        fitted = false;
    } else if (!file.raw) {
        log({"KEYSTROKES: no card file " + fileName + " — a blank card goes in"});
        fitted = fitBlankCard(path, fileName);
        if (fitted && m_presetCue) {
            m_fitted->doc.cueCard = m_presetCue;
            refreshCueCard();
        }
    } else {
        fitted = fitCard(file.raw, path, fileName);
    }
    if (!fitted) {
        queuedCardDone();
        return;
    }
    if (card->side == 1 && m_fitted->doc.version == 1)
        log({"KEYSTROKES: card file " + fileName + " has one side — that side goes in"});
    feed(card->side, true);
}

void TIAppPart::dropPresetCards() {
    if (!m_session) return;
    std::vector<std::string> lines;
    for (const auto& name : m_session->clearQueuedCards())
        lines.push_back("KEYSTROKES: card " + name + " was never read or written");
    if (!lines.empty()) log(lines);
    m_memoryCards.clear();
}

void TIAppPart::queuedCardDone() {
    if (!m_queuedCardInReader) return;
    m_queuedCardInReader = false;
    m_session->hostDone();
}

void TIAppPart::writeFittedSide(int side, const std::vector<uint8_t>& written, bool queued) {
    const std::string saveFailed = "Card save failed: " + m_fitted->path;
    CardDocument doc = m_fitted->doc;
    if (!m_fitted->fresh) {
        // The file may have changed since it was fitted (iCloud, another app): keep its other side.
        FileIO::Read file = m_files.readCard(m_fitted->path);
        if (file.unreadable) {
            error("Card save error", saveFailed);
            return;
        }
        if (file.raw) {
            auto decoded = decodeCardFile(*file.raw, m_cardFileName);
            if (!decoded.card) {
                error("Card save error", decoded.error);
                return;
            }
            doc = std::move(*decoded.card);
        }
    }
    doc.setBank(side, written);
    if (queued && m_presetCue) doc.cueCard = m_presetCue;
    if (!m_files.writeCard(m_fitted->path, encodeCardDocument(doc))) {
        error("Card save error", saveFailed);
        return;
    }
    m_fitted->doc = std::move(doc);
    m_fitted->fresh = false;
    refreshCueCard();  // the corner shows the bank just written
}

// ── State files (reference/SharedArchitecture.md § "State File Loading") ───────

void TIAppPart::loadStateFile(const std::optional<std::vector<uint8_t>>& raw) {
    const std::string title = "State file load error";
    if (!raw) {
        error(title, "Cannot read file.");
        return;
    }
    std::string text(raw->begin(), raw->end());
    if (!isValidUtf8(text)) {
        error(title, "File is not valid UTF-8 text — re-save it with UTF-8 encoding (a non-UTF-8 character, "
                                "e.g. a curly quote or em dash from a word processor, is the usual cause).");
        return;
    }
    const Model targetModel = stateFileModel(text, m_model);
    const ModelInfo& target = info(targetModel);
    LoadStateResult parsed = parseStateFile(text, target.maxStep(), target.hasConstantMemory);
    if (parsed.usesCardReader && !target.hasCardReader) {
        parsed.errors.push_back(std::string("CARD:, ArmCard: and InsertCard: need a card reader; the ") +
                                target.displayName + " has none.");
        auto& k = parsed.keystrokes;
        k.erase(std::remove_if(k.begin(), k.end(), [](const KeystrokeEvent& e) {
                    return e.kind == KeystrokeEvent::Kind::ArmCard || e.kind == KeystrokeEvent::Kind::InsertCard;
                }), k.end());
    }
    if (!parsed.errors.empty()) {
        std::string joined;
        for (const auto& e : parsed.errors) joined += (joined.empty() ? "" : "\n") + e;
        error(title, joined);
    }
    if (targetModel != m_model && !startMachine(targetModel)) return;
    applyState(parsed);
}

// ── Dropped files (reference/SharedArchitecture.md § "Dropped Files") ──────────

DropAction dropActionFor(const std::string& fileName, Model model) {
    if (isStateFileName(fileName)) return DropAction::LoadPreset;
    if (!isCardFileName(fileName)) return DropAction::NotAccepted;
    return info(model).hasCardReader ? DropAction::FitCard : DropAction::Ignore;
}

bool TIAppPart::openDroppedFile(const std::optional<std::vector<uint8_t>>& raw, const std::string& path,
                                        const std::string& fileName) {
    switch (dropAction(fileName)) {
        case DropAction::LoadPreset:
            loadStateFile(raw);
            return raw.has_value();
        case DropAction::FitCard:
            return fitCard(raw, path, fileName);
        case DropAction::Ignore:
        case DropAction::NotAccepted:
            return false;
    }
    return false;
}

void TIAppPart::applyState(const LoadStateResult& parsed) {
    if (!m_session) return;
    if (auto e = stateFilePartitionError(parsed, m_model)) {
        error("State file load error", *e);
        return;
    }
    dropPresetCards();
    // SOLID-STATE-MODULE: and PRINTER: first, as attach and detach; the power
    // cycle that follows is the one the state applier runs before it writes
    // the state (C59-6 design §4). An ID that is no module keeps the module.
    // TRACE is off after a load unless its keystrokes set it (row 59-25).
    std::optional<std::string> module = currentModule();
    if (parsed.solidStateModuleID && isTIModule(*parsed.solidStateModuleID)) module = *parsed.solidStateModuleID;
    const bool printer = parsed.printerConnected.value_or(m_printerConnected);
    m_printerTrace = false;
    if (auto* rc = runControl(); rc && !rc->running()) rc->resume();   // a load unfreezes, as 2.3.0
    m_activeProgram = 0;
    PlaceChange places = changePlacesNow(module, printer);
    m_session->printerControl(PC100C::kTrace, false);
    cu59::applyState(m_session->access().machine(), parsed, m_model);
    m_debug.resetHeatmapBaseline(m_session->access().machine());  // fresh machine state: skip the power-on walk
    adoptPlaces(places, module, printer);
    // LIB-8 AC-8, adapter P-E4 (until M-54): a preset that names the printer
    // (PRINTER:) starts with empty paper; one that does not keeps it (D-29).
    if (parsed.printerConnected && m_paperPart && m_ctx) {
        m_paperPart->sync(*m_ctx);
        if (auto f = m_paperPart->find(*m_ctx, std::string(kPC100C.value)); f.paper) f.paper->cut();
    }
    emitChanged(PrinterChanged);
    adoptPresetCue(parsed.cueCard);
    persistConstantMemory();  // after the cue card: ti58c.mem keeps the file's card
    emitChanged(RunStateChanged | HeatmapChanged);
    if (!parsed.keystrokes.empty()) m_session->playKeystrokes(parsed.keystrokes);
}

// LIB-10 adapter L-E1 (until M-54): the KEYSTROKES directives of a 2.3.0
// preset through the run control, as the steps of AC-32 (Freeze: Calculator
// is pause program, Freeze: CPU pause cpu, FreezeOnStart: pause-on-start,
// Step: step, Resume: resume). Freeze: and Step: hold the playback until
// their pause has landed, as 2.3.0 ran them before it went on.
void TIAppPart::applyControl(const KeystrokeEvent& e) {
    if (!m_session) return;
    using K = KeystrokeEvent::Kind;
    using calcu::debugger::kCpu;
    using calcu::debugger::kProgram;
    const calcu::debugger::LevelName level = e.panel == DebugPanel::Cpu ? kCpu : kProgram;
    calcu::debugger::RunControl* rc = runControl();
    bool done = true;
    switch (e.kind) {
    case K::PrinterTrace: setPrinterControl(PC100C::kTrace.value, e.on); break;
    case K::CutPaper:   // LIB-8 adapter P-E5 (until M-54): the step cut-paper
        if (m_paperPart && m_ctx) m_paperPart->cutPlugged(*m_ctx);
        break;
    case K::ShowPanel: showDebugPanel(e.panel); break;
    case K::Freeze:
        if (rc && rc->running()) {
            m_asmActive = false;
            emitChanged(AsmChanged);
            done = !rc->pause(level).deferred;
        }
        break;
    case K::FreezeOnStart:
        if (rc) rc->pauseOnStart(e.on ? std::optional<calcu::debugger::LevelName>(level) : std::nullopt);
        break;
    case K::Step:
        if (!rc || !rc->paused()) {
            log({"KEYSTROKES: Step: ignored — the calculator is not frozen"});
            break;
        }
        done = !rc->step(level, static_cast<std::uint64_t>(std::max(1, e.count)), std::nullopt,
                         calcu::debugger::StepKind::In)
                    .deferred;
        break;
    case K::Resume:
        if (rc) rc->resume();
        break;
    default: break;
    }
    if (done) m_session->hostDone();
    else m_controlWaitsForPause = true;   // hostDone when the pause lands (followRunState)
    emitChanged(RunStateChanged);
}

void TIAppPart::applyReloadStage(const ReloadStage& stage) {
    if (!m_session) return;
    const ModelInfo& mi = info(m_model);
    if (stage.partitionMaxStep && *stage.partitionMaxStep > mi.maxStep()) {
        log({"KEYSTROKES: reload skipped — its partition (" + std::to_string(*stage.partitionMaxStep) + ") exceeds the " +
             mi.displayName + " maximum (" + std::to_string(mi.maxStep()) + ")"});
    } else {
        applyReload(m_session->access().machine(), stage, m_model);
        if (stage.cueCard) adoptPresetCue(stage.cueCard);
        persistConstantMemory();
        emitChanged(RunStateChanged);
    }
    m_session->hostDone();
}

// ── Debugger ─────────────────────────────────────────────────────────────────

void TIAppPart::followRunState() {
    calcu::debugger::RunControl* rc = runControl();
    if (!rc || !m_session) return;
    rc->sync();   // a new machine first (G11): a pause of the old one is not shown on it
    const calcu::debugger::RunState now = rc->state();
    const bool paused = rc->paused();
    // The CPU tab's frame ring: while it shows, while paused and while a start
    // is armed (2.3.0's trace consumers).
    m_session->access().debugPanel().setPanelTrace(m_cpuDebugEnabled || paused || now.armed.has_value());
    if (paused) {
        // A pause or a step that landed: the frozen view of where the machine
        // stands now (LIB-10 §12.3: rebuilt when the run state changes).
        const calcu::machine::Ticks at = m_session->access().machine().tickCount();
        if (m_frozenAt != at || !m_seenRun.paused) {
            m_frozenAt = at;
            const FrozenViewReason reason = now.owner == calcu::debugger::kCpu && now.reason == calcu::debugger::RunState::Reason::Step
                                                ? FrozenViewReason::StepInstruction
                                                : FrozenViewReason::Freeze;
            DebugModel::Update u = m_debug.frozenView(m_session->access().machine(), reason);
            if (u.diagnostic) diagnostic(*u.diagnostic);
            emitChanged(changeIf(u.live, LiveChanged) | changeIf(u.listing, ListingChanged) | changeIf(u.cpu, CpuChanged));
        }
        if (m_controlWaitsForPause) {   // L-E1: Freeze: or Step: has landed
            m_controlWaitsForPause = false;
            m_session->hostDone();
        }
    } else {
        m_frozenAt.reset();
        if (m_controlWaitsForPause && rc->running()) {   // ended without landing (a reset, a resume)
            m_controlWaitsForPause = false;
            m_session->hostDone();
        }
    }
    if (!now.same(m_seenRun)) {
        m_seenRun = now;
        emitChanged(RunStateChanged);
    }
}

void TIAppPart::scriptedTrace(const std::optional<std::string>& fileName) {
    if (!m_debugger || !m_ctx || !m_session) return;
    calcu::debugger::TraceSession& t = m_debugger->traceSession();
    if (!fileName) {
        t.end(*m_ctx, calcu::debugger::TraceSession::Ending::Off);
        return;
    }
    // LIB-10 AC-22, ledger 59-19: a trace that cannot start fails its preset.
    if (auto why = t.start(*m_ctx, *fileName, false, true)) {
        error("Trace error", "KEYSTROKES \"Trace: " + *fileName + "\": " + *why);
        m_session->cancelKeystrokes();
    }
}

void TIAppPart::setLiveDebugEnabled(bool on) {
    m_liveDebugEnabled = on;
    // Shown while frozen: the frozen view may predate it, so build it now.
    if (on && isFrozen() && m_session) {
        DebugModel::Update u = m_debug.rebuildLive(m_session->access().machine(), true);
        emitChanged(changeIf(u.live, LiveChanged) | changeIf(u.listing, ListingChanged));
    }
}

void TIAppPart::setCpuDebugEnabled(bool on) {
    if (m_cpuDebugEnabled == on) return;
    m_cpuDebugEnabled = on;
    if (!m_session) return;
    // The heatmap shows activity since the panel became visible (no tracing
    // while hidden); a frozen heatmap is static, so keep it.
    if (on && !isFrozen()) {
        m_debug.resetHeatmapBaseline(m_session->access().machine());
        emitChanged(HeatmapChanged);
    }
    followRunState();   // the frame ring
}

void TIAppPart::clearHeatmap() {
    m_debug.clearHeatmap();
    emitChanged(HeatmapChanged);
}

void TIAppPart::cycleDebugLevel() {
    m_debugLevel = static_cast<DebugLevel>((static_cast<int>(m_debugLevel) + 1) % 3);
    if (m_session) m_session->access().debugPanel().setDebugLevel(static_cast<uint8_t>(m_debugLevel));
    emitChanged(LogChanged);
}

void TIAppPart::log(const std::vector<std::string>& lines) {
    m_debug.log().writeAll(lines);
    refreshLog();
}

void TIAppPart::refreshLog() {
    std::string text = m_debug.log().displayText();
    if (text == m_logText) return;
    m_logText = std::move(text);
    emitChanged(LogChanged);
}

void TIAppPart::clearLog() {
    m_debug.log().clear();
    m_logText.clear();
    emitChanged(LogChanged);
}

void TIAppPart::dumpVars() { if (m_session) log(cu59::dumpVars(m_session->access().machine())); }
void TIAppPart::dumpScom() { if (m_session) log(cu59::dumpScom(m_session->access().machine())); }
void TIAppPart::dumpProg() { if (m_session) log(cu59::dumpProg(m_session->access().machine())); }
void TIAppPart::dumpMemory() { if (m_session) log(cu59::dumpMemory(m_session->access().machine())); }

// ── ASM overlay ──────────────────────────────────────────────────────────────

void TIAppPart::loadAsm(const std::optional<std::string>& text, const std::string& fileName) {
    const std::string title = "ASM load error";
    if (!text) {
        m_asmStatus = "Could not read ASM file.";
        error(title, m_asmStatus);
    } else {
        try {
            std::vector<uint16_t> words = parseAsmWords(*text);
            if (!m_session) {
                m_asmStatus = "Machine not initialized yet.";
            } else if (!m_session->access().debugPanel().loadDebugOverlay(words.data(), words.size())) {
                m_asmStatus = "ASM program exceeds overlay range 0x1800-0x1FFF.";
                error(title, m_asmStatus);
            } else {
                m_asmWords = std::move(words);
                m_asmFileName = fileName;
                m_asmStatus = "Loaded " + std::to_string(m_asmWords.size()) + " word(s) at 0x1800.";
            }
        } catch (const AsmParseError& e) {
            m_asmStatus = e.what();
            error(title, m_asmStatus);
        }
    }
    emitChanged(AsmChanged);
}

void TIAppPart::runAsm() {
    if (m_asmWords.empty() || !m_session) {
        m_asmStatus = m_asmWords.empty() ? "No ASM overlay loaded." : "Machine not initialized yet.";
        emitChanged(AsmChanged);
        return;
    }
    calcu::debugger::RunControl* rc = runControl();
    const calcu::debugger::RunState before = runState();
    const bool freezeOnStart = before.armed == calcu::debugger::kCpu;
    if (rc) {
        rc->pauseOnStart(std::nullopt);
        if (!rc->running()) rc->resume();
    }
    AsmRunResult r = runAsmOverlay(m_session->access().machine(), m_asmWords);
    uint32_t changes = AsmChanged;
    if (!r.loaded) {
        m_asmStatus = "ASM program exceeds overlay range 0x1800-0x1FFF.";
        error("ASM load error", m_asmStatus);
    } else if (r.entered) {
        m_asmStatus = "ASM entered at 0x1800 (" + std::to_string(r.steps) + " step(s)).";
        m_asmActive = true;
        if (freezeOnStart && rc && m_rules) {
            // FREEZE ON START was armed: paused in place at the 0x1800 entry.
            m_rules->landNextPauseAtOnce();
            rc->pause(calcu::debugger::kCpu);
        }
    } else {
        m_asmStatus = "ASM entry failed after " + std::to_string(r.steps) + " step(s).";
        error("ASM load error", m_asmStatus);
    }
    emitChanged(changes | RunStateChanged);
}

void TIAppPart::clearAsm() {
    bool wasActive = m_asmActive;
    if (m_session) m_session->access().debugPanel().clearDebugOverlay();
    m_asmWords.clear();
    m_asmActive = false;
    m_asmStatus = "ASM overlay cleared.";
    // A running overlay would now execute the empty range: reset the machine.
    if (wasActive && m_session) {
        if (auto* rc = runControl(); rc && !rc->running()) rc->resume();
        resetStopped();
    }
    emitChanged(AsmChanged);
}

} // namespace cu59
