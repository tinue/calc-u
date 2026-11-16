#include "TIAppPart.hpp"
#include "AsmOverlay.hpp"
#include "ModuleCueCards.hpp"
#include "presets/Reader.hpp"
#include "TIMagneticCardKind.hpp"
#include "KeyboardLayout.hpp"
#include "TextUtil.hpp"
#include "../core/PC100C.hpp"
#include "../core/TIDebugging.hpp"
#include "../core/TICatalogue.hpp"
#include "../core/PC100CPaper.hpp"
#include <algorithm>

namespace cu59 {

using namespace std::chrono_literals;

// The product's flows of cu59::TIAppPart (2.3.0's SessionController, taken
// apart in M-34; the machine control and the presets went to the library in
// M-54). The session plumbing is TIAppPart.cpp.

// ── Machine (LIB-18 §11.3) ───────────────────────────────────────────────────

void TIAppPart::followMachine() {
    calcu::machine::Machine* m = machine();
    m_frozenAt.reset();
    // C59-7 AC-25: a new machine has nothing fitted until the preset or the
    // constant memory file fits something (the media part cleared its binding).
    if (m_lastFit == Fitted::Card) m_lastFit = Fitted::None;
    m_activeProgram = 0;
    if (!m) {
        m_session.reset();
        emitChanged(~0u);
        return;
    }
    // LIB-10 §12.11: a pause or a step waiting to land runs on in the host
    // call that asked for it (2.3.0's freeze and STEP ran there, at most
    // 50000 weights), instead of in paced emulated time.
    if (auto* d = m->debugging(); d && m_lib.debugger()) {
        calcu::debugger::RunControl* rc = &m_lib.debugger()->runControl();
        static_cast<TIDebugging*>(d)->setRunOn([rc] { return !rc->running() && !rc->paused(); });
    }
    m_session = std::make_unique<EmulatorSession>(*m, *m_setup.roms);
    m_model = m_session->model();

    // The own cue card (C59-2 AC-6, C59-3 AC-7, AC-8): a preset's build has
    // none until its `cue-card:` (fitted on this machine already, when the
    // section ran in the same call); a TI-58C built from the constant memory
    // file gets the file's card; any other build none (LIB-18 AC-20).
    const auto& built = m_lib.control().lastBuild();
    m_keeper.machineBuilt(m, built);
    using calcu::startup::BuildOrigin;
    const BuildOrigin origin = built ? built->origin : BuildOrigin::Start;
    const bool presetBuild = origin == BuildOrigin::Preset || origin == BuildOrigin::StartupPreset;
    if (!(presetBuild && m_userCueCard && m_userCueCardGeneration == m_ms->generation())) {
        m_userCueCard.reset();
        if (m_keeper.fromFile()) m_userCueCard = m_keeper.fileCueCard();
    }
    m_lastFit = m_userCueCard ? Fitted::Own : Fitted::None;

    m_moduleID.clear();
    m_moduleCards.clear();
    m_debug.attach(*m_session);
    if (!m_debug.moduleImageError().empty()) diagnostic(m_debug.moduleImageError());
    {
        auto a = m_session->access();
        a.debugPanel().setDebugLevel(static_cast<uint8_t>(m_debugLevel));
        if (!m_asmWords.empty() && !a.debugPanel().loadDebugOverlay(m_asmWords.data(), m_asmWords.size())) {
            m_asmWords.clear();
            m_asmActive = false;
            m_asmStatus = "ASM overlay cleared (incompatible after model switch).";
        }
        a.debugPanel().setPanelTrace(m_cpuDebugEnabled);
    }
    m_asmActive = false;
    m_debug.resetHeatmapBaseline(m_session->access().machine());
    followPlugs(true);
    refreshCueCard();
    emitChanged(ModelChanged | RunStateChanged | CardChanged | PrinterChanged | CueCardChanged | LiveChanged |
                CpuChanged | HeatmapChanged | AsmChanged);
}

void TIAppPart::followPlugs(bool newMachine) {
    if (!m_session) return;
    // The module (D-26: what the machine says is plugged).
    const std::string module = m_session->moduleID();
    if (newMachine || module != m_moduleID) {
        m_moduleID = module;
        m_session->refreshModule();
        if (!newMachine) {
            m_debug.moduleChanged(*m_session);
            if (!m_debug.moduleImageError().empty()) diagnostic(m_debug.moduleImageError());
        }
        // C59-7 AC-22, AC-23: the module cue cards from the module file.
        m_moduleCards = m_setup.roms ? moduleCueCards(*m_setup.roms, m_moduleID, [this](const std::string& m) { diagnostic(m); })
                                     : std::map<int, CueCardContent>{};
        refreshCueCard();
        emitChanged(ModelChanged);
    }
    // The printer, and TRACE (C59-6 AC-6, LIB-18 D-27): an attach puts the
    // remembered position into the new PC-100C; otherwise the position follows
    // what the machine's PC-100C shows (a preset's build, printer-trace:).
    const bool plugged = m_session->printerPlugged();
    if (plugged && !m_printerConnected && !newMachine) m_session->printerControl(PC100C::kTrace, m_printerTrace);
    if (plugged != m_printerConnected) {
        m_printerConnected = plugged;
        emitChanged(PrinterChanged);
    }
    if (plugged)
        if (const auto on = m_session->printerSwitch(PC100C::kTrace); on && *on != m_printerTrace) {
            m_printerTrace = *on;
            emitChanged(PrinterChanged);
        }
}

void TIAppPart::machineRestarted(calcu::sessions::SessionContext& ctx, calcu::sessions::Restart r) {
    m_ctx = &ctx;
    if (!m_session) return;
    // LIB-18 AC-23: the paper, the plugs, TRACE and the fitted cards stay
    // (C59-7 AC-25: a card in the reader was taken out by the core's place);
    // the ASM overlay stops, as 2.3.0's beginReset.
    m_asmActive = false;
    m_activeProgram = 0;
    m_debug.resetHeatmapBaseline(m_session->access().machine());
    log({r == calcu::sessions::Restart::ColdBoot ? "Clean Reset — all registers cleared" : "Calculator Reset"});
    refreshCueCard();
    emitChanged(ModelChanged | CardChanged | AsmChanged | HeatmapChanged);
}

void TIAppPart::fitOwnCueCard(const CueCardContent& card) {
    refreshCueCard();   // D-6: a card fitted before this section is seen first
    m_userCueCard = card;
    m_userCueCardGeneration = m_ms ? m_ms->generation() : 0;
    m_lastFit = Fitted::Own;   // D-6: in the place of the card fitted before
    refreshCueCard();
    emitChanged(CardChanged | CueCardChanged);
}

// ── 60 Hz frame ──────────────────────────────────────────────────────────────

void TIAppPart::frame(Clock::time_point now) {
    if (!m_session || !machine()) return;
    uint32_t changes = 0;
    FrameUpdate f = m_session->tick(static_cast<int>(m_debugLevel));
    if (!f.logLines.empty()) m_debug.log().writeAll(f.logLines);

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

    refreshCueCard();   // D-7: the library program and the fitted card at each call
    if (now >= m_nextLogRefresh) {  // LOG pane, 1 Hz
        m_nextLogRefresh = now + 1s;
        refreshLog();
    }
    emitChanged(changes);
}

void TIAppPart::refreshCueCard() {
    // AC-21: the library program selected now, from the product kind.
    int program = 0;
    if (calcu::machine::Machine* m = machine())
        for (const auto& e : m->capabilities(LibraryProgram::kKind))
            if (auto* k = calcu::machine::capabilityAs<LibraryProgram>(*m, e.id)) program = k->selected().value_or(0);
    m_activeProgram = program;
    // The fitted magnetic card: the media part's binding of the card reader.
    const calcu::media::Binding* b = nullptr;
    std::optional<int> passing;
    if (calcu::media::MediaPart* media = m_lib.media(); media && m_ctx) {
        b = media->bindingOf("card-reader");
        if (calcu::machine::Machine* m = machine(); b && m)
            for (const auto& e : m->capabilities(calcu::media::MediaPlace::kKind))
                if (auto* place = calcu::machine::capabilityAs<calcu::media::MediaPlace>(*m, e.id))
                    passing = place->side();
    }
    if (b && b->armed != m_seenArm) {   // D-6: a card fitted takes the place of the card before
        m_seenArm = b->armed;
        m_lastFit = Fitted::Card;
        m_userCueCard.reset();
    }
    if (!b && m_lastFit == Fitted::Card) m_lastFit = Fitted::None;
    std::optional<CueCardContent> fitted;
    if (m_lastFit == Fitted::Card && b) fitted = fittedCardCue(magneticCardOf(b->content).get(), b->name);
    else if (m_lastFit == Fitted::Own) fitted = m_userCueCard;
    // AC-22: the module cue card of the program selected, if the module file defines one.
    std::optional<CueCardContent> card = fitted;
    bool covered = false;
    if (program > 0)
        if (const auto it = m_moduleCards.find(program); it != m_moduleCards.end()) {
            card = it->second;
            covered = true;
        }
    const bool onShow = m_lastFit == Fitted::Card && b && !covered;
    const std::optional<int> lit = onShow ? passing : std::nullopt;
    if (card != m_cueCard || onShow != m_onShow || lit != m_litCorner) {
        m_cueCard = card;
        m_onShow = onShow;
        m_litCorner = lit;
        emitChanged(CueCardChanged);
    }
}

// ── Printer ──────────────────────────────────────────────────────────────────

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

// ── Magnetic cards (C59-7; the library's media part) ─────────────────────────

bool TIAppPart::cardFitted() const {
    auto* media = const_cast<TIAppPart*>(this)->m_lib.media();
    return media && media->bindingOf("card-reader") != nullptr;
}

bool TIAppPart::tapCard(double nx, double ny) {
    refreshCueCard();
    if (!m_onShow || !m_ctx) return false;   // AC-24: covered by a module cue card
    const auto side = cardCornerAt(nx, ny);
    if (!side) return false;
    calcu::media::MediaPart* media = m_lib.media();
    if (!media) return false;
    if (auto refused = media->feed(*m_ctx, "card-reader", *side)) return false;   // AC-16: one passing
    refreshCueCard();
    return true;
}

// ── Dropped files (reference/SharedArchitecture.md § "Dropped Files") ──────────

DropAction dropActionFor(const std::string& fileName, Model model) {
    if (isPresetFileName(fileName)) return DropAction::LoadPreset;
    // C59-7 AC-4: a card file ends in .magnetcard.yaml; a .u59 or .bin is no card.
    const std::string suffix = magneticCardKind().suffix;
    const bool card = fileName.size() > suffix.size() && fileName[0] != '.' &&
                      calcu::presets::lowerAscii(fileName.substr(fileName.size() - suffix.size())) == suffix;
    if (!card) return DropAction::NotAccepted;
    return info(model).hasCardReader ? DropAction::FitCard : DropAction::Ignore;
}

// ── Debugger ─────────────────────────────────────────────────────────────────

void TIAppPart::followRunState() {
    calcu::debugger::RunControl* rc = runControl();
    if (!rc || !m_session || !machine()) return;
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
    } else {
        m_frozenAt.reset();
    }
    if (!now.same(m_seenRun)) {
        m_seenRun = now;
        emitChanged(RunStateChanged);
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
        if (freezeOnStart && rc) {
            // FREEZE ON START was armed: paused in place at the 0x1800 entry.
            m_levelRules.landNextPauseAtOnce();
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
        m_session->access().machine().reset();
        if (m_ctx) m_ctx->tellRestarted(calcu::sessions::Restart::Reset);
    }
    emitChanged(AsmChanged);
}

} // namespace cu59
