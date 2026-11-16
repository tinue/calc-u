#pragma once
#include <chrono>
#include <functional>

#include "app/SharpAppPart.hpp"
#include "sessions/AppSession.hpp"

namespace shared {

// ── Calc-U-1600's app session for a native host ─────────────────────────
//
// The library's app session with Calc-U-1600's parts (SharpAppPart and the
// peripherals part), for the Qt6 frontend, the tests and tools/wasm_smoke;
// the Web engine builds its own with the shell part and SharpWebPart
// (frontends/web/bindings.cpp). The session owns the part, which goes first
// (it saves the bays and the tape into the live machine's files). A host
// talks to it through a calcu::sessions::Listener and makes the host call
// from its own timer. Single-threaded.
struct SharpSessionOptions {
    calcu::sessions::Hosting hosting = calcu::sessions::Hosting::Paced;
    calcu::pacing::WallClock clock;                                      // default: the steady clock
    std::function<std::chrono::system_clock::time_point()> hostTime;    // default: the system clock
    bool canMountHostFolder = true;                                     // LIB-4 AC-18: a browser cannot
};

calcu::sessions::ProductSession<SharpAppPart> makeSharpSession(Settings& settings, AppPaths& paths,
                                                               SharpSessionOptions options = {});

} // namespace shared
