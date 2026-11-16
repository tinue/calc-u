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
//   TRACE; the paper is the library's `paper PC-100C`, M-53), live,
//   cpu, heatmap, log, asm, settings, debug-font;
//   the debugger tab KEYSTROKES asks for is TIAppPart's `c59.show-debug-panel`;
// - the command `start` (LIB-15 N3: {model, preset}); commands `c59.web.*` for
//   the pointer and the page's key grid (KeyboardLayout's PointerTracker; on-
//   screen keys of the library's typing, LIB-5), the three settings the page offers,
//   a dropped file, the sizes the page draws at; every other command
//   is TIAppPart's (`c59.*`); the cards are the library's `media` topic and
//   `media.*` commands (LIB-7 59, M-55);
//   adapter W59-1 (WebPageData.hpp): `c59.static-data`, `c59.led-geometry`,
//   `c59.mobile-rules`.
// It lives in the app layer so that it builds and is tested natively; the Web
// engine (frontends/web/bindings.cpp) only adds it to the session. Settings:
// Calc-U-59.ini in the persistent file system (LIB-9 AC-23); cards: the
// library's media part, and ti58c.memory.yaml: TIAppPart's TIFileAccess, on the save folder there (design §4,
// D-9). Traces: LIB-10's trace session, handed over with KeepFile when one
// ends (AC-26; the MEMFS /traces of LIB-9 A-3 went in M-53).

namespace cu59 {

namespace topics {
inline const calcu::sessions::TopicName kWebModel{"c59.web.model"};
inline const calcu::sessions::TopicName kWebDisplay{"c59.web.display"};
inline const calcu::sessions::TopicName kWebCueCard{"c59.web.cue-card"};
inline const calcu::sessions::TopicName kWebRunState{"c59.web.run-state"};
inline const calcu::sessions::TopicName kWebPrinter{"c59.web.printer"};
inline const calcu::sessions::TopicName kWebLive{"c59.web.live"};
inline const calcu::sessions::TopicName kWebCpu{"c59.web.cpu"};
inline const calcu::sessions::TopicName kWebHeatmap{"c59.web.heatmap"};
inline const calcu::sessions::TopicName kWebLog{"c59.web.log"};
inline const calcu::sessions::TopicName kWebAsm{"c59.web.asm"};
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
    Value live() const;
    Value cpu() const;
    Value heatmap() const;
    Value log() const;
    Value settingsValue() const;
    Value runState() const;
    std::string saveFolder() const;

    settings::Settings& m_settings;
    TIAppPart* m_app = nullptr;
    calcu::sessions::SessionContext* m_ctx = nullptr;
    int m_lastHeld = -1;
    uint32_t m_changes = 0;
    bool m_settingsDirty = true, m_cueDirty = true, m_debugFontDirty = false;
    bool m_liveWasOn = false, m_cpuWasOn = false, m_postedFrozen = false;
    int m_debugFont = 11;
    PointerTracker m_pointer;
    double m_cardW = 0, m_cardH = 0;

    calcu::sessions::StateTopic* m_model = nullptr;
    calcu::sessions::StateTopic* m_display = nullptr;
    calcu::sessions::StateTopic* m_cue = nullptr;
    calcu::sessions::StateTopic* m_run = nullptr;
    calcu::sessions::StateTopic* m_printer = nullptr;
    calcu::sessions::StateTopic* m_live = nullptr;
    calcu::sessions::StateTopic* m_cpu = nullptr;
    calcu::sessions::StateTopic* m_heatmap = nullptr;
    calcu::sessions::StateTopic* m_log = nullptr;
    calcu::sessions::StateTopic* m_asm = nullptr;
    calcu::sessions::StateTopic* m_settingsTopic = nullptr;
    calcu::sessions::StateTopic* m_font = nullptr;
};

/// A payload that is its value (the page's own forms).
calcu::sessions::PayloadPtr webPayload(std::string_view kind, calcu::sessions::Value v);

} // namespace cu59
