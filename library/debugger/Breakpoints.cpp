#include "debugger/Breakpoints.hpp"
#include <algorithm>

namespace calcu::debugger {

using machine::DebugTargetId;

std::optional<std::size_t> targetIndex(const machine::Debugging& d, const DebugTargetId& id) {
    for (std::size_t i = 0; i < d.targetCount(); ++i)
        if (d.target(i).info().name == id) return i;
    return std::nullopt;
}

namespace {
std::string noTarget(const DebugTargetId& t) { return "The machine has no debug target " + t.value + "."; }
} // namespace

std::variant<BreakpointId, std::string> BreakpointTable::addBreakpoint(Setter setter, const machine::Debugging& d,
                                                                       const DebugTargetId& target, std::uint64_t address,
                                                                       std::vector<std::string> qualifier, HitTest test) {
    const auto i = targetIndex(d, target);
    if (!i) return noTarget(target);
    const machine::TargetInfo& info = d.target(*i).info();
    if (address >= info.spaces[info.codeSpace].size)
        return "The address is outside the space " + info.spaces[info.codeSpace].name + " of " + target.value + ".";
    if (!qualifier.empty() && qualifier.size() != info.offers.qualifier.size())
        return "The target " + target.value + " takes no such qualifier.";
    for (std::size_t f = 0; f < qualifier.size(); ++f) {
        if (qualifier[f].empty()) continue;
        const auto& values = info.offers.qualifier[f].values;
        if (std::find(values.begin(), values.end(), qualifier[f]) == values.end())
            return "The qualifier field " + info.offers.qualifier[f].name + " takes no value " + qualifier[f] + ".";
    }
    breakpoints_.push_back(Breakpoint{++next_, std::move(setter), target, address, std::move(qualifier), std::move(test)});
    return next_;
}

std::variant<BreakpointId, std::string> BreakpointTable::addWatch(Setter setter, const machine::Debugging& d,
                                                                  const DebugTargetId& target, machine::WatchRange range,
                                                                  HitTest test) {
    const auto i = targetIndex(d, target);
    if (!i) return noTarget(target);
    const machine::TargetInfo& info = d.target(*i).info();
    if (!info.offers.watches) return "The target " + target.value + " offers no watches.";
    if (range.space >= info.spaces.size()) return "The target " + target.value + " has no such space.";
    const machine::SpaceInfo& sp = info.spaces[range.space];
    if (sp.writtenView) return "The view " + sp.name + " cannot be watched.";
    if (range.count == 0 || range.first >= sp.size || range.count > sp.size - range.first)
        return "The range runs past the end of the space " + sp.name + ".";
    if (!range.read && !range.write) return "A watch needs read, write or both.";
    watches_.push_back(Watch{++next_, std::move(setter), target, range, std::move(test)});
    return next_;
}

void BreakpointTable::remove(const Setter& setter, std::optional<BreakpointId> id) {
    auto own = [&](const auto& e) { return e.setter == setter && (!id || e.id == *id); };
    breakpoints_.erase(std::remove_if(breakpoints_.begin(), breakpoints_.end(), own), breakpoints_.end());
    watches_.erase(std::remove_if(watches_.begin(), watches_.end(), own), watches_.end());
}

std::vector<Breakpoint> BreakpointTable::breakpoints(const Setter* s) const {
    std::vector<Breakpoint> out;
    for (const auto& b : breakpoints_)
        if (!s || b.setter == *s) out.push_back(b);
    return out;
}

std::vector<Watch> BreakpointTable::watches(const Setter* s) const {
    std::vector<Watch> out;
    for (const auto& w : watches_)
        if (!s || w.setter == *s) out.push_back(w);
    return out;
}

void BreakpointTable::apply(machine::Debugging& d) const {
    for (std::size_t i = 0; i < d.targetCount(); ++i) {
        machine::DebugTarget& t = d.target(i);
        std::vector<std::uint64_t> addresses;
        for (const auto& b : breakpoints_)
            if (b.target == t.info().name) addresses.push_back(b.address);
        std::sort(addresses.begin(), addresses.end());
        addresses.erase(std::unique(addresses.begin(), addresses.end()), addresses.end());
        t.setBreakpoints(std::move(addresses));
        if (t.info().offers.watches) {
            std::vector<machine::WatchRange> ranges;
            for (const auto& w : watches_)
                if (w.target == t.info().name) ranges.push_back(w.range);
            t.setWatches(std::move(ranges));
        }
    }
}

void BreakpointTable::rebind(machine::Debugging* d) {
    auto gone = [&](const auto& e) {
        if (!d) return true;
        const auto i = targetIndex(*d, e.target);
        return !i.has_value();
    };
    breakpoints_.erase(std::remove_if(breakpoints_.begin(), breakpoints_.end(), gone), breakpoints_.end());
    watches_.erase(std::remove_if(watches_.begin(), watches_.end(), gone), watches_.end());
    // A kept watch whose space the new target lacks, or that runs past its end, goes too.
    if (d)
        watches_.erase(std::remove_if(watches_.begin(), watches_.end(),
                                      [&](const Watch& w) {
                                          const auto& info = d->target(*targetIndex(*d, w.target)).info();
                                          return !info.offers.watches || w.range.space >= info.spaces.size() ||
                                                 w.range.first + w.range.count > info.spaces[w.range.space].size;
                                      }),
                       watches_.end());
    if (d) apply(*d);
}

std::vector<const Breakpoint*> BreakpointTable::at(const DebugTargetId& t, std::uint64_t address) const {
    std::vector<const Breakpoint*> out;
    for (const auto& b : breakpoints_)
        if (b.target == t && b.address == address) out.push_back(&b);
    return out;
}

std::vector<const Watch*> BreakpointTable::hitBy(const DebugTargetId& t, const machine::WatchHit& h) const {
    std::vector<const Watch*> out;
    for (const auto& w : watches_)
        if (w.target == t && w.range.space == h.space && h.address >= w.range.first &&
            h.address < w.range.first + w.range.count && (h.write ? w.range.write : w.range.read))
            out.push_back(&w);
    return out;
}

} // namespace calcu::debugger
