#pragma once
#include "app/SharpAppPart.hpp"
#include "sessions/AppSession.hpp"
#include "shell/HostSession.hpp"

namespace shared {

// ── Calc-U-1600's app session for a native host ─────────────────────────
//
// The library's app session with Calc-U-1600's parts (SharpAppPart and the
// peripherals part), for the Qt6 frontend, the tests and tools/wasm_smoke;
// the Web engine builds its own with the shell part and SharpWebPart
// (frontends/web/bindings.cpp), on the same library builder
// (calcu::shell::makeHostSession). The session owns the part, which goes
// first (it saves the bays and the tape into the live machine's files). A
// host talks to it through a calcu::sessions::Listener and makes the host
// call from its own timer. Single-threaded.
//
// The host's side (HostSessionSetup): its settings for LIB-14's SettingsPart
// (Qt6), its own parts, the clock (default: the steady clock), the host time
// (default: the system clock), the hosting; `owned` is the part's.
struct SharpSessionOptions : calcu::shell::HostSessionSetup {
    bool canMountHostFolder = true;   // LIB-4 AC-18: a browser cannot
};

calcu::sessions::ProductSession<SharpAppPart> makeSharpSession(Settings& settings, AppPaths& paths,
                                                               SharpSessionOptions options = {});

} // namespace shared
