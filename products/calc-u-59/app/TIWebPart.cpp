#include "TIWebPart.hpp"
#include "DebugText.hpp"
#include "KeyIds.hpp"
#include "typing/TypingTopics.hpp"
#include "RomHeatmap.hpp"
#include "SevenSegment.hpp"
#include "presets/PresetsPart.hpp"
#include "WebPageData.hpp"
#include "settings/AtomicFile.hpp"
#include "sessions/Args.hpp"
#include "sessions/LibraryTopics.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace cu59 {

using namespace calcu;
using sessions::CommandOutcome;
using sessions::Done;
using sessions::Refused;
using sessions::ReplyMessage;
using sessions::SessionContext;
using sessions::StartOperation;
using sessions::Value;
namespace fs = std::filesystem;
using sessions::bytesArg;
using sessions::flagArg;
using sessions::realArg;
using sessions::textArg;
using sessions::truncatedIntegerArg;

namespace {

std::string hexColor(uint32_t rgb) {
    char buf[8];
    std::snprintf(buf, sizeof buf, "#%06x", rgb & 0xFFFFFFu);
    return buf;
}
Value rect(double x, double y, double w, double h) { return Value(Value::Array{Value(x), Value(y), Value(w), Value(h)}); }

ReplyMessage answer(std::string_view kind, Value v) {
    ReplyMessage r;
    r.done = true;
    r.result = webPayload(kind, std::move(v));
    return r;
}

} // namespace

sessions::PayloadPtr webPayload(std::string_view kind, Value v) { return sessions::valuePayload(kind, std::move(v)); }

TIWebPart::TIWebPart(settings::Settings& settings)
    : m_settings(settings) {}

TIWebPart::~TIWebPart() {
    if (m_app) m_app->setChangeObserver(nullptr);
}

void TIWebPart::bind(TIAppPart& app) {
    m_app = &app;
    m_changes = ~0u;
    app.setChangeObserver([this](uint32_t c) { m_changes |= c; });
}

std::string TIWebPart::saveFolder() const { return m_app ? m_app->files().saveFolder() : std::string(); }

void TIWebPart::attach(sessions::SessionSetup& s) {
    using sessions::History;
    m_model = &s.stateTopic(topics::kWebModel);
    m_display = &s.stateTopic(topics::kWebDisplay);
    m_cue = &s.stateTopic(topics::kWebCueCard);
    m_run = &s.stateTopic(topics::kWebRunState);
    m_printer = &s.stateTopic(topics::kWebPrinter);
    m_live = &s.stateTopic(topics::kWebLive);
    m_cpu = &s.stateTopic(topics::kWebCpu);
    m_heatmap = &s.stateTopic(topics::kWebHeatmap);
    m_log = &s.stateTopic(topics::kWebLog);
    m_asm = &s.stateTopic(topics::kWebAsm);
    m_settingsTopic = &s.stateTopic(topics::kWebSettings);
    m_font = &s.stateTopic(topics::kWebDebugFont);
    m_ctx = &s.context();

    auto cmd = [&s](const char* name, std::function<CommandOutcome(const Value&, SessionContext&)> f) {
        s.command(sessions::CommandName{name}, std::move(f));
    };
    auto done = [](auto&& f) {
        return [f](const Value& a, SessionContext&) -> CommandOutcome {
            f(a);
            return Done{};
        };
    };

    // LIB-15 N3, AC-16: the start is LIB-18's `start {model, preset}` (the
    // model by its name, the preset as a bundle path `examples:<path>`;
    // LIB-18 §11.2, LIB-6 §11.2): the page sends it.

    // The pointer on the canvas (canvas-normalized, KeyboardLayout's
    // PointerTracker): keys go through the keyboard path, so a click shorter
    // than the ROM's debounce still holds the key the minimum time.
    cmd("c59.web.pointer-down", done([this](const Value& a) {
        if (!m_app) return;
        // A press, not a drag across the card, feeds a side of the fitted card.
        if (m_app->tapCard(realArg(a, "x"), realArg(a, "y"))) return;
        apply(m_pointer.move(realArg(a, "x"), realArg(a, "y")));
    }));
    cmd("c59.web.pointer-move", done([this](const Value& a) { apply(m_pointer.move(realArg(a, "x"), realArg(a, "y"))); }));
    cmd("c59.web.pointer-up", done([this](const Value&) { apply(m_pointer.up()); }));
    // The page's own key grid (the portrait layout's buttons): an on-screen
    // key of the library's typing (LIB-5 AC-24). Host keys are the library's
    // commands host-key-down / host-key-up, which the page sends itself.
    cmd("c59.web.key-button-down", done([this](const Value& a) {
        screenKey(static_cast<int>(truncatedIntegerArg(a, "row")) * 5 + static_cast<int>(truncatedIntegerArg(a, "col")), true,
                  kButtonPointer);
    }));
    cmd("c59.web.key-button-up", done([this](const Value&) { screenKey(-1, false, kButtonPointer); }));
    // The page lost the focus: the pointer's full speed goes, every key held comes up (AC-10).
    cmd("c59.web.blur", done([this](const Value&) {
        if (!m_app) return;
        const auto c = m_pointer.up();
        if (c.fullSpeed) m_app->setFullSpeed(*c.fullSpeed);
        if (c.release >= 0) screenKey(-1, false, kCanvasPointer);
        if (auto* t = m_app->typing()) t->engine().focusLost();
    }));

    // The settings the page offers (Calc-U-59.ini, LIB-9).
    cmd("c59.web.set-startup-model", done([this](const Value& a) {
        const auto raw = truncatedIntegerArg(a, "model", -1);
        m_settings.setStartupModelChoice(raw >= 0 && raw <= 2 ? std::optional<Model>(static_cast<Model>(raw)) : std::nullopt);
        m_settingsDirty = true;
    }));
    cmd("c59.web.set-trace-max-mb", done([this](const Value& a) {
        m_settings.setTraceMaxFileSizeMB(static_cast<int>(truncatedIntegerArg(a, "mb", 50)));
        m_settingsDirty = true;
    }));
    cmd("c59.web.set-keyboard-feedback", done([this](const Value& a) {
        const auto mode = truncatedIntegerArg(a, "mode");
        m_settings.setKeyboardFeedback(mode == 1   ? settings::KeyboardFeedback::Haptic
                                       : mode == 2 ? settings::KeyboardFeedback::Click
                                                   : settings::KeyboardFeedback::Off);
        m_settingsDirty = true;
    }));

    // A dropped file: a preset loads as an operation (LIB-3 AC-32), a card is
    // fitted as an opened one; anything else is refused with today's sentence.
    cmd("c59.web.drop", [this](const Value& a, SessionContext& ctx) -> CommandOutcome {
        const std::string name = textArg(a, "name");
        if (!m_app) return Refused{"No machine is running."};
        switch (m_app->dropAction(name)) {
            case DropAction::NotAccepted: return Refused{name + ": not a .ti59, .ti58, .ti58c or .magnetcard.yaml file"};
            case DropAction::Ignore: return Done{};
            case DropAction::LoadPreset: {   // LIB-6 §11.2: load-preset {content, name}
                const auto bytes = bytesArg(a, "bytes");
                if (!bytes || !m_app->presetsPart()) return Refused{"Cannot read file."};
                return m_app->presetsPart()->loadPreset(
                    Value::object({{"content", std::string(bytes->begin(), bytes->end())}, {"name", name}}), ctx);
            }
            case DropAction::FitCard: break;
        }
        // C59-7 §3.7: imported into the save folder and fitted (media.open-file, LIB-13 AC-4).
        m_app->fitDroppedCard(name, {}, bytesArg(a, "bytes"));
        return Done{};
    });

    // The sizes the page draws at.
    cmd("c59.web.cue-card-size", done([this](const Value& a) {
        m_cardW = realArg(a, "width");
        m_cardH = realArg(a, "height");
        m_cueDirty = true;
    }));
    cmd("c59.web.debug-width", done([this](const Value& a) {
        m_debugFont = debugPanelFontSize(realArg(a, "width"));
        m_debugFontDirty = true;
    }));

    auto req = [&s](const char* name, std::function<ReplyMessage(const Value&)> f) {
        s.request(sessions::RequestName{name}, [f = std::move(f)](const Value& a, SessionContext&) { return f(a); });
    };
    req("c59.static-data", [this](const Value&) {
        return answer("c59.static-data", webStaticData(m_app ? m_app->allModules() : std::vector<ModuleMetadata>{}));
    });
    req("c59.led-geometry", [](const Value& a) { return answer("c59.led-geometry", webLedGeometry(realArg(a, "w"), realArg(a, "h"))); });
    req("c59.mobile-rules", [](const Value& a) { return answer("c59.mobile-rules", webMobileRules(realArg(a, "w"), realArg(a, "h"))); });
}

void TIWebPart::apply(const PointerTracker::Change& c) {
    if (!m_app) return;
    if (c.fullSpeed) m_app->setFullSpeed(*c.fullSpeed);
    if (c.release >= 0) screenKey(c.release, false, kCanvasPointer);
    if (c.press >= 0) screenKey(c.press, true, kCanvasPointer);
}

// An on-screen key by its index (row × 5 + column) through the typing
// engine; a press is dropped while an operation runs (LIB-5 E11), a release
// always passes.
void TIWebPart::screenKey(int index, bool down, std::uint32_t pointer) {
    calcu::typing::TypingPart* t = m_app ? m_app->typing() : nullptr;
    if (!t) return;
    if (!down) {
        t->engine().screenKeyUp(calcu::machine::KeyId{}, pointer);
        return;
    }
    if (index < 0 || index >= 45 || (m_ctx && m_ctx->busy())) return;
    if (const auto id = keyIdAt(matrixCode(index / 5, index % 5))) t->engine().screenKeyDown(*id, pointer);
}

int TIWebPart::heldKey() const {
    calcu::typing::TypingPart* t = m_app ? m_app->typing() : nullptr;
    if (!t) return -1;
    const auto held = t->engine().liveHeld();
    if (held.empty()) return -1;
    const auto code = matrixCodeOf(held.back());
    return code ? (*code / 10 - 1) * 5 + (*code % 10 - 1) : -1;
}

void TIWebPart::afterCall(SessionContext& ctx, calcu::pacing::WallTime) { drain(ctx); }

// One post per kind of change, as 2.3.0's WebController::drain().
void TIWebPart::drain(SessionContext&) {
    if (!m_app) return;
    uint32_t c = std::exchange(m_changes, 0u);
    using S = TIAppPart;
    if (const int h = heldKey(); h != m_lastHeld) {   // the key drawn pressed: the typing's held-keys
        m_lastHeld = h;
        c |= S::RunStateChanged;
    }
    auto set = [](sessions::StateTopic* t, std::string_view kind, Value v) { t->set(webPayload(kind, std::move(v))); };

    if (c & S::ModelChanged) {
        set(m_model, "c59.web.model", Value::object({{"model", static_cast<int>(m_app->model())}, {"moduleID", m_app->moduleID()}}));
        // The model used last (LIB-9: the startup model "last used").
    }
    if (c & (S::DisplayChanged | S::ModelChanged)) {
        // Per cell (0 = rightmost) its segment mask, brightness and decimal
        // point (0–1), flat: [segments, level, dp] × 12, as float32 bytes.
        const LedFrame& d = m_app->display();
        const double calc = m_app->calcIndicatorOpacity();
        float cells[36];
        for (int i = 0; i < 12; ++i) {
            const LedCell cell = ledCell(i, d.segments[i], d.digitLevel[i], calc);
            cells[i * 3] = cell.segments;
            cells[i * 3 + 1] = static_cast<float>(cell.opacity);
            cells[i * 3 + 2] = d.dpLevel[i] / 255.0f;
        }
        Value::Bytes raw(sizeof cells);
        std::memcpy(raw.data(), cells, sizeof cells);
        set(m_display, "c59.web.display", Value(std::move(raw)));
    }
    if ((c & (S::CueCardChanged | S::CardChanged | S::ModelChanged)) || m_cueDirty) {   // the lit corner too
        m_cueDirty = false;
        set(m_cue, "c59.web.cue-card", cueCard());
    }
    if (c & (S::RunStateChanged | S::ModelChanged | S::AsmChanged)) set(m_run, "c59.web.run-state", runState());
    if (c & (S::PrinterChanged | S::ModelChanged))   // LIB-8 P-E1: the paper is the library's `paper PC-100C`
        set(m_printer, "c59.web.printer",
            Value::object({{"connected", m_app->printerConnected()}, {"trace", m_app->printerTrace()}}));
    // Freezing switches the step window to the full listing and the CPU rows
    // to every row's registers; a panel switched on gets its view at once.
    const bool freezeFlipped = m_app->isFrozen() != m_postedFrozen;
    m_postedFrozen = m_app->isFrozen();
    const bool liveOn = m_app->liveDebugEnabled() && !m_liveWasOn, cpuOn = m_app->cpuDebugEnabled() && !m_cpuWasOn;
    m_liveWasOn = m_app->liveDebugEnabled();
    m_cpuWasOn = m_app->cpuDebugEnabled();
    if (((c & (S::LiveChanged | S::ListingChanged)) || freezeFlipped || liveOn) && m_app->liveDebugEnabled())
        set(m_live, "c59.web.live", live());
    if (((c & S::CpuChanged) || freezeFlipped || cpuOn) && m_app->cpuDebugEnabled()) set(m_cpu, "c59.web.cpu", cpu());
    if (((c & S::HeatmapChanged) || cpuOn) && m_app->cpuDebugEnabled()) set(m_heatmap, "c59.web.heatmap", heatmap());
    if (c & S::LogChanged) set(m_log, "c59.web.log", log());
    if (c & S::AsmChanged)
        set(m_asm, "c59.web.asm",
            Value::object({{"fileName", m_app->asmFileName()}, {"words", m_app->asmWordCount()},
                           {"status", m_app->asmStatus()}, {"active", m_app->asmActive()}}));
    if (m_settingsDirty) {
        m_settingsDirty = false;
        set(m_settingsTopic, "c59.web.settings", settingsValue());
    }
    if (m_debugFontDirty) {
        m_debugFontDirty = false;
        set(m_font, "c59.web.debug-font", Value(m_debugFont));
    }
}

Value TIWebPart::settingsValue() const {
    const auto chosen = m_settings.startupModelChoice();
    return Value::object({{"startupModel", chosen ? static_cast<int>(*chosen) : -1},
                          {"traceMaxMB", m_settings.traceMaxFileSizeMB()},
                          {"keyboardFeedback", static_cast<int>(m_settings.keyboardFeedback())}});
}

// The run control's state as the page's FREEZE ON START buttons read it
// (LIB-10: FREEZE, STEP and RESUME follow the library's `run-state`).
Value TIWebPart::runState() const {
    const calcu::debugger::RunState d = m_app->runState();
    return Value::object({{"frozen", d.paused},
                          {"owner", !d.owner ? Value() : Value(*d.owner == calcu::debugger::kCpu ? "cpu" : "calculator")},
                          {"pcArmed", d.armed == calcu::debugger::kProgram},
                          {"scanArmed", d.armed == calcu::debugger::kCpu},
                          {"fullSpeed", m_app->fullSpeed()},
                          {"heldKey", heldKey()},
                          {"asmActive", m_app->asmActive()}});
}

Value TIWebPart::log() const {
    return Value::object({{"text", m_app->logText()}, {"level", static_cast<int>(m_app->debugLevel())}});
}

// ── Cue card: a draw list for m_cardW × m_cardH (CueCardRenderer in Qt) ──────
Value TIWebPart::cueCard() const {
    const auto& card = m_app->cueCard();
    if (!card || m_cardW <= 0) return Value();
    const double w = m_cardW, h = m_cardH;
    const CueCardLayout& L = cueCardLayout(card->template_);
    const CueCardGeometry g = cueCardGeometry(*card, w, h);
    const std::string ink = hexColor(cueCardInk(*card));
    const bool boxed = card->style == CardButtonStyle::Button;

    Value::Array items;
    auto textItem = [&](const std::string& s, double size, const char* align, double cx, double cy, double width,
                        bool box = false) {
        if (s.empty()) return;
        Value::Object t{{"kind", Value("text")}, {"text", Value(s)},        {"size", Value(size)},
                        {"align", Value(align)}, {"cx", Value(cx)},         {"cy", Value(cy)},
                        {"width", Value(width)}, {"color", Value(ink)}};
        if (box) t.push_back({"box", rect(g.buttonPadX, g.buttonPadY, 0, 0)});
        items.emplace_back(std::move(t));
    };
    auto badge = [&](int bank, const CueCardFrame& f) {
        items.push_back(Value::object({{"kind", "badge"},
                                       {"text", std::to_string(bank)},
                                       {"size", g.bankFontSize},
                                       {"frame", rect(f.cx - f.width / 2, f.cy - f.height / 2, f.width, f.height)}}));
    };
    auto rectItem = [&](double x, double y, double rw, double rh, const std::string& color, double alpha) {
        items.push_back(Value::object({{"kind", "rect"}, {"rect", rect(x, y, rw, rh)}, {"color", color}, {"alpha", alpha}}));
    };

    if (L.hasId) {
        textItem(card->title, g.titleFontSize, "left", g.gridCenter, g.titleY, g.gridWidth);
        textItem(card->id, g.titleFontSize, "right", g.gridCenter, g.titleY, g.gridWidth);
    } else {
        textItem(card->title, g.titleFontSize, "center", w / 2, g.titleY, g.titleWidth);
    }
    if (card->template_ == CueCardTemplate::MagnetCard) {
        if (card->bankLeft) badge(*card->bankLeft, g.bankLeft);
        if (card->bankRight) badge(*card->bankRight, g.bankRight);
    }
    auto gridRow = [&](int row, double y) {
        auto spans = labelSpans(*card, row);
        if (spans.empty()) return;
        double fs = cellFittingFontSize(spans, L, w, g.gridFontSize);
        if (L.drawsDividers)
            for (int i : dividersBetween(spans))
                rectItem(cueCardDividerX(L, i, w) - 0.5, y - g.dividerHeight / 2, 1, g.dividerHeight,
                         hexColor(kCueCardDividerColor), 1);
        for (const auto& sp : spans) {
            CueCardFrame f = cueCardSpanFrame(L, sp, y, w);
            textItem(sp.label, fs, "center", f.cx, f.cy, f.width);
        }
    };
    // Top row: Row1 text, or the A′–E′ grid.
    if (!card->row1.empty())
        textItem(card->row1, g.gridFontSize, toString(card->row1Align), g.gridCenter, g.gridY0, g.gridWidth, boxed);
    else gridRow(0, g.gridY0);
    // Bottom row: Row2 / Row2R text, or the A–E grid.
    if (!card->row2.empty() || !card->row2R.empty()) {
        if (card->row2R.empty()) {
            textItem(card->row2, g.gridFontSize, toString(card->row2Align), g.gridCenter, g.gridY1, g.gridWidth, boxed);
        } else {
            textItem(card->row2, g.gridFontSize, toString(card->row2Align), g.row2Left.cx, g.row2Left.cy,
                     g.row2Left.width, boxed);
            textItem(card->row2R, g.gridFontSize, toString(card->row2RAlign), g.row2Right.cx, g.row2Right.cy,
                     g.row2Right.width, boxed);
        }
    } else {
        gridRow(1, g.gridY1);
    }
    rectItem(0, 0, w, g.washHeight, hexColor(kCueCardWashColor), kCueCardWashAlpha);
    if (auto lit = m_app->litCardCorner()) {
        const CardCornerGeometry cg = cardCornerGeometry(*lit, w, h);
        Value::Array arrow;
        for (const auto& pt : cg.arrow) {
            arrow.emplace_back(pt.x);
            arrow.emplace_back(pt.y);
        }
        items.push_back(Value::object({{"kind", "corner"},
                                       {"rect", rect(cg.x, cg.y, cg.width, cg.height)},
                                       {"radius", cg.radius},
                                       {"side", *lit},
                                       {"color", hexColor(kCardCornerLitRGB)},
                                       {"arrow", Value(std::move(arrow))}}));
        const auto& bank = *lit == 0 ? card->bankLeft : card->bankRight;
        if (bank) badge(*bank, *lit == 0 ? g.bankLeft : g.bankRight);
    }
    // Plain text of the card, for assistive technology and tests.
    std::string label = card->title;
    for (const auto& l : card->labels)
        if (!l.empty() && l != kCueCardInvisibleMarker) label += " " + l;
    for (const auto* s : {&card->row1, &card->row2, &card->row2R})
        if (!s->empty()) label += " " + *s;
    return Value::object({{"template", std::string(toString(card->template_))},
                          {"width", w},
                          {"height", h},
                          {"items", Value(std::move(items))},
                          {"label", label}});
}

// ── Debugger rows, formatted as Qt6/app/debug/*Widget do ─────────────────────
Value TIWebPart::live() const {
    const LiveDebugSnapshot& s = m_app->debug().live();
    char buf[64];
    Value::Object o;
    std::snprintf(buf, sizeof buf, "FA:%04X", s.fA);
    o.push_back({"fA", Value(std::string(buf))});
    std::snprintf(buf, sizeof buf, "FB:%04X", s.fB);
    o.push_back({"fB", Value(std::string(buf))});
    o.push_back({"hir0", Value("HIR0:" + s.statusRegister)});
    o.push_back({"partition", Value(partitionLine(s.programRegCount, s.dataRegCount))});
    o.push_back({"source", Value(programSourceLabel(s.programSource, s.prSourceFlag))});
    // Program steps: the full listing when frozen, else ±5 around the current step.
    const bool frozen = m_app->isFrozen() && !m_app->debug().listing().empty();
    const auto& entries = frozen ? m_app->debug().listing() : s.programWindow;
    Value::Array steps;
    for (const auto& e : entries) steps.push_back(Value::object({{"text", programStepLine(e, frozen)}, {"current", e.isCurrent}}));
    o.push_back({"steps", Value(std::move(steps))});
    o.push_back({"stepHighlight",
                 Value(hexColor(stepHighlightColor(frozen ? m_app->debug().listingSource() : s.programSource)))});
    o.push_back({"fix", Value("FIX:" + s.fixIndicator)});
    o.push_back({"angle", Value(angleModeLabel(s.angleMode))});
    o.push_back({"angleKnown", Value(s.angleMode.has_value())});
    o.push_back({"eng", Value(!s.engIndicator.empty())});
    o.push_back({"second", Value(!s.secondIndicator.empty())});
    o.push_back({"inv", Value(!s.invIndicator.empty())});
    Value::Array returns;
    for (int i = 0; i < 6; ++i) {
        const int a = s.returnAddresses[static_cast<size_t>(i)];
        returns.push_back(Value::object({{"text", returnLevelLine(i + 1, a)},
                                         {"color", hexColor(returnLevelColor(s.returnAddressSourceFlags[static_cast<size_t>(i)], a == 0))}}));
    }
    o.push_back({"returns", Value(std::move(returns))});
    Value::Array flags;
    for (int i = 0; i < 10; ++i) flags.emplace_back(static_cast<bool>(s.calcFlags[static_cast<size_t>(i)]));
    o.push_back({"flags", Value(std::move(flags))});
    Value::Array hir;
    for (int i = 0; i < 8; ++i)
        hir.push_back(Value::object({{"text", hirLine(i + 1, s.hir[static_cast<size_t>(i)])}, {"zero", s.hir[static_cast<size_t>(i)] == 0}}));
    o.push_back({"hir", Value(std::move(hir))});
    o.push_back({"x", Value(xRegisterLine(s.xValue, s.xRaw))});
    o.push_back({"xColor", Value(hexColor(xRegisterColor(s.xValue, s.xRaw)))});
    o.push_back({"t", Value(tRegisterLine(s.tRegister))});
    o.push_back({"tZero", Value(s.tRegister == 0)});
    Value::Array vars;
    for (const auto& r : s.nonZeroRegs) vars.emplace_back(registerLine(r));
    o.push_back({"vars", Value(std::move(vars))});
    return Value(std::move(o));
}

// The CPU rows; the registers only for the rows the panel can show — the
// newest while running, any when frozen.
Value TIWebPart::cpu() const {
    const auto& history = m_app->debug().cpuHistory();
    const bool frozen = m_app->isFrozen();
    char buf[64];
    Value::Array rows;
    for (size_t i = 0; i < history.size(); ++i) {
        const auto& e = history[i];
        const CpuFrame& f = e.frame;
        Value::Object r{{"text", Value(inspectorLine(e))},
                        {"pc", Value(e.pc)},
                        {"history", Value(e.isHistory)},
                        {"current", Value(e.isCurrent)}};
        if (frozen || i + 1 == history.size()) {
            Value::Array regs;
            const std::pair<const char*, const uint8_t*> named[] = {
                {"A", f.A}, {"B", f.B}, {"C", f.C}, {"D", f.D}, {"E", f.E}, {"Sout", f.Sout}};
            for (const auto& [name, nib] : named) regs.push_back(Value(Value::Array{Value(name), Value(registerNibbles(nib))}));
            r.push_back({"registers", Value(std::move(regs))});
            r.push_back({"KR", Value("KR: " + hexBin16(f.KR))});
            r.push_back({"fA", Value("fA: " + hexBin16(f.fA))});
            r.push_back({"fB", Value("fB: " + hexBin16(f.fB))});
            std::snprintf(buf, sizeof buf, "SR: %04X", f.SR);
            r.push_back({"SR", Value(std::string(buf))});
            r.push_back({"R5", Value("R5: " + std::to_string(f.R5))});
            r.push_back({"cond", Value(condFlag(f.flags))});
            r.push_back({"idle", Value(idleFlag(f.flags))});
            std::snprintf(buf, sizeof buf, "DIGIT: %2d", f.postDigit);
            r.push_back({"digit", Value(std::string(buf))});
            r.push_back({"lit", Value("Lit: " + std::to_string(f.maxDigitDecay))});
            r.push_back({"displayOn", Value(f.displayOn != 0)});
        }
        rows.emplace_back(std::move(r));
    }
    return Value(std::move(rows));
}

Value TIWebPart::heatmap() const {
    const auto& counts = m_app->debug().heatmapCounts();
    Value::Bytes steps(counts.size());
    for (size_t i = 0; i < counts.size(); ++i) steps[i] = heatStep(counts[i]);
    return Value(std::move(steps));
}

} // namespace cu59
