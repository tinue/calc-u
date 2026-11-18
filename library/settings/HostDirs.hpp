#pragma once
#include <functional>
#include <string>

// What the caller passes when the library starts: the host's folders. The
// library decides nothing about the platform; a harness or a test passes
// folders of its own and so touches nothing of the user's:
// FrontendKind::Given, every folder as passed here (the harness's run
// folders; the test rigs).

namespace calcu::settings {

// Given: no platform's rule; the save, trace and screenshot folders are the
// ones passed below (empty: none), no serial link folder.
enum class FrontendKind { Qt6, Swift, Web, Given };

struct HostDirs {
    FrontendKind frontend = FrontendKind::Qt6;
    std::string home;                // `~` in values; Qt6: base of the save folder
    std::string settingsBase;        // Application Support / %APPDATA% / XDG / the Web file system
    std::string saveFolder;          // Swift: iCloud or local Documents; Web: in its file system; Given: the folder; Qt6: empty
    std::string applicationSupport;  // Swift: traces with [trace] location = app-support
    // Swift macOS: the folder the bookmark grants, asked each time
    // the trace folder is (at each trace start: the sandbox's
    // grant is the frontend's); unset or empty: none.
    std::function<std::string()> chosenTraceFolder;
    // Swift iOS: whether iCloud can be used now (signed in), asked at each
    // trace start where [trace] location = icloud:
    // while it says no, ensure(Folder::Traces) refuses; unset: always.
    std::function<bool()> iCloudAvailable;
    std::string traceFolder;         // Given: the trace folder; empty: none
    std::string screenshotFolder;    // Given: the screenshot folder; empty: none
};

} // namespace calcu::settings
