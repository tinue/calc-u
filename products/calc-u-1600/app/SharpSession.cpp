#include "app/SharpSession.hpp"

#include <memory>

namespace shared {

namespace sessions = calcu::sessions;

sessions::ProductSession<SharpAppPart> makeSharpSession(Settings& settings, AppPaths& paths,
                                                        SharpSessionOptions options) {
    auto part = std::make_shared<SharpAppPart>(settings, paths, SharpAppPartOptions{options.canMountHostFolder});
    sessions::AppSessionSetup setup;
    setup.clock = options.clock ? std::move(options.clock) : calcu::pacing::WallClock(&calcu::pacing::steadyWallTime);
    setup.hostTime = std::move(options.hostTime);
    // LIB-4 D-13: the library's control part among the library's parts.
    setup.parts.push_back(&part->controlPart());
    for (sessions::SessionPart* p : part->sessionParts()) setup.parts.push_back(p);
    setup.hosting = options.hosting;
    sessions::ProductSession<SharpAppPart> out;
    out.part = part.get();
    setup.owned = std::move(part);
    out.session = std::make_unique<sessions::AppSession>(std::move(setup));
    return out;
}

} // namespace shared
