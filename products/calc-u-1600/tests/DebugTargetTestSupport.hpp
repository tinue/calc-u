#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "../core/CPU/WatchSet.hpp"
#include "../core/Debug/SharpDebugging.hpp"
#include "../core/PC1500/PC1500Machine.hpp"
#include "../core/PC1600/PC1600Machine.hpp"

// The debug-target tests on the
// Sharp machines' machine::Debugging (core/Debug/SharpDebugging) without a
// session. Target drives it with threads from 1, breakpoints and watches
// armed while set, a free run of a budget and a resume past the breakpoint
// the bus owner stands on, through the interface only: setChecks(), run(),
// takeStop(), passOnce() and the observer.

namespace debugtest {

struct Stop {
    enum Kind : std::uint8_t { None, Breakpoint, Watch, Budget };
    Kind kind = None;
    int thread = 0;   // 1, and 2 = the PC-1600's LH5803
    struct {
        std::uint16_t addr = 0;
        bool write = false;
        std::uint8_t space = 0;
    } hit;
};

class Target {
public:
    explicit Target(PC1500Machine& m) : m_d(sharp::makeSharpDebugging(m)) {}
    explicit Target(PC1600Machine& m) : m_d(sharp::makeSharpDebugging(m)) {}

    sharp::SharpDebugging& debugging() { return *m_d; }
    int threadCount() const { return static_cast<int>(m_d->targetCount()); }
    int busOwner() const { return static_cast<int>(m_d->current()) + 1; }
    sharp::SharpDebugTarget& t(int thread) const {
        return m_d->sharpTarget(thread == 1 ? 0 : m_d->targetCount() - 1);
    }

    std::uint16_t pc(int thread) const { return t(thread).pc(); }
    std::uint64_t retired(int thread) const { return t(thread).retired(); }
    bool readRegister(int thread, const std::string& name, std::uint32_t* v) const { return t(thread).readRegister(name, v); }
    bool writeRegister(int thread, const std::string& name, std::uint32_t v) { return t(thread).writeRegister(name, v); }
    bool peek(int thread, debug::Space s, std::uint16_t a, std::uint8_t* v) const { return t(thread).peekByte(s, a, v); }
    bool poke(int thread, debug::Space s, std::uint16_t a, std::uint8_t v) { return t(thread).pokeByte(s, a, v); }

    // Armed while set: the checks follow the lists.
    void setBreakpoints(int thread, const std::vector<std::uint16_t>& addrs) {
        t(thread).setBreakpoints(std::vector<std::uint64_t>(addrs.begin(), addrs.end()));
        m_bps[thread == 1 ? 0 : 1] = !addrs.empty();
        arm();
    }
    void setWatches(int thread, const std::vector<WatchSet::Watch>& watches) {
        std::vector<calcu::machine::WatchRange> ranges;
        for (const auto& w : watches)
            ranges.push_back({w.space, w.lo, static_cast<std::uint64_t>(w.hi - w.lo) + 1, w.read, w.write});
        t(thread).setWatches(ranges);
        m_watches[thread == 1 ? 0 : 1] = !watches.empty();
        arm();
    }
    bool breakpointsActive() const { return m_bps[0] || m_bps[1]; }

    // A free run of up to `budget` ticks after a resume.
    Stop runFrom(std::uint64_t budget) {
        m_d->passOnce(m_d->current());
        arm();
        m_d->run(budget);
        return stopOf(m_d->takeStop());
    }
    // Machine steps until `thread` has carried out an instruction (the other
    // CPU of a PC-1600 runs meanwhile when it has the bus), at most
    // `maxSteps` instructions of any CPU.
    Stop stepInstruction(int thread, std::uint64_t maxSteps) {
        struct Observer final : calcu::machine::InstructionObserver {
            std::size_t target;
            std::uint64_t left;
            bool stopAfter(const calcu::machine::InstructionEvent& e) override {
                return e.target == target || --left == 0;
            }
        } obs;
        obs.target = thread == 1 ? 0 : m_d->targetCount() - 1;
        obs.left = maxSteps;
        m_d->passOnce(m_d->current());
        calcu::machine::Checks c = checks();
        c.observer = &obs;
        m_d->setChecks(c);
        m_d->run(1'000'000'000);
        arm();
        const auto stop = m_d->takeStop();
        if (stop && stop->kind == calcu::machine::DebugStop::Observer) return {};
        return stopOf(stop);
    }

private:
    calcu::machine::Checks checks() const {
        calcu::machine::Checks c;
        c.breakpoints = m_bps[0] || m_bps[1];
        c.watches = m_watches[0] || m_watches[1];
        return c;
    }
    void arm() { m_d->setChecks(checks()); }
    Stop stopOf(const std::optional<calcu::machine::DebugStop>& s) const {
        Stop out;
        if (!s) return out;
        out.thread = static_cast<int>(s->target) + 1;
        if (s->kind == calcu::machine::DebugStop::Breakpoint) out.kind = Stop::Breakpoint;
        else if (s->kind == calcu::machine::DebugStop::Watch) {
            out.kind = Stop::Watch;
            if (!s->hits.empty()) {
                out.hit.addr = static_cast<std::uint16_t>(s->hits[0].address);
                out.hit.write = s->hits[0].write;
                out.hit.space = static_cast<std::uint8_t>(s->hits[0].space);
            }
        }
        return out;
    }

    std::unique_ptr<sharp::SharpDebugging> m_d;
    bool m_bps[2] = {false, false};
    bool m_watches[2] = {false, false};
};

} // namespace debugtest
