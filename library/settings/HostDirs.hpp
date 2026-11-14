#pragma once
#include <string>

// What the caller passes when the library starts (LIB-9 AC-12, AC-18;
// design §3): the host's folders. The library decides nothing about the
// platform; a harness or a test passes folders of its own and so touches
// nothing of the user's (AC-18; the harness passes FrontendKind::Qt6, D-11).

namespace calcu::settings {

enum class FrontendKind { Qt6, Swift, Web };

struct HostDirs {
    FrontendKind frontend = FrontendKind::Qt6;
    std::string home;                // `~` in values (AC-33); Qt6: base of the save folder (AC-13)
    std::string settingsBase;        // AC-2: Application Support / %APPDATA% / XDG / the Web file system
    std::string saveFolder;          // Swift: iCloud or local Documents (AC-14); Web: in its file system (AC-15); Qt6: empty
    std::string applicationSupport;  // Swift: traces with [trace] location = app-support (AC-24)
    std::string chosenTraceFolder;   // Swift macOS: the folder the bookmark grants (AC-34); else empty
};

} // namespace calcu::settings
