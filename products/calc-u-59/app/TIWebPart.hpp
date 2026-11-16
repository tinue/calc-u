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
// - state topics `c59.web.*`: model, display (the LED cells), cue-card (a
//   draw list at the page's card size), run-state, printer (connected and
//   TRACE; the paper is the library's `paper PC-100C`, M-53), card,
//   settings, debug-font; the debug panel's data are TIDebugPart's product
//   topics (C59-4 AC-25), whose values are the page's forms (M-56);
// - the command `start` (LIB-15 N3: {model, preset}); commands `c59.web.*` for
//   the pointer and the page's key grid (KeyboardLayout's PointerTracker; on-
//   screen keys of the library's typing, LIB-5), the three settings the page offers, cards from the save
//   folder, the sizes the page draws at (a dropped file is LIB-13's `open`, M-56); every other command
//   is TIAppPart's (`c59.*`);
// - requests for card files (downloads) and the fresh card name;
//   adapter W59-1 (WebPageData.hpp): `c59.static-data`, `c59.led-geometry`,
//   `c59.mobile-rules`.
// It lives in the app layer so that it builds and is tested natively; the Web
// engine (frontends/web/bindings.cpp) only adds it to the session. Settings:
// Calc-U-59.ini in the persistent file system (LIB-9 AC-23); cards and
// ti58c.mem: TIAppPart's TIFileAccess on the save folder there (design §4,
// D-9). Traces: LIB-10's trace session, handed over with KeepFile when one
// ends (AC-26; the MEMFS /traces of LIB-9 A-3 went in M-53).

namespace cu59 {

namespace topics {
inline const calcu::sessions::TopicName kWebModel{"c59.web.model"};
inline const calcu::sessions::TopicName kWebDisplay{"c59.web.display"};
inline const calcu::sessions::TopicName kWebCueCard{"c59.web.cue-card"};
inline const calcu::sessions::TopicName kWebRunState{"c59.web.run-state"};
inline const calcu::sessions::TopicName kWebPrinter{"c59.web.printer"};
inline const calcu::sessions::TopicName kWebCard{"c59.web.card"};
inline const calcu::sessions::TopicName kWebSettings{"c59.web.settings"};
inline const calcu::sessions::TopicName kWebDebugFont{"c59.web.debug-font"};
} // namespace topics

class TIWebPart final : public calcu::sessions::SessionPart {
public:
    /// `presetsDir`: the bundled presets (/presets). Traces are the library's,
    /// handed over with KeepFile when they end (LIB-10 AC-26; M-53).
    explicit TIWebPart(settings::Settings& settings);
    ~TIWebPart();

    /// Once, before the session is built: the product's part it draws.
    void bind(TIAppPart& app);

    void attach(calcu::sessions::SessionSetup&) override;
    void afterCall(calcu::sessions::SessionContext&, calcu::pacing::WallTime) override;

private:
    using Value = calcu::sessions::Value;
    /// Once per call: one set per topic of what changed since the last call.
    void drain(calcu::sessions::SessionContext&);
    void apply(const PointerTracker::Change& c);
    void screenKey(int index, bool down, std::uint32_t pointer);
    int heldKey() const;   ///< the last key live input holds (row × 5 + col), -1 none
    static constexpr std::uint32_t kCanvasPointer = 1, kButtonPointer = 2;
    Value cueCard() const;
    Value card() const;
    Value settingsValue() const;
    Value runState() const;
    std::string saveFolder() const;

    settings::Settings& m_settings;
    TIAppPart* m_app = nullptr;
    calcu::sessions::SessionContext* m_ctx = nullptr;
    int m_lastHeld = -1;
    uint32_t m_changes = 0;
    bool m_settingsDirty = true, m_cardsDirty = true, m_cueDirty = true, m_debugFontDirty = false;
    int m_debugFont = 11;
    PointerTracker m_pointer;
    double m_cardW = 0, m_cardH = 0;

    calcu::sessions::StateTopic* m_model = nullptr;
    calcu::sessions::StateTopic* m_display = nullptr;
    calcu::sessions::StateTopic* m_cue = nullptr;
    calcu::sessions::StateTopic* m_run = nullptr;
    calcu::sessions::StateTopic* m_printer = nullptr;
    calcu::sessions::StateTopic* m_card = nullptr;
    calcu::sessions::StateTopic* m_settingsTopic = nullptr;
    calcu::sessions::StateTopic* m_font = nullptr;
};

/// A payload that is its value (the page's own forms).
calcu::sessions::PayloadPtr webPayload(std::string_view kind, calcu::sessions::Value v);

} // namespace cu59
