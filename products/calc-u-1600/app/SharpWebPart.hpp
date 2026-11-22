#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "app/AppPaths.hpp"
#include "app/Settings.hpp"
#include "app/SharpAppPart.hpp"
#include "sessions/Payload.hpp"
#include "sessions/SessionPart.hpp"
#include "shell/ModelHost.hpp"

namespace cu1600 {

// ── SharpWebPart: what only Calc-U-1600's Web page asks for ─────────────
//
// The Web page is put together from the React set's shell: menus, bar, settings, notices and "Loading…" are drawn from the
// library's topics (`control-state`, `busy`, `settings`, `media`, ...) and
// send the library's commands. What is left here is the product's own:
// - the stream topic `c1600.printer` (the CE-158's printer output);
// - the requests `c1600.shell-product` (SharpShellProduct, SharpChrome),
//   `c1600.faceplate-map {model}` (the hit map, the LCD's layout),
//   `c1600.inspector-menu`, `c1600.inspector-view`, `c1600.module-yaml`,
//   `c1600.screen-image`;
// - test aids, only with the CMake option CALCU_WEB_TEST_AIDS (on, but off
//   in a release Web build, `frontends/web/build.sh --release`):
//   the command `c1600.upload` (a file into the uploads folder; the
//   Web tests' way to hand over a preset, the page does not send it) and
//   the request `c1600.display-text` (the display as text).
// The tape counter and the floppy lamp are SharpAppPart's `c1600.lamps`.
// It lives in the app layer so that it builds and is tested natively; the
// Web engine (bindings.cpp) only adds it to the session.
class SharpWebPart final : public calcu::sessions::SessionPart {
public:
    SharpWebPart(SharpAppPart& app, std::string uploadsDir, std::string version = {});
    ~SharpWebPart();

    void attach(calcu::sessions::SessionSetup&) override;
    void afterCall(calcu::sessions::SessionContext&, calcu::pacing::WallTime) override;

    // The shell of the page's `shell` message: SharpShellProduct's product
    // and settings dialog.
    calcu::shell::ModelSetup shellSetup() const;

private:
    void printer();

    SharpAppPart& m_app;
    std::string m_uploadsDir;
    std::string m_version;
    calcu::sessions::StreamTopic* m_printer = nullptr;
};

namespace topics {
inline const calcu::sessions::TopicName kWebPrinter{"c1600.printer"};
} // namespace topics

} // namespace cu1600
