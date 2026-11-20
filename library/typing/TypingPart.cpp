#include "typing/TypingPart.hpp"
#include "sessions/Args.hpp"
#include "sessions/LibraryTopics.hpp"
#include "sessions/MachineSession.hpp"
#include "typing/KeyNames.hpp"
#include "typing/TypingTopics.hpp"
#include <memory>
#include <string>

namespace calcu::typing {

using sessions::CommandOutcome;
using sessions::Done;
using sessions::Refused;
using sessions::SessionContext;
using sessions::Value;

namespace {

bool hasKey(const machine::Machine& m, const machine::KeyId& k) {
    for (const auto& d : m.descriptor().keys)
        if (d.id == k) return true;
    return false;
}

// The names of a `key` command: an array of strings, or one string of names
// separated by spaces.
std::vector<std::string> namesOf(const Value& args) {
    std::vector<std::string> out;
    const Value* v = args.find("names");
    if (!v) return out;
    if (const auto* a = v->elements()) {
        for (const auto& e : *a)
            if (e.string()) out.push_back(*e.string());
    } else if (const std::string* s = v->string()) {
        std::string word;
        for (char c : *s + " ") {
            if (c == ' ' || c == '\t') {
                if (!word.empty()) out.push_back(word);
                word.clear();
            } else {
                word += c;
            }
        }
    }
    return out;
}

} // namespace

TypingPart::TypingPart(const TypingProduct& p, sessions::Hosting h) : product_(p), hosting_(h) {}

void TypingPart::attach(sessions::SessionSetup& s) {
    ctx_ = &s.context();
    sessions::MachineSession& ms = ctx_->machineSession();
    engine_.bind(ms, [this] { machineChanged(*ctx_); });
    engine_.setVirtualTime(hosting_ == sessions::Hosting::VirtualTime);
    engine_.setOnHeldChanged([this] { heldChanged(); });
    engine_.setOnFinished([this](Token t, const TypeResult& r) { finished(t, r); });
    ms.addHook(engine_);
    held_ = &s.stateTopic(kHeldKeysTopic);
    held_->set(std::make_shared<HeldKeys>());

    auto live = [this](auto f) {
        return [this, f](const Value& a, SessionContext& c) -> CommandOutcome {
            sync();
            if (c.busy()) return Done{};   // dropped while an operation runs
            return f(a);
        };
    };
    auto always = [this](auto f) {
        return [this, f](const Value& a, SessionContext&) -> CommandOutcome {
            sync();
            return f(a);
        };
    };
    s.command(kHostKeyDown, live([this](const Value& a) -> CommandOutcome {
                  engine_.hostKeyDown(hostKeyEventOf(a));
                  return Done{};
              }));
    s.command(kHostKeyUp, always([this](const Value& a) -> CommandOutcome {
                  engine_.hostKeyUp(sessions::textArg(a, "physical"));
                  return Done{};
              }));
    s.command(kHostText, live([this](const Value& a) -> CommandOutcome {
                  engine_.hostText(decodeUtf8(sessions::textArg(a, "text")));
                  return Done{};
              }));
    s.command(kScreenKeyDown, [this](const Value& a, SessionContext& c) -> CommandOutcome {
        sync();
        const machine::Machine* m = engine_.machine();
        if (!m) return Refused{sessions::kNoMachine};
        const machine::KeyId key{sessions::textArg(a, "key")};
        if (!hasKey(*m, key)) return Refused{"Unknown key: " + key.value + "."};
        if (c.busy()) return Done{};
        engine_.screenKeyDown(key, static_cast<std::uint32_t>(sessions::truncatedIntegerArg(a, "pointer")));
        return Done{};
    });
    s.command(kScreenKeyUp, always([this](const Value& a) -> CommandOutcome {
                  engine_.screenKeyUp(machine::KeyId{sessions::textArg(a, "key")},
                                      static_cast<std::uint32_t>(sessions::truncatedIntegerArg(a, "pointer")));
                  return Done{};
              }));
    s.command(kFocusLost, always([this](const Value&) -> CommandOutcome {
                  engine_.focusLost();
                  return Done{};
              }));
    s.command(kPaste, live([this](const Value& a) -> CommandOutcome {
                  engine_.paste(sessions::textArg(a, "text"));
                  return Done{};
              }));
    s.command(kKey, always([this](const Value& a) -> CommandOutcome {
                  if (!engine_.machine()) return Refused{sessions::kNoMachine};
                  const std::vector<std::string> names = namesOf(a);
                  if (names.empty()) return Refused{"The key command needs one or more key IDs."};
                  // an unknown name refuses the command, nothing is typed
                  auto presses = pressesOfNames(engine_.profile(), engine_.machine()->descriptor(), names);
                  if (const auto* no = std::get_if<KeyNamesRefused>(&presses)) return Refused{"Unknown key: " + no->name + "."};
                  // hold: seconds of emulated time, every press held exactly this long
                  std::optional<machine::Ticks> hold;
                  if (const Value* h = a.find("hold"); h && h->real())
                      hold = std::max<machine::Ticks>(1, ticksOf(*h->real(), engine_.machine()->descriptor().tickRate));
                  return deferred(engine_.typeKeys(std::get<std::vector<Press>>(std::move(presses)), Origin::Typed, false, hold));
              }));
    if (product_.typesText())
        s.command(kType, always([this](const Value& a) -> CommandOutcome {
                      const TypingProfile* p = engine_.profile();
                      if (!engine_.machine()) return Refused{sessions::kNoMachine};
                      if (!p || !p->charKeys || !p->enter) return Refused{"Unknown command: type."};   // as on a model without `type`
                      return deferred(engine_.typeText(sessions::textArg(a, "text"), true, true, Origin::Typed));
                  }));
    s.request(kTypingRequest, [this](const Value&, SessionContext&) {
        sync();
        auto st = std::make_shared<TypingState>();
        st->idle = engine_.idle();
        st->waiting = engine_.waiting();
        return sessions::replyGiven(std::move(st));
    });
}

CommandOutcome TypingPart::deferred(const Queued& q) {
    if (q.refused) return Refused{*q.refused};
    if (auto r = engine_.takeResult(q.token)) {   // over at once
        if (r->done) return Done{};
        return Refused{r->reason};
    }
    calls_[q.token] = ctx_->currentCall();
    return sessions::Deferred{};
}

void TypingPart::finished(Token t, const TypeResult& r) {
    const auto it = calls_.find(t);
    if (it == calls_.end()) return;   // not a command's (typeAndWait's, or answered at once)
    const sessions::CallId call = it->second;
    calls_.erase(it);
    engine_.takeResult(t);
    ctx_->reply(call, sessions::ReplyMessage{call, r.done, r.done ? std::string() : r.reason, nullptr});
}

void TypingPart::heldChanged() {
    if (!held_) return;
    auto h = std::make_shared<HeldKeys>();
    h->live = engine_.liveHeld();
    held_->set(std::move(h));
}

void TypingPart::sync() {
    if (!ctx_) return;
    engine_.setPaused(ctx_->machineSession().paused(), wall_);
}

void TypingPart::beforeCall(SessionContext& c, pacing::WallTime now) {
    ctx_ = &c;
    wall_ = now;
    sync();
    engine_.hostCall(now);
}

void TypingPart::afterCall(SessionContext& c, pacing::WallTime now) {
    ctx_ = &c;
    wall_ = now;
    sync();
    engine_.hostCall(now);
}

void TypingPart::machineChanged(SessionContext& c) {
    ctx_ = &c;
    machine::Machine* m = c.machineSession().machine();
    const std::uint64_t generation = c.machineSession().generation();
    if (engineGeneration_ == generation) return;
    engineGeneration_ = generation;
    engine_.setMachine(m, m ? product_.profile(m->model()) : nullptr, m ? product_.keyboardMap(m->model()) : nullptr);
    heldChanged();
}

} // namespace calcu::typing
