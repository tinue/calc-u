#include "app/SharpSession.hpp"

#include <memory>

namespace shared {

namespace sessions = calcu::sessions;

sessions::ProductSession<SharpAppPart> makeSharpSession(Settings& settings, AppPaths& paths,
                                                        SharpSessionOptions options) {
    SharpAppPartOptions partOptions;
    partOptions.canMountHostFolder = options.canMountHostFolder;
    auto part = std::make_shared<SharpAppPart>(settings, paths, std::move(partOptions));
    SharpAppPart& p = *part;
    calcu::shell::HostSessionSetup host = std::move(options);
    host.owned = std::move(part);
    return calcu::shell::makeHostSession(p, std::move(host));
}

} // namespace shared
