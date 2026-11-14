#pragma once
#include "sessions/Value.hpp"
#include <set>
#include <string>

// Host services (LIB-3 design §3.6; spec AC-26, AC-27; frontend.md Q-2): what
// a frontend states it has when it starts to listen. The session asks for a
// service with one message (ServiceAsk) and gets one answer (ServiceAnswer).

namespace calcu::sessions {

enum class HostService { AudioSink, ClipboardText, ClipboardImage, FilePicker, FolderPicker, KeepFile, OpenUrl, Haptics };

struct HostServices {
    std::set<HostService> offered;
    bool recordsSound = false;   // a host that records all sound instead of playing it (the harness; AC-20)
    bool offers(HostService s) const { return offered.count(s) != 0; }
};

struct ServiceAnswer {
    bool given = false;
    Value result;
    std::string reason;   // why it was not given
};

} // namespace calcu::sessions
