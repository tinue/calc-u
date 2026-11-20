#include "TIAppPart.hpp"
#include "ModuleCueCards.hpp"
#include "TIMagneticCardKind.hpp"
#include "KeyboardLayout.hpp"
#include "TextUtil.hpp"
#include "../core/PC100C.hpp"
#include "../core/TIDebugging.hpp"
#include "../core/TIDebugKinds.hpp"
#include "sessions/LibraryTopics.hpp"
#include "peripherals/Controls.hpp"
#include "../core/TICatalogue.hpp"
#include "../core/PC100CPaper.hpp"
#include <algorithm>

namespace cu59 {

using namespace std::chrono_literals;

namespace {
const calcu::machine::PlacePath kSlot{{kTIModulePlace}};
const calcu::machine::PlacePath kPrinter{{kTIPrinterPlace}};
const calcu::machine::CapabilityId kControls{"printer/controls"};

/// The module in `slot-1` now (empty: none).
std::string moduleIn(const calcu::machine::Machine& m) {
    for (const auto& e : m.plugged().byPlace)
        if (e.place == kSlot) return e.peripheral.value;
    return {};
}

bool printerIn(const calcu::machine::Machine& m) {
    for (const auto& e : m.plugged().byPlace)
        if (e.place == kPrinter) return true;
    return false;
}

/// The PC-100C switch's position; nullopt without a PC-100C.
std::optional<bool> printerSwitch(calcu::machine::Machine& m, const calcu::machine::ControlId& id) {
    auto* c = calcu::machine::capabilityAs<calcu::peripherals::Controls>(m, kControls);
    if (!c) return std::nullopt;
    for (const auto& info : c->list())
        if (info.id == id) return info.on;
    return std::nullopt;
}
} // namespace

// The product's flows of cu59::TIAppPart; the machine control and the
// presets are the library's. The session plumbing is TIAppPart.cpp.

// ── Machine ──────────────────────────────────────────────────────────────────

void TIAppPart::followMachine() {
    calcu::machine::Machine* m = machine();
    // A new machine has nothing fitted until the preset or the
    // constant memory file fits something (the media part cleared its binding).
    if (m_lastFit == Fitted::Card) m_lastFit = Fitted::None;
    m_activeProgram = 0;
    if (!m) {
        m_sampler.reset();
        return;
    }
    // A pause or a step waiting to land runs on in the host call that asked
    // for it (at most 50000 weights), instead of in paced emulated time.
    if (auto* d = m->debugging()) {
        calcu::debugger::RunControl* rc = &m_lib.debugger().runControl();
        static_cast<TIDebugging*>(d)->setRunOn([rc] { return !rc->running() && !rc->paused(); });
    }
    m_sampler.emplace();
    m_model = modelFrom(m->model()).value_or(Model::TI59);

    // The own cue card: a preset's build has
    // none until its `cue-card:` (fitted on this machine already, when the
    // section ran in the same call); a TI-58C built from the constant memory
    // file gets the file's card; any other build none.
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
    calcu::machine::Machine* m = machine();
    if (!m_sampler || !m) return;
    // The module (what the machine says is plugged).
    const std::string module = moduleIn(*m);
    if (newMachine || module != m_moduleID) {
        m_moduleID = module;
        // The module cue cards from the module file.
        m_moduleCards = m_setup.roms.set ? moduleCueCards(*m_setup.roms.set, m_moduleID, [this](const std::string& m) { diagnostic(m); })
                                     : std::map<int, CueCardContent>{};
        refreshCueCard();
    }
    // The printer, and TRACE: the position follows
    // what the machine's PC-100C shows (the library's set-control, a preset's build,
    // printer-trace:) and stays while none is plugged; an attach puts it into
    // the new PC-100C (TIProductMachines::attaching).
    m_printerConnected = printerIn(*m);
    if (m_printerConnected)
        if (const auto on = printerSwitch(*m, PC100C::kTrace)) m_printerTrace = *on;
}

void TIAppPart::machineRestarted(calcu::sessions::SessionContext& ctx, calcu::sessions::Restart r) {
    m_ctx = &ctx;
    if (!m_sampler) return;
    // The paper, the plugs, TRACE and the fitted cards stay (a card in the
    // reader was taken out by the core's place; the ASM overlay's run ends
    // in the core).
    m_activeProgram = 0;
    diagnostic(r == calcu::sessions::Restart::ColdBoot ? "Clean Reset — all registers cleared" : "Calculator Reset");
    refreshCueCard();
}

void TIAppPart::fitOwnCueCard(const CueCardContent& card) {
    refreshCueCard();   // a card fitted before this section is seen first
    m_userCueCard = card;
    m_userCueCardGeneration = m_ms ? m_ms->generation() : 0;
    m_lastFit = Fitted::Own;   // in the place of the card fitted before
    refreshCueCard();
}

// ── 60 Hz frame ──────────────────────────────────────────────────────────────

void TIAppPart::frame(Clock::time_point) {
    calcu::machine::Machine* m = machine();
    if (!m_sampler || !m) return;
    const FrameUpdate f = m_sampler->tick(*m);
    m_display = f.led;
    m_calcOpacity = f.calcIndicatorOpacity;
    refreshCueCard();   // the library program and the fitted card at each call
}

void TIAppPart::refreshCueCard() {
    // The library program selected now, from the product kind.
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
    if (b && b->armed != m_seenArm) {   // a card fitted takes the place of the card before
        m_seenArm = b->armed;
        m_lastFit = Fitted::Card;
        m_userCueCard.reset();
    }
    if (!b && m_lastFit == Fitted::Card) m_lastFit = Fitted::None;
    std::optional<CueCardContent> fitted;
    if (m_lastFit == Fitted::Card && b) fitted = fittedCardCue(magneticCardOf(b->content).get(), b->name);
    else if (m_lastFit == Fitted::Own) fitted = m_userCueCard;
    // The module cue card of the program selected, if the module file defines one.
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

// ── Magnetic cards (the library's media part) ────────────────────────────────

bool TIAppPart::cardFitted() const {
    return m_lib.media().bindingOf("card-reader") != nullptr;
}

bool TIAppPart::tapCard(double nx, double ny) {
    refreshCueCard();
    if (!m_onShow || !m_ctx) return false;   // covered by a module cue card
    const auto side = cardCornerAt(nx, ny);
    if (!side) return false;
    if (auto refused = m_lib.media().feed(*m_ctx, "card-reader", *side)) return false;   // refused while a card is in the reader
    refreshCueCard();
    return true;
}

} // namespace cu59
