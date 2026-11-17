#include "TIAppPart.hpp"
#include "ModuleCueCards.hpp"
#include "TIMagneticCardKind.hpp"
#include "KeyboardLayout.hpp"
#include "TextUtil.hpp"
#include "../core/PC100C.hpp"
#include "../core/TIDebugging.hpp"
#include "../core/TIDebugKinds.hpp"
#include "sessions/LibraryTopics.hpp"
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
    // C59-7 AC-25: a new machine has nothing fitted until the preset or the
    // constant memory file fits something (the media part cleared its binding).
    if (m_lastFit == Fitted::Card) m_lastFit = Fitted::None;
    m_activeProgram = 0;
    if (!m) {
        m_session.reset();
        return;
    }
    // LIB-10 §12.11: a pause or a step waiting to land runs on in the host
    // call that asked for it (2.3.0's freeze and STEP ran there, at most
    // 50000 weights), instead of in paced emulated time.
    if (auto* d = m->debugging()) {
        calcu::debugger::RunControl* rc = &m_lib.debugger().runControl();
        static_cast<TIDebugging*>(d)->setRunOn([rc] { return !rc->running() && !rc->paused(); });
    }
    m_session = std::make_unique<EmulatorSession>(*m, *m_setup.roms.set);
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
    followPlugs(true);
    refreshCueCard();
}

void TIAppPart::followPlugs(bool newMachine) {
    if (!m_session) return;
    // The module (D-26: what the machine says is plugged).
    const std::string module = m_session->moduleID();
    if (newMachine || module != m_moduleID) {
        m_moduleID = module;
        // C59-7 AC-22, AC-23: the module cue cards from the module file.
        m_moduleCards = m_setup.roms.set ? moduleCueCards(*m_setup.roms.set, m_moduleID, [this](const std::string& m) { diagnostic(m); })
                                     : std::map<int, CueCardContent>{};
        refreshCueCard();
    }
    // The printer, and TRACE (C59-6 AC-6, LIB-18 D-27): the position follows
    // what the machine's PC-100C shows (LIB-2's set-control, a preset's build,
    // printer-trace:) and stays while none is plugged; an attach puts it into
    // the new PC-100C (TIProductMachines::attaching).
    m_printerConnected = m_session->printerPlugged();
    if (m_printerConnected)
        if (const auto on = m_session->printerSwitch(PC100C::kTrace)) m_printerTrace = *on;
}

void TIAppPart::machineRestarted(calcu::sessions::SessionContext& ctx, calcu::sessions::Restart r) {
    m_ctx = &ctx;
    if (!m_session) return;
    // LIB-18 AC-23: the paper, the plugs, TRACE and the fitted cards stay
    // (C59-7 AC-25: a card in the reader was taken out by the core's place;
    // the ASM overlay's run ends in the core, C59-4 AC-18).
    m_activeProgram = 0;
    diagnostic(r == calcu::sessions::Restart::ColdBoot ? "Clean Reset — all registers cleared" : "Calculator Reset");
    refreshCueCard();
}

void TIAppPart::fitOwnCueCard(const CueCardContent& card) {
    refreshCueCard();   // D-6: a card fitted before this section is seen first
    m_userCueCard = card;
    m_userCueCardGeneration = m_ms ? m_ms->generation() : 0;
    m_lastFit = Fitted::Own;   // D-6: in the place of the card fitted before
    refreshCueCard();
}

// ── 60 Hz frame ──────────────────────────────────────────────────────────────

void TIAppPart::frame(Clock::time_point) {
    if (!m_session || !machine()) return;
    const FrameUpdate f = m_session->tick();
    m_display = f.led;
    m_calcOpacity = f.calcIndicatorOpacity;
    refreshCueCard();   // D-7: the library program and the fitted card at each call
}

void TIAppPart::refreshCueCard() {
    // AC-21: the library program selected now, from the product kind.
    int program = 0;
    if (calcu::machine::Machine* m = machine())
        if (auto* k = calcu::machine::first<LibraryProgram>(*m)) program = k->selected().value_or(0);
    m_activeProgram = program;
    // The fitted magnetic card: the media part's binding of the card reader.
    const calcu::media::Binding* b = nullptr;
    std::optional<int> passing;
    if (m_ctx) {
        b = m_lib.media().bindingOf("card-reader");
        if (calcu::machine::Machine* m = machine(); b && m)
            if (auto* place = calcu::machine::first<calcu::media::MediaPlace>(*m)) passing = place->side();
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
    m_cueCard = std::move(card);
    m_onShow = onShow;
    m_litCorner = lit;
}

// ── Magnetic cards (C59-7; the library's media part) ─────────────────────────

bool TIAppPart::cardFitted() const {
    return m_lib.media().bindingOf("card-reader") != nullptr;
}

bool TIAppPart::tapCard(double nx, double ny) {
    refreshCueCard();
    if (!m_onShow || !m_ctx) return false;   // AC-24: covered by a module cue card
    const auto side = cardCornerAt(nx, ny);
    if (!side) return false;
    if (auto refused = m_lib.media().feed(*m_ctx, "card-reader", *side)) return false;   // AC-16: one passing
    refreshCueCard();
    return true;
}

} // namespace cu59
