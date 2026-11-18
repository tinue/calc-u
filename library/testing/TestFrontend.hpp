#pragma once
#include "sessions/LibraryTopics.hpp"
#include "sessions/Messages.hpp"
#include "sessions/TopicMirror.hpp"
#include <cstddef>
#include <string>
#include <vector>

// The test frontend (in library/testing/ for the library's tests and both
// products'): a TopicMirror, the raw message log and two delivery modes, at once (the caller's thread) and through a
// queue (another thread, a Web Worker), pumped by the test.

namespace calcu::sessions::test {

inline std::string describe(const Message& m) {
    struct V {
        std::string operator()(const StateMessage& s) const {
            return "state " + s.topic.value + " " + std::string(s.state->kind().value) + " " + toText(s.state->toValue());
        }
        std::string operator()(const StreamMessage& s) const {
            std::string out = "stream " + s.topic.value + (s.reset ? " reset" : "");
            for (const auto& p : s.appended) out += " " + toText(p->toValue());
            return out;
        }
        std::string operator()(const ReplyMessage& r) const {
            return "reply " + std::to_string(r.call) + (r.done ? " done" : " refused ") + r.reason +
                   (r.result ? " " + toText(r.result->toValue()) : "");
        }
        std::string operator()(const ServiceAsk& a) const {
            return "ask " + std::to_string(a.ask) + " " + std::to_string(static_cast<int>(a.service)) + " " + toText(a.args);
        }
        std::string operator()(const SoundStart& s) const { return "sound-start " + std::to_string(s.sampleRate); }
        std::string operator()(const SoundSamples& s) const {
            long long sum = 0;
            for (auto x : s.mono) sum += x;
            return "sound " + std::to_string(s.mono.size()) + " " + std::to_string(sum);
        }
        std::string operator()(const SoundStop&) const { return "sound-stop"; }
    };
    return std::visit(V{}, m);
}

// The notices a mirror got since the last take, as their sentences.
inline std::vector<std::string> noticeSentences(TopicMirror& mirror) {
    std::vector<std::string> out;
    for (const auto& p : mirror.takeAppended(kNoticesTopic))
        if (const auto* n = payloadAs<Notice>(p)) out.push_back(n->sentence);
    return out;
}

class TestFrontend : public MessageSink {
public:
    enum class Delivery { AtOnce, Queued };
    explicit TestFrontend(Delivery d = Delivery::AtOnce) : delivery(d) {}

    void deliver(const Message& m) override {
        if (delivery == Delivery::Queued) queue.push_back(m);
        else take(m);
    }
    void pump() {
        std::vector<Message> q;
        q.swap(queue);
        for (const auto& m : q) take(m);
    }

    // What arrived, from position `mark` of the log on.
    std::vector<Message> since(std::size_t mark) const {
        return std::vector<Message>(log.begin() + static_cast<std::ptrdiff_t>(mark), log.end());
    }
    std::vector<std::string> described() const {
        std::vector<std::string> out;
        for (const auto& m : log) out.push_back(describe(m));
        return out;
    }
    int replies(CallId id) const {
        int n = 0;
        for (const auto& m : log)
            if (const auto* r = std::get_if<ReplyMessage>(&m); r && r->call == id) ++n;
        return n;
    }
    const ReplyMessage* reply(CallId id) const {
        for (const auto& m : log)
            if (const auto* r = std::get_if<ReplyMessage>(&m); r && r->call == id) return r;
        return nullptr;
    }
    int stateMessages(const TopicName& t, std::size_t from = 0) const {
        int n = 0;
        for (std::size_t i = from; i < log.size(); ++i)
            if (const auto* s = std::get_if<StateMessage>(&log[i]); s && s->topic == t) ++n;
        return n;
    }
    template <class M> int count(std::size_t from = 0) const {
        int n = 0;
        for (std::size_t i = from; i < log.size(); ++i)
            if (std::holds_alternative<M>(log[i])) ++n;
        return n;
    }
    template <class P> const P* state(const TopicName& t) const {
        const PayloadPtr* p = mirror.state(t);
        return p ? payloadAs<P>(*p) : nullptr;
    }
    const BusyState* busy() const { return state<BusyState>(kBusyTopic); }
    std::vector<std::string> noticeSentences() { return test::noticeSentences(mirror); }

    Delivery delivery;
    TopicMirror mirror;
    std::vector<Message> log;     // everything taken, in order
    std::vector<Message> queue;   // Queued: not pumped yet

private:
    void take(const Message& m) {
        log.push_back(m);
        mirror.deliver(m);
    }
};

} // namespace calcu::sessions::test
