#include "debugger/Profile.hpp"
#include <algorithm>

namespace calcu::debugger {

std::vector<Profile::Entry> Profile::top(std::size_t n) const {
    std::vector<Entry> all;
    all.reserve(counts_.size());
    for (const auto& [k, c] : counts_) all.push_back(Entry{k.first, k.second, c});
    std::stable_sort(all.begin(), all.end(), [](const Entry& a, const Entry& b) { return a.count > b.count; });
    if (all.size() > n) all.resize(n);
    return all;
}

} // namespace calcu::debugger
