#pragma once
#include "KeyboardLayout.hpp"
#include "Settings.hpp"
#include "TIAppPart.hpp"
#include "sessions/Payload.hpp"
#include "sessions/SessionPart.hpp"
#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

// ── TIWebPart: what Calc-U-59's Web page draws, as session messages ──────────
//
// The Web frontend on the library's shell: what the page draws are product
// topics, commands and requests of the app session, in the page's own form
// -- the page only draws them, as
// Calc-U-1600's SharpWebPart does for its page:
// - state topics `c59.web.*`, each derived from the value of a product topic
//   and refreshed when that value changed:
//   display (the LED cells of `c59.calc-indicator`), cue-card (a draw list
//   of `c59.cue-card` at the page's card size), debug-font; the page reads
//   the control state's `controls` (TRACE), the library's `run-state` (FREEZE ON START),
//   `busy` (the speed) and `held-keys` (the key drawn pressed) itself; the
//   debug panel's data are TIDebugPart's product topics, whose values are
//   the page's forms;
// - the commands `c59.web.cue-card-size` and `c59.web.debug-width`: the sizes
//   the page draws at (the start is the library's `start`, the page's key grid
//   sends the library's screen-key-down/-up, a dropped file is the library's
//   `open`); every other command is TIAppPart's (`c59.*`);
// - the cards are the library's `media` topic and `media.*` commands;
// - requests for the drawing material: `c59.led-geometry`, `c59.mobile-rules`
//   `c59.faceplate-map`, `c59.heatmap-grid`, `c59.shell-product`.
// It lives in the app layer so that it builds and is tested natively; the Web
// engine (frontends/web/bindings.cpp) only adds it to the session. Settings:
// Calc-U-59.ini in the persistent file system; cards (the media part) and
// the constant memory file: TIAppPart's TIFileAccess on the save folder
// there. Traces: the library's trace session, handed over with KeepFile when
// one ends.

namespace cu59 {

namespace topics {
inline const calcu::sessions::TopicName kWebDisplay{"c59.web.display"};
inline const calcu::sessions::TopicName kWebCueCard{"c59.web.cue-card"};
inline const calcu::sessions::TopicName kWebDebugFont{"c59.web.debug-font"};
} // namespace topics

class TIWebPart final : public calcu::sessions::SessionPart {
public:
    TIWebPart();
    ~TIWebPart();

    /// Once, before the session is built: the product's part it draws.
    void bind(TIAppPart& app);

    void attach(calcu::sessions::SessionSetup&) override;
    void afterCall(calcu::sessions::SessionContext&, calcu::pacing::WallTime) override;

private:
    using Value = calcu::sessions::Value;
    /// Once per call: the page's forms of the product topics whose value changed.
    void drain();
    Value cueCard(const TICueCardState&) const;

    TIAppPart* m_app = nullptr;
    bool m_cueDirty = true, m_debugFontDirty = false;
    int m_debugFont = 11;
    double m_cardW = 0, m_cardH = 0;

    // The product topics drawn (TIAppPart's), and their values last drawn.
    const calcu::sessions::StateTopic* m_calcIndicator = nullptr;
    const calcu::sessions::StateTopic* m_cueCard = nullptr;
    calcu::sessions::PayloadPtr m_calcIndicatorDrawn, m_cueDrawn;

    calcu::sessions::StateTopic* m_display = nullptr;
    calcu::sessions::StateTopic* m_cue = nullptr;
    calcu::sessions::StateTopic* m_font = nullptr;
};

} // namespace cu59
