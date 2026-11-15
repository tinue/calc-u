#include "app/SharpSession.hpp"

#include <memory>

namespace shared {

namespace sessions = calcu::sessions;

sessions::ProductSession<SharpAppPart> makeSharpSession(Settings& settings, AppPaths& paths,
                                                        SharpSessionOptions options) {
    auto part = std::make_shared<SharpAppPart>(settings, paths);
    sessions::AppSessionSetup setup;
    setup.clock = options.clock ? std::move(options.clock) : calcu::pacing::WallClock([] {
        return std::chrono::duration_cast<calcu::pacing::WallTime>(std::chrono::steady_clock::now().time_since_epoch());
    });
    setup.hostTime = std::move(options.hostTime);
    setup.parts = part->sessionParts();
    setup.hosting = options.hosting;
    sessions::ProductSession<SharpAppPart> out;
    out.part = part.get();
    setup.owned = std::move(part);
    out.session = std::make_unique<sessions::AppSession>(std::move(setup));
    return out;
}

} // namespace shared
