#include "TIWebPart.hpp"
#include "DebugText.hpp"
#include "KeyIds.hpp"
#include "typing/TypingTopics.hpp"
#include "SevenSegment.hpp"
#include "presets/PresetsPart.hpp"
#include "WebPageData.hpp"
#include "TIShellProduct.hpp"
#include "KeyboardLayout.hpp"
#include "RomHeatmap.hpp"
#include "shell/ValueForms.hpp"
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
    r.result = sessions::valuePayload(kind, std::move(v));
    return r;
}

} // namespace

TIWebPart::TIWebPart() = default;

TIWebPart::~TIWebPart() = default;

void TIWebPart::bind(TIAppPart& app) { m_app = &app; }

void TIWebPart::attach(sessions::SessionSetup& s) {
    // TIAppPart's topics (declared before: the part is behind it), drawn in the page's form.
    m_calcIndicator = &s.stateTopic(topics::kCalcIndicator);
    m_cueCard = &s.stateTopic(topics::kCueCard);
    m_display = &s.stateTopic(topics::kWebDisplay);
    m_cue = &s.stateTopic(topics::kWebCueCard);
    m_font = &s.stateTopic(topics::kWebDebugFont);

    auto cmd = [&s](const char* name, std::function<CommandOutcome(const Value&, SessionContext&)> f) {
        s.command(sessions::CommandName{name}, std::move(f));
    };
    auto done = [](auto&& f) {
        return [f](const Value& a, SessionContext&) -> CommandOutcome {
            f(a);
            return Done{};
        };
    };

    // The start is the library's `start {model, preset}` (the model by its
    // name, the preset as a bundle path `examples:<path>`): the page sends it.

    // The page's own key grid (the portrait layout's buttons) and the host
    // keys are the library's typing commands (screen-key-down/-up,
    // host-key-down/-up), which the page sends itself.
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
    // The product's data for the React set's shell, the settings dialog of
    // the layout, from TIShellProduct; the page
    // adds its acts and its version.
    req("c59.shell-product", [](const Value& a) {
        const std::string version = textArg(a, "version");
        const ShellFrontend f = textArg(a, "layout") == "portrait" ? ShellFrontend::WebPortrait : ShellFrontend::WebWide;
        return answer("c59.shell-product",
                      Value::object({{"product", calcu::shell::toValue(shellProduct(version, ""))},
                                     {"settingsSpec", calcu::shell::toValue(settingsSpec(f, "Version " + version))}}));
    });
    // The faceplate of a model: the hit map (base image pixels), the
    // card area and the canvas (faceplateMapValue), and what the page draws
    // with: each key's own rectangle by key ID (the pressed key, the key
    // buttons) and the card's two corners (fractions of the card).
    req("c59.faceplate-map", [](const Value& a) {
        Value v = faceplateMapValue(calcu::machine::ModelId{textArg(a, "model")});
        Value::Array keys;
        for (int row = 0; row < 9; ++row)
            for (int col = 0; col < 5; ++col) {
                const auto id = keyIdAt(matrixCode(row, col));
                if (!id) continue;
                const NRect& k = kKeyRects[row][col];
                keys.push_back(Value::object({{"name", id->value},
                                              {"x", k.x * kCanvasWidth},
                                              {"y", (kKeyboardYStart + k.y * kKeyboardYScale) * kCanvasHeight},
                                              {"w", k.w * kCanvasWidth},
                                              {"h", k.h * kKeyboardYScale * kCanvasHeight}}));
            }
        Value::Array corners;
        for (int side : {0, 1}) {
            const NRect r = cardCornerRect(side);
            corners.push_back(rect(r.x, r.y, r.w, r.h));
        }
        Value::Object o = *v.members();
        o.push_back({"keyRects", Value(std::move(keys))});
        o.push_back({"cardCorners", Value(std::move(corners))});
        return answer("c59.faceplate-map", Value(std::move(o)));
    });
    // The ROM heatmap's grid and colours (the debug panel's CPU tab).
    req("c59.heatmap-grid", [](const Value&) {
        Value::Array colors;   // [r, g, b] per heat step
        for (int i = 0; i < 6; ++i)
            colors.push_back(Value(Value::Array{Value(kHeatColors[i].r), Value(kHeatColors[i].g), Value(kHeatColors[i].b)}));
        return answer("c59.heatmap-grid", Value::object({{"cols", kHeatmapCols}, {"rows", kHeatmapRows}, {"colors", Value(std::move(colors))}}));
    });
    req("c59.led-geometry", [](const Value& a) { return answer("c59.led-geometry", webLedGeometry(realArg(a, "w"), realArg(a, "h"))); });
    req("c59.mobile-rules", [](const Value& a) { return answer("c59.mobile-rules", webMobileRules(realArg(a, "w"), realArg(a, "h"))); });
}

void TIWebPart::afterCall(SessionContext&, calcu::pacing::WallTime) { drain(); }

// The page's forms of the product topics whose value changed since they were
// last drawn (and the cue card when the page's card size changed).
void TIWebPart::drain() {
    if (!m_app || !m_calcIndicator) return;
    auto set = [](sessions::StateTopic* t, std::string_view kind, Value v) { t->set(sessions::valuePayload(kind, std::move(v))); };
    auto changed = [](const sessions::StateTopic& source, sessions::PayloadPtr& drawn) {
        const sessions::PayloadPtr& now = source.current();
        if (!now || (drawn && drawn->equals(*now))) return false;
        drawn = now;
        return true;
    };

    if (changed(*m_calcIndicator, m_calcIndicatorDrawn)) {
        // Per cell (0 = rightmost) its segment mask, brightness and decimal
        // point (0–1), flat: [segments, level, dp] × 12, as float32 bytes.
        const auto& led = *sessions::payloadAs<TICalcIndicatorState>(m_calcIndicatorDrawn);
        const LedFrame& d = led.led;
        float cells[36];
        for (int i = 0; i < 12; ++i) {
            const LedCell cell = ledCell(i, d.segments[i], d.digitLevel[i], led.calcIndicatorOpacity);
            cells[i * 3] = cell.segments;
            cells[i * 3 + 1] = static_cast<float>(cell.opacity);
            cells[i * 3 + 2] = d.dpLevel[i] / 255.0f;
        }
        Value::Bytes raw(sizeof cells);
        std::memcpy(raw.data(), cells, sizeof cells);
        set(m_display, "c59.web.display", Value(std::move(raw)));
    }
    if (changed(*m_cueCard, m_cueDrawn) || (m_cueDirty && m_cueDrawn)) {   // the lit corner too
        m_cueDirty = false;
        set(m_cue, "c59.web.cue-card", cueCard(*sessions::payloadAs<TICueCardState>(m_cueDrawn)));
    }
    if (m_debugFontDirty) {
        m_debugFontDirty = false;
        set(m_font, "c59.web.debug-font", Value(m_debugFont));
    }
}

// ── Cue card: a draw list for m_cardW × m_cardH (CueCardRenderer in Qt) ──────
Value TIWebPart::cueCard(const TICueCardState& state) const {
    const auto& card = state.card;
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
    if (auto lit = state.litCorner) {
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

} // namespace cu59
