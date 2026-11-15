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
//   draw list at the page's card size), run-state, printer (the lines from
//   the first one that changed), card, live, cpu, heatmap, log, asm, traces,
//   settings, debug-font; stream `c59.web.trace-ready` (a manual trace ended);
//   the debugger tab KEYSTROKES asks for is TIAppPart's `c59.show-debug-panel`;
// - the command `start` (LIB-15 N3: {model, preset}); commands `c59.web.*` for
//   the pointer and the host keys (KeyboardLayout's PointerTracker,
//   KeyboardMap), the three settings the page offers, cards from the save
//   folder, a dropped file, the sizes the page draws at; every other command
//   is TIAppPart's (`c59.*`);
// - requests for card and trace files (downloads) and the fresh card name;
//   adapter W59-1 (WebPageData.hpp): `c59.static-data`, `c59.led-geometry`,
//   `c59.mobile-rules`.
// It lives in the app layer so that it builds and is tested natively; the Web
// engine (frontends/web/bindings.cpp) only adds it to the session. Settings:
// Calc-U-59.ini in the persistent file system (LIB-9 AC-23); cards and
// ti58c.mem: TIAppPart's TIFileAccess on the save folder there (design §4,
// D-9); traces in `traceDir` (MEMFS /traces, LIB-9 A-3 until M-53).

namespace cu59 {

namespace topics {
inline const calcu::sessions::TopicName kWebModel{"c59.web.model"};
inline const calcu::sessions::TopicName kWebDisplay{"c59.web.display"};
inline const calcu::sessions::TopicName kWebCueCard{"c59.web.cue-card"};
inline const calcu::sessions::TopicName kWebRunState{"c59.web.run-state"};
inline const calcu::sessions::TopicName kWebPrinter{"c59.web.printer"};
inline const calcu::sessions::TopicName kWebCard{"c59.web.card"};
inline const calcu::sessions::TopicName kWebLive{"c59.web.live"};
inline const calcu::sessions::TopicName kWebCpu{"c59.web.cpu"};
inline const calcu::sessions::TopicName kWebHeatmap{"c59.web.heatmap"};
inline const calcu::sessions::TopicName kWebLog{"c59.web.log"};
inline const calcu::sessions::TopicName kWebAsm{"c59.web.asm"};
inline const calcu::sessions::TopicName kWebTraces{"c59.web.traces"};
inline const calcu::sessions::TopicName kWebSettings{"c59.web.settings"};
inline const calcu::sessions::TopicName kWebDebugFont{"c59.web.debug-font"};
inline const calcu::sessions::TopicName kWebTraceReady{"c59.web.trace-ready"};   // stream
} // namespace topics

class TIWebPart final : public calcu::sessions::SessionPart {
public:
    /// `presetsDir`: the bundled presets (/presets); `traceDir`: where traces go.
    TIWebPart(settings::Settings& settings, std::string presetsDir, std::filesystem::path traceDir);
    ~TIWebPart();

    /// Once the app session and its TIAppPart exist (makeAppSession).
    void bind(TIAppPart& app);

    void attach(calcu::sessions::SessionSetup&) override;
    void afterCall(calcu::sessions::SessionContext&, calcu::pacing::WallTime) override;

private:
    using Value = calcu::sessions::Value;
    /// Once per call: one set per topic of what changed since the last call.
    void drain(calcu::sessions::SessionContext&);
    void apply(const PointerTracker::Change& c);
    Value cueCard() const;
    Value printer(bool cut);
    Value live() const;
    Value cpu() const;
    Value heatmap() const;
    Value log() const;
    Value card() const;
    Value settingsValue() const;
    Value runState() const;
    Value traces();
    std::string saveFolder() const;

    settings::Settings& m_settings;
    std::string m_presetsDir;
    std::filesystem::path m_traceDir;
    TIAppPart* m_app = nullptr;
    uint32_t m_changes = 0;
    bool m_settingsDirty = true, m_cardsDirty = true, m_cueDirty = true, m_debugFontDirty = false;
    bool m_liveWasOn = false, m_cpuWasOn = false, m_postedFrozen = false, m_traceWasOn = false;
    int m_debugFont = 11;
    PointerTracker m_pointer;
    double m_cardW = 0, m_cardH = 0;
    size_t m_sentLines = 0, m_sentCodes = 0;
    std::vector<std::pair<std::string, uintmax_t>> m_postedTraces;
    bool m_tracesPosted = false;
    std::optional<Model> m_savedModel;

    calcu::sessions::StateTopic* m_model = nullptr;
    calcu::sessions::StateTopic* m_display = nullptr;
    calcu::sessions::StateTopic* m_cue = nullptr;
    calcu::sessions::StateTopic* m_run = nullptr;
    calcu::sessions::StateTopic* m_printer = nullptr;
    calcu::sessions::StateTopic* m_card = nullptr;
    calcu::sessions::StateTopic* m_live = nullptr;
    calcu::sessions::StateTopic* m_cpu = nullptr;
    calcu::sessions::StateTopic* m_heatmap = nullptr;
    calcu::sessions::StateTopic* m_log = nullptr;
    calcu::sessions::StateTopic* m_asm = nullptr;
    calcu::sessions::StateTopic* m_traces = nullptr;
    calcu::sessions::StateTopic* m_settingsTopic = nullptr;
    calcu::sessions::StateTopic* m_font = nullptr;
    calcu::sessions::StreamTopic* m_traceReady = nullptr;
};

/// A payload that is its value (the page's own forms).
calcu::sessions::PayloadPtr webPayload(std::string_view kind, calcu::sessions::Value v);

} // namespace cu59
