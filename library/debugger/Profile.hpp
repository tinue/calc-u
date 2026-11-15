#pragma once
#include <cstddef>
#include <cstdint>
#include <map>
#include <utility>
#include <vector>

// The profile of AC-20 (`profile on|off|report`; LIB-10 design §2): while on,
// how often each address of each target was carried out.

namespace calcu::debugger {

class Profile {
public:
    struct Entry {
        std::size_t target = 0;
        std::uint64_t address = 0;
        std::uint64_t count = 0;
    };
    void setOn(bool on) { on_ = on; }
    bool on() const { return on_; }
    void count(std::size_t target, std::uint64_t address) { ++counts_[{target, address}]; }
    void clear() { counts_.clear(); }
    // The `n` most frequent, the most first; equal counts by target and address.
    std::vector<Entry> top(std::size_t n) const;
    std::size_t distinct() const { return counts_.size(); }

private:
    bool on_ = false;
    std::map<std::pair<std::size_t, std::uint64_t>, std::uint64_t> counts_;
};

} // namespace calcu::debugger
