#include "TIAppPart.hpp"
#include "TIAppPartInternal.hpp"
#include "CardFile.hpp"
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
    m_pass.reset();
    m_fitted.reset();
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

    m_moduleID.clear();
    m_moduleCards.clear();
    m_moduleMetadata = {};
    followPlugs(true);
    refreshCueCard();
    emitChanged(ModelChanged | RunStateChanged | CardChanged | PrinterChanged | CueCardChanged);
}

void TIAppPart::followPlugs(bool newMachine) {
    if (!m_session) return;
    // The module (D-26: what the machine says is plugged).
    const std::string module = m_session->moduleID();
    if (newMachine || module != m_moduleID) {
        m_moduleID = module;
        m_session->refreshModule();
        auto cards = moduleCards(m_setup.cuecards, m_moduleID);
        m_moduleCards = std::move(cards.cards);
        m_moduleMetadata = cards.metadata;
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
    // (the ASM overlay's run ends in the core, C59-4 AC-18).
    m_pass.reset();
    m_activeProgram = 0;
    diagnostic(r == calcu::sessions::Restart::ColdBoot ? "Clean Reset — all registers cleared" : "Calculator Reset");
    refreshCueCard();
    emitChanged(ModelChanged | CardChanged);
}

void TIAppPart::fitOwnCueCard(const CueCardContent& card) {
    m_userCueCard = card;
    m_userCueCardGeneration = m_ms ? m_ms->generation() : 0;
    m_fitted.reset();
    refreshCueCard();
    emitChanged(CardChanged | CueCardChanged);
}

// ── 60 Hz frame ──────────────────────────────────────────────────────────────

void TIAppPart::frame(Clock::time_point now) {
    if (!m_session || !machine()) return;
    uint32_t changes = 0;
    FrameUpdate f = m_session->tick(m_pass.has_value());
    if (f.cardAutoEjected) ejectCard();  // the core ejected at CRD_OFF: save written data

    if (f.led != m_display) {
        m_display = f.led;
        changes |= DisplayChanged;
    }
    if (f.calcIndicatorOpacity != m_calcOpacity) {
        m_calcOpacity = f.calcIndicatorOpacity;
        changes |= DisplayChanged;
    }

    if (now >= m_nextProgramCheck) {  // cue-card program detection, 2 Hz
        m_nextProgramCheck = now + 500ms;
        checkProgramNumber();
    }
    emitChanged(changes);
}

void TIAppPart::checkProgramNumber() {
    const CalculatorState* cs = kindOf<CalculatorState>(m_session->access().machine());
    if (!cs) return;
    int program = activeLibraryProgram(cs->snapshotCPU().SCOM[9]);
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

bool TIAppPart::feedSide(int side) { return feed(side); }

bool TIAppPart::feed(int side) {
    if (!m_session || !m_fitted || side < 0 || side > 1 || m_pass) return false;
    startSwipe(m_fitted->doc.bank(side), {side});
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
    if (!written.empty() && m_fitted) writeFittedSide(pass.side, written);
    emitChanged(CardChanged);
}

void TIAppPart::writeFittedSide(int side, const std::vector<uint8_t>& written) {
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
    if (!m_files.writeCard(m_fitted->path, encodeCardDocument(doc))) {
        error("Card save error", saveFailed);
        return;
    }
    m_fitted->doc = std::move(doc);
    m_fitted->fresh = false;
    refreshCueCard();  // the corner shows the bank just written
}

// ── Opened card files (LIB-13 §10.7, until M-55) ────────────────────────────

std::optional<std::string> TIAppPart::openCard(const calcu::opening::OpenedFile& f) {
    if (!info(m_model).hasCardReader) return "a magnetic card goes into the card reader of a TI-59.";   // AC-12
    if (m_pass) return "the card reader is passing a card.";   // EC-4
    if (!m_session) return std::string(calcu::sessions::kNoMachine);
    // Where it lies (AC-4, AC-12); an upload (the Web) is imported into the
    // save folder once it is fitted, so a bad file never replaces one there.
    const bool upload = !f.path || f.path->empty();
    const std::string path = upload ? m_files.cardFilePath(f.name) : *f.path;
    if (!f.bytes) return "it could not be read.";
    auto decoded = decodeCardFile(*f.bytes, f.name);
    if (!decoded.card) return decoded.error;
    fit(path, f.name, std::move(*decoded.card), false);
    if (upload) m_files.writeCard(path, std::string(f.bytes->begin(), f.bytes->end()));
    return std::nullopt;
}

// ── Debugger ─────────────────────────────────────────────────────────────────

// The run control's state, for the Web page's FREEZE ON START buttons
// (TIWebPart's c59.web.run-state); the panel's data is TIDebugPart's.
void TIAppPart::followRunState() {
    calcu::debugger::RunControl* rc = runControl();
    if (!rc || !m_session || !machine()) return;
    const calcu::debugger::RunState now = rc->state();
    if (!now.same(m_seenRun)) {
        m_seenRun = now;
        emitChanged(RunStateChanged);
    }
}

} // namespace cu59
