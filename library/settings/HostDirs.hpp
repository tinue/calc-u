#pragma once
#include <string>

// What the caller passes when the library starts (LIB-9 AC-12, AC-18;
// design §3): the host's folders. The library decides nothing about the
// platform; a harness or a test passes folders of its own and so touches
// nothing of the user's (AC-18): FrontendKind::Given, every folder as passed
// here (the harness's run folders, LIB-11 D-11; the test rigs).

namespace calcu::settings {

// Given: no platform's rule; the save, trace and screenshot folders are the
// ones passed below (empty: none), no serial link folder.
enum class FrontendKind { Qt6, Swift, Web, Given };

struct HostDirs {
    FrontendKind frontend = FrontendKind::Qt6;
    std::string home;                // `~` in values (AC-33); Qt6: base of the save folder (AC-13)
    std::string settingsBase;        // AC-2: Application Support / %APPDATA% / XDG / the Web file system
    std::string saveFolder;          // Swift: iCloud or local Documents (AC-14); Web: in its file system (AC-15); Given: the folder; Qt6: empty
    std::string applicationSupport;  // Swift: traces with [trace] location = app-support (AC-24)
    std::string chosenTraceFolder;   // Swift macOS: the folder the bookmark grants (AC-34); else empty
    std::string traceFolder;         // Given: the trace folder; empty: none
    std::string screenshotFolder;    // Given: the screenshot folder; empty: none
};

} // namespace calcu::settings
