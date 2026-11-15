#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "app/AppPaths.hpp"
#include "app/MachineCodeLoad.hpp"
#include "app/MediaLibrary.hpp"
#include "app/Settings.hpp"
#include "app/SharpAppPart.hpp"
#include "sessions/Payload.hpp"
#include "sessions/SessionPart.hpp"

namespace shared {

// ── SharpWebPart: what Calc-U-1600's Web page draws, as session messages ──
//
// The Web frontend on the library's shell (LIB-15 1600, M-31; spec AC-27):
// what 0.8.1's WebController posted and offered (frontends/web/bindings.cpp
// at the pin) are product topics, commands and requests of the app session,
// in the page's own form -- the page only draws them:
// - state topics `c1600.ui` (the control state with its pickers),
//   `c1600.lcd` (the display output as the LCD painter takes it: dots and
//   the indicators laid out), `c1600.lamps` (floppy lamp, tape counter),
//   `c1600.paper` (the plotter strokes at the page's pane width); stream
//   topic `c1600.printer` (the CE-158's printer output);
// - commands `c1600.*` for the keys, typing, the bays, the media and the
//   settings; requests for the page's static data (adapter W16-1:
//   `c1600.faceplate`, `c1600.lcd-constants`, until M-49) and for what a
//   dialog shows. The operations are SharpAppPart's commands.
// It lives in the app layer so that it builds and is tested natively; the
// Web engine (bindings.cpp) only adds it to the session.
class SharpWebPart final : public calcu::sessions::SessionPart {
public:
    SharpWebPart(SharpAppPart& app, Settings& settings, AppPaths& paths, std::string uploadsDir);
    ~SharpWebPart();

    void attach(calcu::sessions::SessionSetup&) override;
    void afterCall(calcu::sessions::SessionContext&, calcu::pacing::WallTime) override;
    void machineChanged(calcu::sessions::SessionContext&) override;
    void operationEnded(calcu::sessions::SessionContext&) override;

private:
    void refresh(calcu::sessions::SessionContext&);
    calcu::sessions::Value ui() const;
    void lcd(calcu::sessions::SessionContext&);
    void lamps();
    void paper(bool force);
    void printer();

    SharpAppPart& m_app;
    Settings& m_settings;
    AppPaths& m_paths;
    std::string m_uploadsDir;
    std::unique_ptr<MediaLibrary> m_library;

    calcu::sessions::StateTopic* m_ui = nullptr;
    calcu::sessions::StateTopic* m_lcd = nullptr;
    calcu::sessions::StateTopic* m_lamps = nullptr;
    calcu::sessions::StateTopic* m_paper = nullptr;
    calcu::sessions::StreamTopic* m_printer = nullptr;

    ProgramChoice m_program;
    std::string m_programName;
    std::optional<MachineCodeLoad> m_code;
    std::uint64_t m_calls = 0;
    double m_paperWidth = 0;
    std::uint64_t m_paperRevision = UINT64_MAX;
    PaperKind m_paperKind = PaperKind::None;
};

namespace topics {
inline const calcu::sessions::TopicName kWebUi{"c1600.ui"};
inline const calcu::sessions::TopicName kWebLcd{"c1600.lcd"};
inline const calcu::sessions::TopicName kWebLamps{"c1600.lamps"};
inline const calcu::sessions::TopicName kWebPaper{"c1600.paper"};
inline const calcu::sessions::TopicName kWebPrinter{"c1600.printer"};
} // namespace topics

} // namespace shared
