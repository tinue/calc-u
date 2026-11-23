#include "harness/attach/AttachedHarness.hpp"
#include "formats/Duration.hpp"
#include "harness/Protocol.hpp"
#include "machine/Machine.hpp"
#include "presets/PresetsPart.hpp"
#include "sessions/LibraryTopics.hpp"

namespace calcu::harness {

using namespace sessions;

namespace {

std::string escaped(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else out += c;
    }
    return out;
}

} // namespace

AttachedHarness::AttachedHarness(AppSession& s, HarnessProduct& p, AttachEndpoint& e, std::function<bool()> suspended,
                                 parts::LibraryParts& parts)
    : session_(s), product_(p), endpoint_(e), suspended_(std::move(suspended)) {
    // The commands of `calcu serve`, in its order.
    addServeCommands(registry_, parts, product_);
}

AttachedHarness::~AttachedHarness() { detach(); }

void AttachedHarness::handle(const AttachEvent& e) {
    switch (e.kind) {
    case AttachEvent::Kind::Attached:
        detach();
        client_ = e.client;
        json_ = false;
        // A second listener, sent every state topic once at once.
        host_ = std::make_unique<HarnessHost>(session_, product_, HarnessHost::Attached{},
                                              [this](const Message& m) { message(m); });
        break;
    case AttachEvent::Kind::Line:
        if (e.client != client_ || !host_) return;
        queue_.push_back(e.line);
        process();
        break;
    case AttachEvent::Kind::Detached:
        if (e.client == client_) detach();   // as if never attached
        break;
    }
}

// The listener goes, its pending replies with it, keys it holds come up.
void AttachedHarness::detach() {
    if (!host_) return;
    queue_.clear();
    pending_.reset();
    const auto held = std::move(held_);
    held_.clear();
    for (const auto& k : held) {   // the keys it holds come up
        auto parsed = parseLine("key-up " + k, registry_);
        if (auto* cl = std::get_if<CommandLine>(&parsed)) {
            CommandContext ctx{*host_, *cl};
            (*registry_.find(cl->path))(ctx);
        }
    }
    host_->finish();
    host_.reset();
}

void AttachedHarness::lines(const std::string& text) {
    std::vector<std::string> out;
    std::size_t at = 0;
    while (at < text.size()) {
        std::size_t nl = text.find('\n', at);
        if (nl == std::string::npos) nl = text.size();
        out.push_back(text.substr(at, nl - at));
        at = nl + 1;
    }
    if (!out.empty()) endpoint_.write(client_, std::move(out));
}

void AttachedHarness::reply(const Reply& r) { lines(json_ ? writeJson(r) : writePlain(r)); }

void AttachedHarness::message(const Message& m) {
    auto text = [](const PayloadPtr& p) { return payloadText(p); };
    auto one = [&](const std::string& topic, const std::string& t, harness::Value v) {
        if (json_) {
            harness::Value inner = harness::Value::object();
            inner.set("topic", topic);
            inner.set("value", std::move(v));
            harness::Value outer = harness::Value::object();
            outer.set("message", std::move(inner));
            lines(writeJson(outer));
        } else {
            lines("message " + topic + " " + escaped(t));
        }
    };
    auto value = [](const PayloadPtr& p) { return p ? fromSession(p->toValue()) : harness::Value(); };
    if (const auto* s = std::get_if<StateMessage>(&m)) {
        one(s->topic.value, text(s->state), value(s->state));
    } else if (const auto* st = std::get_if<StreamMessage>(&m)) {
        if (st->reset) one(st->topic.value, "reset", harness::Value("reset"));
        for (const auto& p : st->appended) one(st->topic.value, text(p), value(p));
    }
}

std::optional<machine::Ticks> AttachedHarness::ticks() {
    if (!host_) return std::nullopt;
    const ReplyMessage r = host_->ask(Request{kTicksRequest, {}});
    const auto* t = payloadAs<EmulatedTimeReply>(r.result);
    if (!r.done || !t) return std::nullopt;
    return t->ticks;
}

void AttachedHarness::process() {
    while (host_ && !pending_ && !queue_.empty()) {
        std::string l = std::move(queue_.front());
        queue_.pop_front();
        runLine(l);
    }
}

void AttachedHarness::runLine(const std::string& text) {
    // `json` is the attach transport's, not a harness command: the registry
    // does not know it, so it is taken before the line is parsed.
    const std::string line = trimmed(text);
    const std::string word = line.substr(0, line.find_first_of(" \t"));
    if (word == "json") {
        if (suspended_ && suspended_()) return reply(refusedReply(kSuspended));
        const std::string arg = trimmed(std::string_view(line).substr(word.size()));
        if (arg != "on" && arg != "off") return reply(refusedReply(kJsonUsage));
        json_ = arg == "on";
        return reply(doneReply());
    }
    auto parsed = parseLine(text, registry_);
    if (auto* refusal = std::get_if<std::string>(&parsed)) {
        if (suspended_ && suspended_()) return reply(refusedReply(kSuspended));
        return reply(refusedReply(*refusal));
    }
    const CommandLine& cl = std::get<CommandLine>(parsed);
    if (cl.path.empty()) return;
    if (suspended_ && suspended_()) return reply(refusedReply(kSuspended));
    if (cl.path.size() == 1 && cl.path[0] == "wait") {   // on the app's clock
        if (cl.argText.empty()) {   // until idle, the session's operation on the app's clock
            const machine::MachineDescriptor* d = host_->descriptor();
            const auto now = ticks();
            if (!d || !now) return reply(refusedReply(kNoMachine));
            const ReplyMessage r = host_->command(Command{presets::kWaitIdleCommand, {}});
            if (r.done) return reply(timeReply(0, d->tickRate));
            if (r.reason != HarnessHost::kPendingReply) return reply(refusedReply(r.reason));
            Pending p;
            p.idle = true;
            p.start = *now;
            p.rate = d->tickRate;
            p.call = host_->lastCall();
            pending_ = p;
            return;
        }
        std::string error;
        const auto us = formats::parseDuration(cl.argText, &error);
        if (!us) return reply(refusedReply(error));
        const machine::MachineDescriptor* d = host_->descriptor();
        const auto now = ticks();
        if (!d || !now) return reply(refusedReply(kNoMachine));
        Pending p;
        p.wait = true;
        p.start = *now;
        p.rate = d->tickRate;
        p.target = *now + ticksFor(*us, d->tickRate);
        pending_ = p;
        return;
    }
    if (cl.path.size() == 1 && cl.path[0] == "key-down") held_.insert(trimmed(cl.argText));
    if (cl.path.size() == 1 && cl.path[0] == "key-up") held_.erase(trimmed(cl.argText));
    const Handler* h = registry_.find(cl.path);
    CommandContext ctx{*host_, cl};
    Reply r = (*h)(ctx);
    if (!r.done && r.reason == HarnessHost::kPendingReply) {   // an operation: its reply when it ends
        Pending p;
        p.call = host_->lastCall();
        pending_ = p;
        return;
    }
    reply(r);
}

void AttachedHarness::afterHostCall() {
    if (!host_ || !pending_) return;
    if (pending_->wait) {
        const auto now = ticks();
        if (!now || *now < pending_->target) return;
        const Pending p = *pending_;
        pending_.reset();
        reply(timeReply(*now - p.start, p.rate));
    } else {
        auto r = host_->takeReplyIfAny(pending_->call);
        if (!r) return;
        const Pending p = *pending_;
        pending_.reset();
        if (p.idle && r->done) {
            const auto now = ticks();
            Reply out = timeReply(now && *now > p.start ? *now - p.start : 0, p.rate);
            const bool hour = now && *now - p.start >= static_cast<machine::Ticks>(p.rate) * 3600;
            out.result.set("hourReached", hour);
            out.lines.push_back(std::string("hour reached: ") + (hour ? "yes" : "no"));
            reply(out);
        } else {
            reply(r->done ? doneReply() : refusedReply(r->reason));
        }
    }
    process();
}

} // namespace calcu::harness
