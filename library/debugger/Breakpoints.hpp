#pragma once
#include "machine/DebugTarget.hpp"
#include "machine/Ids.hpp"
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <variant>
#include <vector>

// Breakpoints and watches (LIB-10 design §3.5; spec AC-10 to AC-12, AC-15,
// AC-47, EC-3). Each belongs to its setter ("harness", "dap"; D-13) and has
// an ID the table gives, never reused in a session. A test of the setter
// (AC-12) is asked at a hit; the library sets none. The table hands each
// target the addresses and ranges of every setter; the run control sorts
// the hits out (G6, G7).

namespace calcu::debugger {

using Setter = std::string;
using BreakpointId = std::uint32_t;
using HitTest = std::function<bool(const machine::DebugTarget&)>;   // true: pause (AC-12)

inline const Setter kHarnessSetter = "harness";

struct Breakpoint {
    BreakpointId id = 0;
    Setter setter;
    machine::DebugTargetId target;
    std::uint64_t address = 0;
    std::vector<std::string> qualifier;   // a value per field of the target's qualifier, "" free; empty: none
    HitTest test;
};

struct Watch {
    BreakpointId id = 0;
    Setter setter;
    machine::DebugTargetId target;
    machine::WatchRange range;
    HitTest test;
};

class BreakpointTable {
public:
    // The ID, or the reason it is refused: a target the machine does not
    // have, an address outside the target's code space, a qualifier that is
    // not the target's (§3.9).
    std::variant<BreakpointId, std::string> addBreakpoint(Setter, const machine::Debugging&, const machine::DebugTargetId&,
                                                          std::uint64_t address, std::vector<std::string> qualifier,
                                                          HitTest = {});
    // EC-3: a range past the end of its space is refused; so is a target
    // without watches (AC-11) and a written view (AC-48: read-only).
    std::variant<BreakpointId, std::string> addWatch(Setter, const machine::Debugging&, const machine::DebugTargetId&,
                                                     machine::WatchRange, HitTest = {});
    // Only the setter's own (AC-47); without an ID every one of them.
    void remove(const Setter&, std::optional<BreakpointId> = {});
    std::vector<Breakpoint> breakpoints(const Setter* = nullptr) const;
    std::vector<Watch> watches(const Setter* = nullptr) const;
    bool anyBreakpoints() const { return !breakpoints_.empty(); }
    bool anyWatches() const { return !watches_.empty(); }
    // Per target: setBreakpoints and setWatches.
    void apply(machine::Debugging&) const;
    // A new machine (AC-15): those whose target name is there are kept and
    // set again, the rest dropped. nullptr: the machine offers no targets.
    void rebind(machine::Debugging*);

    // The breakpoints of target `t` (by name) at `address`, and the watches
    // of `t` whose range holds a hit of their kind.
    std::vector<const Breakpoint*> at(const machine::DebugTargetId& t, std::uint64_t address) const;
    std::vector<const Watch*> hitBy(const machine::DebugTargetId& t, const machine::WatchHit&) const;

private:
    BreakpointId next_ = 0;
    std::vector<Breakpoint> breakpoints_;
    std::vector<Watch> watches_;
};

// The index of the target of that name, or none.
std::optional<std::size_t> targetIndex(const machine::Debugging&, const machine::DebugTargetId&);

} // namespace calcu::debugger
