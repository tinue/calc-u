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
// The Web frontend on the library's shell (LIB-15 59, M-34; spec AC-19 to
// AC-22): what 2.3.0's WebController posted and offered (frontends/web/
// bindings.cpp at the pin) are product topics, commands and requests of the
// app session, in the page's own form -- the page only draws them, as
// Calc-U-1600's SharpWebPart does for its page:
// - state topics `c59.web.*`, each derived from the value of a product topic
//   and refreshed when that value changed (Part E simplification E-3):
//   display (the LED cells of `c59.calc-indicator`), cue-card (a draw list
//   of `c59.cue-card` at the page's card size), debug-font; the page reads
//   the control state's `controls` (TRACE), the library's `run-state` (FREEZE ON START),
//   `busy` (the speed) and `held-keys` (the key drawn pressed) itself; the
//   debug panel's data are TIDebugPart's product topics (C59-4 AC-25), whose
//   values are the page's forms (M-56);
// - the command `start` (LIB-15 N3: {model, preset}); commands `c59.web.*` for
//   the page's key grid (the pointer is the library's shell rule since M-57;
//   on-screen keys of the library's typing, LIB-5), the three settings the page offers,
//   the sizes the page draws at (a dropped file is LIB-13's `open`, M-56); every other command
//   is TIAppPart's (`c59.*`);
// - the cards are the library's `media` topic and `media.*` commands (LIB-7 59, M-55);
//   adapter W59-1 (WebPageData.hpp): `c59.static-data`, `c59.led-geometry`,
//   `c59.mobile-rules`.
// It lives in the app layer so that it builds and is tested natively; the Web
// engine (frontends/web/bindings.cpp) only adds it to the session. Settings:
// Calc-U-59.ini in the persistent file system (LIB-9 AC-23); cards (the media part) and
// ti58c.mem: TIAppPart's TIFileAccess on the save folder there (design §4,
// D-9). Traces: LIB-10's trace session, handed over with KeepFile when one
// ends (AC-26; the MEMFS /traces of LIB-9 A-3 went in M-53).

namespace cu59 {

namespace topics {
inline const calcu::sessions::TopicName kWebDisplay{"c59.web.display"};
inline const calcu::sessions::TopicName kWebCueCard{"c59.web.cue-card"};
inline const calcu::sessions::TopicName kWebDebugFont{"c59.web.debug-font"};
} // namespace topics

class TIWebPart final : public calcu::sessions::SessionPart {
public:
    /// `presetsDir`: the bundled presets (/presets). Traces are the library's,
    /// handed over with KeepFile when they end (LIB-10 AC-26; M-53).
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
    void screenKey(int index, bool down, std::uint32_t pointer);
    static constexpr std::uint32_t kButtonPointer = 2;
    Value cueCard(const TICueCardState&) const;

    TIAppPart* m_app = nullptr;
    calcu::sessions::SessionContext* m_ctx = nullptr;
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
