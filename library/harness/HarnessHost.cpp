#include "harness/HarnessHost.hpp"
#include "harness/Product.hpp"
#include "harness/Run.hpp"
#include "sessions/MachineSession.hpp"
#include <cmath>
#include <cstdio>

namespace calcu::harness {

using sessions::Message;
using sessions::ReplyMessage;

Value fromSession(const sessions::Value& v) {
    if (v.isNull()) return Value();
    if (const bool* b = v.boolean()) return Value(*b);
    if (const std::int64_t* i = v.integer()) return Value(static_cast<long long>(*i));
    if (const double* d = v.real()) {
        if (!std::isfinite(*d)) return Value();
        return Value(Decimal{static_cast<std::int64_t>(std::llround(*d * 1e6)), 6});
    }
    if (const std::string* s = v.string()) return Value(*s);
    if (const auto* bytes = v.bytes()) {
        std::string hex = "0x";
        char buf[3];
        for (std::uint8_t b : *bytes) {
            std::snprintf(buf, sizeof buf, "%02x", b);
            hex += buf;
        }
        return Value(hex);
    }
    if (const auto* a = v.elements()) {
        Value out = Value::array();
        for (const auto& e : *a) out.push(fromSession(e));
        return out;
    }
    Value out = Value::object();
    if (const auto* o = v.members())
        for (const auto& m : *o) out.set(m.key, fromSession(m.value));
    return out;
}

HarnessHost::HarnessHost(sessions::AppSession& s, HarnessProduct& p, Attached,
                         std::function<void(const sessions::Message&)> tap)
    : onMessage(std::move(tap)), session_(s), product_(p) {
    listener_.onMessage = [this](const Message& m) {   // every message but a reply
        if (onMessage && !std::holds_alternative<ReplyMessage>(m)) onMessage(m);
    };
    listener_.listen(session_);   // no host services (C59-5 AC-12)
}

HarnessHost::HarnessHost(sessions::AppSession& s, HarnessProduct& p, const RunSetup& r)
    : session_(s), product_(p), setup_(&r) {
    listener_.listen(session_);
    // The replay of a listener that starts late marks every stream with a
    // reset; for the harness nothing has started over before its first read.
    for (const auto& t : topics())
        if (t.stream) listener_.mirror().takeReset(t.name);
}

HarnessHost::~HarnessHost() { finish(); }

std::optional<ReplyMessage> HarnessHost::takeReplyIfAny(sessions::CallId id) {
    for (auto it = replies_.begin(); it != replies_.end(); ++it)
        if (it->call == id) {
            ReplyMessage r = std::move(*it);
            replies_.erase(it);
            return r;
        }
    return std::nullopt;
}

// Run in VirtualTime, every command replies at once: an operation runs to its
// end inside send() (AC-7). Attached, an operation's reply is kept for
// takeReplyIfAny().
ReplyMessage HarnessHost::command(sessions::Command c) {
    sessions::Listener::ReplyHandler later;
    if (attached()) later = [this](const ReplyMessage& r) { replies_.push_back(r); };
    const auto sent = listener_.send(c.name, std::move(c.args), std::move(later));
    lastCall_ = sent.call;
    if (sent.reply) return *sent.reply;
    if (!attached()) return ReplyMessage{sent.call, false, "The session gave no reply.", nullptr};
    return ReplyMessage{sent.call, false, kPendingReply, nullptr};
}

ReplyMessage HarnessHost::ask(sessions::Request q) { return listener_.ask(q.name, std::move(q.args)); }

machine::Ticks HarnessHost::advance(machine::Ticks n) { return session_.advanceUnpaced(n); }

std::vector<sessions::TopicEntry> HarnessHost::topics() {
    const ReplyMessage r = ask(sessions::Request{sessions::kTopicsRequest, {}});
    if (const auto* l = sessions::payloadAs<sessions::TopicList>(r.result)) return l->topics;
    return {};
}

std::optional<TopicRead> HarnessHost::readTopic(std::string_view name) {
    for (const auto& t : topics()) {
        if (t.name.value != name) continue;
        TopicRead out;
        out.topic = t.name.value;
        out.stream = t.stream;
        if (t.stream) {
            out.reset = listener_.mirror().takeReset(t.name);
            out.items = listener_.mirror().takeAppended(t.name);
        } else if (const sessions::PayloadPtr* p = listener_.mirror().state(t.name)) {
            out.state = *p;
        }
        return out;
    }
    return std::nullopt;
}

const machine::DisplayOutput* HarnessHost::display() const { return listener_.display(); }

const machine::MachineDescriptor* HarnessHost::descriptor() {
    const machine::Machine* m = session_.machineSession().machine();
    return m ? &m->descriptor() : nullptr;
}

DisplayText HarnessHost::displayText() {
    const machine::DisplayOutput* o = display();
    const machine::MachineDescriptor* d = descriptor();
    if (!o || !d) {
        DisplayText off;
        off.on = false;
        return off;
    }
    DisplayText t = product_.readDisplay(*o, *d);
    t.on = o->on;
    return t;
}

void HarnessHost::SoundSink::deliver(const Message& m) {
    if (const auto* s = std::get_if<sessions::SoundSamples>(&m)) wav.write(s->mono);
}

Reply HarnessHost::record(const std::string& path) {
    // What the machine made before the command is not part of the file.
    if (!attached()) session_.advanceUnpaced(0);
    if (soundListener_) {
        session_.stopListening(*soundListener_);
        soundListener_.reset();
    }
    if (sound_) sound_->wav.close();
    auto sink = std::make_unique<SoundSink>();
    std::string error;
    const int rate = session_.machineSession().soundRate();
    if (!sink->wav.open(path, rate, &error)) return refusedReply(error);
    sound_ = std::move(sink);
    sessions::HostServices services;
    services.recordsSound = true;
    soundListener_ = session_.listen(*sound_, services);
    return doneReply();
}

void HarnessHost::finish() {
    if (finished_) return;
    finished_ = true;
    if (soundListener_) {
        if (!attached()) session_.advanceUnpaced(0);   // the sound of the last operation
        session_.stopListening(*soundListener_);
        soundListener_.reset();
    }
    if (sound_) sound_->wav.close();
    listener_.stop();
}

} // namespace calcu::harness
