// The first commands: wait, key-down, key-up, display-text, display, state,
// capabilities, ticks, audio, state-save, state-restore, quit.
#include "files/Files.hpp"
#include "formats/Duration.hpp"
#include "harness/Commands.hpp"
#include "harness/HarnessHost.hpp"
#include "harness/StatePart.hpp"
#include "machine/Attach.hpp"
#include "machine/Machine.hpp"
#include "presets/PresetsPart.hpp"
#include "sessions/LibraryTopics.hpp"
#include "typing/KeyNames.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <variant>

namespace calcu::harness {

using namespace sessions;

namespace {


Reply needs(const char* usage) { return refusedReply(std::string("usage: ") + usage); }

// The key as the `key` step resolves it: the key ID, then the ID in
// any case, then one of its other spellings in any case. An unknown key
// is passed as written, and the session refuses it.
std::string resolveKey(const machine::MachineDescriptor* d, const std::string& written) {
    const machine::KeyDescriptor* k = d ? typing::keyNamed(*d, written) : nullptr;
    return k ? k->id.value : written;
}

std::string ownerText(const machine::Owner& o) {
    if (const auto* c = std::get_if<machine::ComponentOwner>(&o)) return "component:" + c->component.value;
    const auto& p = std::get<machine::PeripheralOwner>(o);
    return "peripheral:" + p.peripheral.value + "@" + machine::placeText(p.place);
}

// `wait` without a duration waits until the machine has settled, and
// `wait-finished` until its program has ended too; at most an hour of
// emulated time (the counterpart of a preset's implicit wait).
Reply waitIdle(CommandContext& c, bool finished) {
    const machine::MachineDescriptor* d = c.host.descriptor();
    if (!d) return refusedReply(kNoMachine);
    const machine::Machine* m = c.host.session().machineSession().machine();
    const machine::Ticks t0 = m ? m->tickCount() : 0;
    const ReplyMessage r =
        c.host.command(Command{finished ? presets::kWaitFinishedCommand : presets::kWaitIdleCommand, {}});
    if (!r.done) return refusedReply(r.reason);
    m = c.host.session().machineSession().machine();
    const machine::Ticks ran = m ? m->tickCount() - t0 : 0;
    const bool hour = ran >= static_cast<machine::Ticks>(d->tickRate) * 3600;
    Reply reply = timeReply(ran, d->tickRate);
    reply.result.set("hourReached", hour);
    reply.lines.push_back(std::string("hour reached: ") + (hour ? "yes" : "no"));
    return reply;
}

Reply waitCmd(CommandContext& c) {
    if (c.line.argText.empty()) return waitIdle(c, false);
    std::string error;
    const auto us = formats::parseDuration(c.line.argText, &error);
    if (!us) return refusedReply(error);
    const machine::MachineDescriptor* d = c.host.descriptor();
    if (!d) return refusedReply(kNoMachine);
    const machine::Ticks ran = c.host.advance(ticksFor(*us, d->tickRate));
    Reply reply = timeReply(ran, d->tickRate);
    // A wait ends where the machine pauses, or at once on a
    // paused machine; then the reply gives the run state too.
    if (auto t = c.host.readTopic("run-state"); t && t->state) {
        const sessions::Value v = t->state->toValue();
        const sessions::Value* p = v.find("paused");
        if (p && p->boolean() && *p->boolean()) {
            reply.result.set("runState", fromSession(v));
            const sessions::Value* owner = v.find("owner");
            const sessions::Value* reason = v.find("reason");
            reply.lines.push_back("run state: paused, owner " + (owner && owner->string() ? *owner->string() : std::string()) +
                                  ", reason " + (reason && reason->string() ? *reason->string() : std::string()));
        }
    }
    return reply;
}

Handler keyCmd(bool down) {
    return [down](CommandContext& c) {
        if (c.line.argText.empty()) return needs(down ? "key-down <key ID>" : "key-up <key ID>");
        const std::string key = resolveKey(c.host.descriptor(), c.line.argText);
        sessions::Value a = sessions::Value::object({{"key", key}});
        return fromSessionReply(c.host.command(Command{down ? kKeyDownCommand : kKeyUpCommand, std::move(a)}));
    };
}

Reply displayTextCmd(CommandContext& c) {
    const DisplayText t = c.host.displayText();
    return doneReply(displayTextValue(t), displayTextLines(t));
}

Reply displayCmd(CommandContext& c) {
    const machine::DisplayOutput* o = c.host.display();
    const machine::MachineDescriptor* d = c.host.descriptor();
    if (!o || !d) return refusedReply(kNoMachine);
    return doneReply(displayValue(*o, d->display), displayLines(*o, d->display));
}

Reply stateCmd(CommandContext& c) {
    if (c.line.argText.empty()) return needs("state <topic>");
    auto t = c.host.readTopic(c.line.argText);
    if (!t) return refusedReply("There is no topic " + c.line.argText + ".");
    Value v = Value::object();
    v.set("topic", t->topic);
    std::vector<std::string> lines;
    if (!t->stream) {
        v.set("shape", "state");
        v.set("state", t->state ? fromSession(t->state->toValue()) : Value());
        lines.push_back(payloadText(t->state));
    } else {
        v.set("shape", "stream");
        v.set("reset", t->reset);
        Value items = Value::array();
        if (t->reset) lines.push_back("reset");
        for (const auto& i : t->items) {
            items.push(fromSession(i->toValue()));
            lines.push_back(payloadText(i));
        }
        v.set("items", std::move(items));
    }
    return doneReply(std::move(v), std::move(lines));
}

Reply capabilitiesCmd(CommandContext& c) {
    const ReplyMessage r = c.host.ask(Request{kCapabilitiesRequest, {}});
    if (!r.done) return refusedReply(r.reason);
    const auto* o = payloadAs<MachineOffer>(r.result);
    if (!o) return refusedReply("The session gave no capabilities.");
    std::vector<std::string> lines;
    Value caps = Value::array(), places = Value::array();
    for (const auto& e : o->capabilities) {
        const std::string owner = ownerText(e.owner);
        lines.push_back("capability " + std::string(e.kind.value) + " " + e.id.value + " " + owner);
        Value x = Value::object();
        x.set("kind", std::string(e.kind.value));
        x.set("id", e.id.value);
        x.set("owner", owner);
        caps.push(std::move(x));
    }
    // The machine's places, then the places of what is plugged in.
    std::vector<machine::PlacePath> all;
    for (const auto& p : o->places) all.push_back(machine::PlacePath{{p}});
    for (const auto& e : o->plugged.byPlace)
        if (std::find(all.begin(), all.end(), e.place) == all.end()) all.push_back(e.place);
    for (const auto& place : all) {
        Value plugged = Value::array();
        std::string text;
        for (const auto& e : o->plugged.byPlace)
            if (e.place == place) {
                const std::string one = e.peripheral.value + (e.variant ? ":" + e.variant->value : std::string());
                text += (text.empty() ? "" : " ") + one;
                plugged.push(one);
            }
        lines.push_back("place " + machine::placeText(place) + " " + (text.empty() ? std::string("empty") : text));
        Value x = Value::object();
        x.set("place", machine::placeText(place));
        x.set("plugged", std::move(plugged));
        places.push(std::move(x));
    }
    Value v = Value::object();
    v.set("capabilities", std::move(caps));
    v.set("places", std::move(places));
    return doneReply(std::move(v), std::move(lines));
}

Reply ticksCmd(CommandContext& c) {
    const ReplyMessage r = c.host.ask(Request{kTicksRequest, {}});
    if (!r.done) return refusedReply(r.reason);
    const auto* t = payloadAs<EmulatedTimeReply>(r.result);
    const machine::MachineDescriptor* d = c.host.descriptor();
    if (!t || !d) return refusedReply(kNoMachine);
    return timeReply(t->ticks, d->tickRate);
}

Reply audioCmd(CommandContext& c) {
    if (c.line.argText.empty()) return needs("audio <file>");
    const machine::MachineDescriptor* d = c.host.descriptor();
    if (!d) return refusedReply(kNoMachine);
    if (!d->hasSound) return refusedReply("This machine has no sound.");
    return c.host.record(c.line.argText);
}

Reply stateSaveCmd(CommandContext& c) {
    if (c.line.argText.empty()) return needs("state-save <file>");
    const ReplyMessage r = c.host.command(Command{kStateSaveCommand, {}});
    if (!r.done) return refusedReply(r.reason);
    const auto* b = payloadAs<StateBytes>(r.result);
    if (!b) return refusedReply("The session gave no state.");
    std::ofstream out(files::toPath(c.line.argText), std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(b->bytes.data()), static_cast<std::streamsize>(b->bytes.size()));
    if (!out) return refusedReply("Cannot write the file " + c.line.argText + ".");
    return doneReply();
}

Reply stateRestoreCmd(CommandContext& c) {
    if (c.line.argText.empty()) return needs("state-restore <file>");
    std::optional<std::vector<std::uint8_t>> bytes = files::readBytes(files::toPath(c.line.argText));
    if (!bytes) return refusedReply("Cannot read the file " + c.line.argText + ".");
    sessions::Value args = sessions::Value::object({{"bytes", sessions::Value(std::move(*bytes))}});
    return fromSessionReply(c.host.command(Command{kStateRestoreCommand, std::move(args)}));
}

Reply waitFinishedCmd(CommandContext& c) {
    if (!c.line.argText.empty()) return refusedReply("usage: wait-finished");
    return waitIdle(c, true);
}

// The product's answers now: takes keys, waits, the program has ended, and
// what keeps the machine busy.
Reply idleStateCmd(CommandContext& c) {
    const ReplyMessage r = c.host.ask(Request{presets::kIdleStateRequest, {}});
    if (!r.done) return refusedReply(r.reason);
    const sessions::Value v = r.result ? r.result->toValue() : sessions::Value();
    auto yes = [&](const char* key) {
        const sessions::Value* b = v.find(key);
        return b && b->boolean() && *b->boolean();
    };
    Value result = Value::object();
    result.set("takesKeys", yes("takesKeys")).set("waits", yes("waits")).set("ended", yes("ended"));
    std::vector<std::string> lines = {std::string("takes keys: ") + (yes("takesKeys") ? "yes" : "no"),
                                      std::string("waits: ") + (yes("waits") ? "yes" : "no"),
                                      std::string("ended: ") + (yes("ended") ? "yes" : "no")};
    std::string busy;
    if (const sessions::Value* b = v.find("busy"); b && b->elements())
        for (const sessions::Value& e : *b->elements())
            if (e.string()) busy += (busy.empty() ? "" : ", ") + *e.string();
    lines.push_back("busy: " + (busy.empty() ? std::string("nothing") : busy));
    return doneReply(std::move(result), std::move(lines));
}

} // namespace

void addFirstCommands(CommandRegistry& r) {
    r.add("wait", waitCmd, "wait [<duration>]        let emulated time pass: 2s, 1.5s, 500ms; none: until settled");
    r.add("wait-finished", waitFinishedCmd, "wait-finished            until the program has ended and the machine has settled");
    r.add("idle-state", idleStateCmd, "idle-state               takes keys, waits, ended, busy: the product's answers now");
    r.add("key-down", keyCmd(true), "key-down <key ID>        the key down");
    r.add("key-up", keyCmd(false), "key-up <key ID>          the key up");
    addTypingCommands(r);
    r.add("display-text", displayTextCmd, "display-text             the display as text");
    r.add("display", displayCmd, "display                  the display output as the machine gives it");
    r.add("state", stateCmd, "state <topic>            the state of a topic, or what a stream appended");
    r.add("capabilities", capabilitiesCmd, "capabilities             what the machine offers now; its places");
    r.add("ticks", ticksCmd, "ticks                    the emulated time");
    r.add("audio", audioCmd, "audio <file>             all sound output from now on into a WAV file");
    r.add("state-save", stateSaveCmd, "state-save <file>        the machine's full state into a file");
    r.add("state-restore", stateRestoreCmd, "state-restore <file>     continue from a saved state");
    r.add("quit", [](CommandContext& c) {
        c.quit = true;
        return doneReply();
    }, "quit                     end serve");
}

} // namespace calcu::harness
