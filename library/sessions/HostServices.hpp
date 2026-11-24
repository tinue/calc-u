#pragma once
#include "sessions/Value.hpp"
#include <optional>
#include <set>
#include <string>
#include <string_view>

// Host services: what a frontend states it has when it starts to listen. The session asks for a
// service with one message (ServiceAsk) and gets one answer (ServiceAnswer).

namespace calcu::sessions {

enum class HostService { AudioSink, ClipboardText, ClipboardImage, FilePicker, FolderPicker, KeepFile, OpenUrl, Haptics };
inline constexpr HostService kHostServices[] = {HostService::AudioSink,    HostService::ClipboardText,
                                                HostService::ClipboardImage, HostService::FilePicker,
                                                HostService::FolderPicker, HostService::KeepFile,
                                                HostService::OpenUrl,      HostService::Haptics};

// A service by name, as a translating host (the Web worker, the Swift
// bridge) states it: "audio-sink", "clipboard-text", "clipboard-image",
// "file-picker", "folder-picker", "keep-file", "open-url", "haptics".
inline const char* serviceName(HostService s) {
    switch (s) {
    case HostService::AudioSink: return "audio-sink";
    case HostService::ClipboardText: return "clipboard-text";
    case HostService::ClipboardImage: return "clipboard-image";
    case HostService::FilePicker: return "file-picker";
    case HostService::FolderPicker: return "folder-picker";
    case HostService::KeepFile: return "keep-file";
    case HostService::OpenUrl: return "open-url";
    case HostService::Haptics: return "haptics";
    }
    return "";
}
inline std::optional<HostService> serviceNamed(std::string_view name) {
    for (HostService s : kHostServices)
        if (name == serviceName(s)) return s;
    return std::nullopt;
}

struct HostServices {
    std::set<HostService> offered;
    bool recordsSound = false;   // a host that records all sound instead of playing it (the harness)
    // The clipboard may be written without the user's gesture, by a preset
    // step: Qt and iOS. Never the Web, whose browsers write it only in one.
    bool clipboardInSteps = false;
    bool offers(HostService s) const { return offered.count(s) != 0; }
};

struct ServiceAnswer {
    bool given = false;
    Value result;
    std::string reason;   // why it was not given
};

// The reason of an answer the user cancelled (a picker closed, a save
// dialog dismissed): no failure.
inline constexpr std::string_view kCancelled = "cancelled";

// Why the service failed, for the app log; nullopt when it was given or
// the user cancelled it.
inline std::optional<std::string> serviceFailure(const ServiceAnswer& a) {
    if (a.given || a.reason == kCancelled) return std::nullopt;
    return a.reason.empty() ? std::string("the host gives no reason") : a.reason;
}

} // namespace calcu::sessions
