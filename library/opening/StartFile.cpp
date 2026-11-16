#include "opening/StartFile.hpp"

namespace calcu::opening {

std::optional<std::string> startFile(const std::vector<std::string>& arguments,
                                     const std::function<bool(const std::string&)>& isFile) {
    for (std::size_t i = arguments.size(); i > 1; --i) {
        const std::string& a = arguments[i - 1];
        if (!a.empty() && a[0] != '-' && isFile(a)) return a;
    }
    return std::nullopt;
}

} // namespace calcu::opening
