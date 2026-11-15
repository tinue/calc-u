#include "TIKeystrokePlayback.hpp"
#include <algorithm>

namespace cu59 {

void TIKeystrokePlayback::play(std::vector<KeystrokeEvent> events) {
    Script s;
    s.events = std::move(events);
    s.generation = m_generation;
    m_scripts.push_back(std::move(s));
}

void TIKeystrokePlayback::cancel() {
    ++m_generation;
    m_scripts.clear();   // a queued script never starts (no playbackStarted)
}

void TIKeystrokePlayback::advance(Clock::time_point now) {
    m_now = std::max(m_now, now);
    for (int guard = 0; guard < 10'000; ++guard) {
        if (!m_current) {
            while (!m_scripts.empty() && cancelled(m_scripts.front())) m_scripts.pop_front();
            if (m_scripts.empty()) return;
            m_current = std::move(m_scripts.front());
            m_scripts.pop_front();
        }
        if (step(*m_current, m_now)) return;   // waits
        m_current.reset();
    }
}

bool TIKeystrokePlayback::step(Script& s, Clock::time_point now) {
    using K = KeystrokeEvent::Kind;
    using P = Script::Phase;
    auto seconds = [](double x) { return std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(x)); };
    if (!s.started) {
        s.started = true;
        m_playing = true;
        m_host.playbackStarted();
    }
    auto finish = [&] {
        m_playing = false;
        m_host.playbackFinished();
        return false;
    };
    for (;;) {
        switch (s.phase) {
        case P::Typing:   // the engine holds the key, then waits for the scan loop again
            if (!cancelled(s) && !m_host.typingIdle()) return true;
            s.phase = P::Next;
            break;
        case P::Sleeping:
            if (now < s.wake && !cancelled(s)) return true;
            s.phase = P::Next;
            break;
        case P::WaitingToApply: {
            if (cancelled(s)) {
                s.phase = P::Next;
                break;
            }
            if (m_host.isFrozen() || !m_host.isKeyReady()) return true;
            const KeystrokeEvent& e = s.events[s.next - 1];
            if (e.kind == K::Reload) m_host.applyReload(e.reload);
            else m_host.queueCard(e);
            s.phase = P::WaitingHost;
            break;
        }
        case P::WaitingHost:
            if (!cancelled(s) && m_host.hostPending()) return true;
            s.phase = P::Next;
            break;
        case P::Next: {
            if (s.next == s.events.size() || cancelled(s)) return finish();
            const KeystrokeEvent& e = s.events[s.next++];
            switch (e.kind) {
            case K::Key:
                if (m_host.typeKey(e.code)) s.phase = P::Typing;
                break;
            case K::Wait:
                s.wake = now + seconds(e.seconds);
                s.phase = P::Sleeping;
                break;
            case K::FullSpeed: m_host.setFullSpeed(true); break;
            case K::RegularSpeed: m_host.setFullSpeed(false); break;
            case K::ZeroElapseTime: m_host.zeroElapseTime(); break;
            case K::ReportElapseTime: m_host.reportElapseTime(); break;
            case K::Trace: m_host.scriptedTrace(e.traceName); break;
            case K::Reload:
            case K::InsertCard: s.phase = P::WaitingToApply; break;
            case K::ArmCard: m_host.queueCard(e); break;
            case K::PrinterTrace:
            case K::CutPaper:
            case K::ShowPanel:
            case K::Freeze:
            case K::FreezeOnStart:
            case K::Step:
            case K::Resume:
                m_host.control(e);
                s.phase = P::WaitingHost;
                break;
            }
            break;
        }
        }
    }
}

} // namespace cu59
