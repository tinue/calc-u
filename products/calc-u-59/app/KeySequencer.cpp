#include "KeySequencer.hpp"
#include <algorithm>
#include <utility>

namespace cu59 {

// ── Lane ─────────────────────────────────────────────────────────────────────

KeySequencer::Lane::Lane(KeySequencer& owner) : m_owner(owner) {}

KeySequencer::Lane::~Lane() { ++m_generation; }

void KeySequencer::Lane::post(Task task) {
    task.generation = m_generation;
    m_tasks.push_back(std::move(task));
}

void KeySequencer::Lane::cancel() {
    ++m_generation;
    m_tasks.clear();
}

// A task cancelled before it started never runs (no playbackStarted).
bool KeySequencer::Lane::takeNext(Task& task) {
    while (!m_tasks.empty()) {
        task = std::move(m_tasks.front());
        m_tasks.pop_front();
        if (!cancelled(task.generation)) return true;
    }
    return false;
}

std::optional<KeySequencer::Clock::time_point> KeySequencer::Lane::advance(Clock::time_point now) {
    for (;;) {
        if (!m_current) {
            Task task;
            if (!takeNext(task)) return std::nullopt;
            m_current = std::move(task);
        }
        auto wake = m_owner.step(*this, *m_current, now);
        if (!wake) {
            m_current.reset();
            continue;
        }
        if (*wake > now) return wake;
    }
}

// ── KeySequencer ─────────────────────────────────────────────────────────────

KeySequencer::KeySequencer(Host& host) : m_host(host), m_script(*this), m_keys(*this) {}

KeySequencer::~KeySequencer() {
    m_script.cancel();
    m_keys.cancel();
}

void KeySequencer::advance(Clock::time_point now) {
    m_now = std::max(m_now, now);
    releaseDue(m_now);
    m_script.advance(m_now);
    m_keys.advance(m_now);
}

// A tap waits for the narrow steady-state scan loop, holds the key kHold —
// not cut short by a cancel, the key must still come up — then waits for the
// scan loop again. The narrow loop, not the wider key-detect range: that also
// covers the release gate, where a new key is discarded, and the key just
// pressed being decoded. (With a printer the narrow loop includes the confirm
// scan: the printer line makes every pass take it.) A freeze stops the CPU
// outright: a keyboard tap goes in at once, a script waits until it runs
// again; the wait after a key ends at once instead of forever.
std::optional<KeySequencer::Clock::time_point> KeySequencer::step(Lane& lane, Task& task, Clock::time_point now) {
    using K = KeystrokeEvent::Kind;
    using P = Task::Phase;
    auto seconds = [](double s) { return std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(s)); };
    // Not in the scan loop yet: look again kScanPoll after the last look.
    auto poll = [&] {
        if (now >= task.wake) task.wake = now + kScanPoll;
        return task.wake;
    };
    if (task.script && task.next == 0 && task.phase == P::Next) {
        m_playing = true;
        m_host.playbackStarted();
    }
    for (;;) {
        switch (task.phase) {
        case P::WaitingToPress:
            if (lane.cancelled(task.generation)) {
                task.phase = P::Next;
                break;
            }
            // A script waits out a freeze; a keyboard tap goes in, as a finger would.
            if (task.script ? !m_host.isFrozen() && m_host.isKeyReady()
                            : m_host.isFrozen() || m_host.isKeyReady()) {
                if (!task.script) m_host.heldKeyChanged(highlightID(task.code));
                m_host.pressKey(task.code);
                task.wake = now + kHold;
                task.phase = P::Holding;
                return task.wake;
            }
            return poll();
        case P::Holding:
            if (now < task.wake) return task.wake;
            m_host.releaseKey(task.code);
            task.phase = P::WaitingIdle;
            break;
        case P::WaitingIdle:
            if (lane.cancelled(task.generation) || m_host.isFrozen() || m_host.isKeyReady()) {
                task.phase = P::Next;
                break;
            }
            return poll();
        case P::Sleeping:
            if (now < task.wake && !lane.cancelled(task.generation)) return task.wake;
            task.phase = P::Next;
            break;
        case P::WaitingToApply: {
            if (lane.cancelled(task.generation)) {
                task.phase = P::Next;
                break;
            }
            if (m_host.isFrozen() || !m_host.isKeyReady()) return poll();
            const KeystrokeEvent& e = task.events[task.next - 1];
            if (e.kind == K::Reload) m_host.applyReload(e.reload);
            else m_host.queueCard(e);
            task.phase = P::WaitingHost;
            break;
        }
        case P::WaitingHost:
            if (lane.cancelled(task.generation) || !m_host.hostPending()) {
                task.phase = P::Next;
                break;
            }
            return poll();
        case P::Next: {
            // A keyboard sequence that ran its last tap clears the highlight
            // even if cancelled during that tap; one cancelled earlier doesn't.
            if (task.next == task.events.size() || (task.script && lane.cancelled(task.generation))) {
                finish(task);
                return std::nullopt;
            }
            if (lane.cancelled(task.generation)) return std::nullopt;
            const KeystrokeEvent& e = task.events[task.next++];
            switch (e.kind) {
            case K::Key:
                task.code = e.code;
                task.wake = now;
                task.phase = P::WaitingToPress;
                break;
            case K::Wait:
                task.wake = now + seconds(e.seconds);
                task.phase = P::Sleeping;
                break;
            case K::FullSpeed: m_host.setFullSpeed(true); break;
            case K::RegularSpeed: m_host.setFullSpeed(false); break;
            case K::ZeroElapseTime: m_host.zeroElapseTime(); break;
            case K::ReportElapseTime: m_host.reportElapseTime(); break;
            case K::Trace: m_host.scriptedTrace(e.traceName); break;
            case K::Reload:
            case K::InsertCard:
                task.wake = now;
                task.phase = P::WaitingToApply;
                break;
            case K::ArmCard: m_host.queueCard(e); break;
            case K::PrinterTrace:
            case K::CutPaper:
            case K::ShowPanel:
            case K::Freeze:
            case K::FreezeOnStart:
            case K::Step:
            case K::Resume:
                m_host.control(e);
                task.wake = now;
                task.phase = P::WaitingHost;
                break;
            }
            break;
        }
        }
    }
}

void KeySequencer::finish(Task& task) {
    if (task.script) {
        m_playing = false;
        m_host.playbackFinished();
    } else {
        m_host.heldKeyChanged(-1);
    }
}

void KeySequencer::play(std::vector<KeystrokeEvent> events) {
    Task task;
    task.events = std::move(events);
    task.script = true;
    m_script.post(std::move(task));
}

void KeySequencer::cancel() { m_script.cancel(); }

void KeySequencer::keyDown(const std::vector<uint8_t>& codes) {
    if (codes.empty()) return;
    // The hardware rejects chords: one key at a time.
    keyUp();
    m_keys.cancel();
    if (codes.size() == 1) {
        m_heldCode = codes[0];
        m_heldSince = now();
        m_host.heldKeyChanged(highlightID(codes[0]));
        m_host.pressKey(codes[0]);
        return;
    }
    Task task;
    for (uint8_t code : codes) {
        KeystrokeEvent e;
        e.code = code;
        task.events.push_back(e);
    }
    m_keys.post(std::move(task));
}

void KeySequencer::keyUp() {
    if (!m_heldCode) return;
    const uint8_t code = *m_heldCode;
    const Clock::time_point since = m_heldSince;
    m_heldCode.reset();
    m_host.heldKeyChanged(-1);
    auto earliest = since + kHold;
    if (now() >= earliest) m_host.releaseKey(code);
    else releaseAt(code, earliest);
}

void KeySequencer::releaseAll() {
    m_keys.cancel();
    const std::optional<uint8_t> code = std::exchange(m_heldCode, std::nullopt);
    if (code) m_host.releaseKey(*code);
    m_host.heldKeyChanged(-1);
}

void KeySequencer::releaseAt(uint8_t code, Clock::time_point when) { m_releases.emplace_back(when, code); }

void KeySequencer::releaseDue(Clock::time_point now) {
    for (;;) {
        auto it = std::min_element(m_releases.begin(), m_releases.end());
        if (it == m_releases.end() || now < it->first) return;
        uint8_t code = it->second;
        m_releases.erase(it);
        m_host.releaseKey(code);
    }
}

} // namespace cu59
