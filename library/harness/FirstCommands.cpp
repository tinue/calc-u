// The first commands (LIB-11 design §3.3; spec AC-9 to AC-12, AC-23, AC-24;
// EC-2, EC-5 to EC-7).
#include "formats/Duration.hpp"
#include "harness/Commands.hpp"
#include "harness/HarnessHost.hpp"
#include "harness/StatePart.hpp"
#include "machine/Attach.hpp"
#include "machine/Machine.hpp"
#include "presets/PresetsPart.hpp"
#include "sessions/LibraryTopics.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <variant>

namespace calcu::harness {

using namespace sessions;

namespace {

std::string lower(std::string_view s) {
    std::string out(s);
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

std::filesystem::path pathOf(const std::string& s) { return std::filesystem::path(std::u8string(s.begin(), s.end())); }

Reply fromSessionReply(const ReplyMessage& r) {
    if (!r.done) return refusedReply(r.reason);
    return doneReply();
}

Reply needs(const char* usage) { return refusedReply(std::string("usage: ") + usage); }

// The key as the `key` step resolves it (D-9): the key ID, then the ID in
// any case, then one of its other spellings in any case. An unknown key
// is passed as written, and the session refuses it.
std::string resolveKey(const machine::MachineDescriptor* d, const std::string& written) {
    if (!d) return written;
    for (const auto& k : d->keys)
        if (k.id.value == written) return written;
    const std::string w = lower(written);
    for (const auto& k : d->keys)
        if (lower(k.id.value) == w) return k.id.value;
    for (const auto& k : d->keys)
        for (const auto& o : k.otherSpellings)
            if (lower(o) == w) return k.id.value;
    return written;
}

std::string ownerText(const machine::Owner& o) {
    if (const auto* c = std::get_if<machine::ComponentOwner>(&o)) return "component:" + c->component.value;
    const auto& p = std::get<machine::PeripheralOwner>(o);
    return "peripheral:" + p.peripheral.value + "@" + machine::placeText(p.place);
}

// LIB-6 AC-34: `wait` without a duration waits until the machine is idle,
// at most an hour of emulated time (the counterpart of a preset's implicit wait).
Reply waitIdle(CommandContext& c) {
    const machine::MachineDescriptor* d = c.host.descriptor();
    if (!d) return refusedReply(kNoMachine);
    const machine::Machine* m = c.host.session().machineSession().machine();
    const machine::Ticks t0 = m ? m->tickCount() : 0;
    const ReplyMessage r = c.host.command(Command{presets::kWaitIdleCommand, {}});
    if (!r.done) {
        if (r.reason == "Unknown command: wait-idle.") return refusedReply("This build reads no presets of format 2 yet.");
        return refusedReply(r.reason);
    }
    m = c.host.session().machineSession().machine();
    const machine::Ticks ran = m ? m->tickCount() - t0 : 0;
    const bool hour = ran >= static_cast<machine::Ticks>(d->tickRate) * 3600;
    Reply reply = timeReply(ran, d->tickRate);
    reply.result.set("hourReached", hour);
    reply.lines.push_back(std::string("hour reached: ") + (hour ? "yes" : "no"));
    return reply;
}

Reply waitCmd(CommandContext& c) {
    if (c.line.argText.empty()) return waitIdle(c);
    std::string error;
    const auto us = formats::parseDuration(c.line.argText, &error);
    if (!us) return refusedReply(error);   // EC-2
    const machine::MachineDescriptor* d = c.host.descriptor();
    if (!d) return refusedReply(kNoMachine);
    const machine::Ticks ran = c.host.advance(ticksFor(*us, d->tickRate));
    return timeReply(ran, d->tickRate);
}

Handler keyCmd(bool down) {
    return [down](CommandContext& c) {
        if (c.line.argText.empty()) return needs(down ? "key-down <key ID>" : "key-up <key ID>");
        const std::string key = resolveKey(c.host.descriptor(), c.line.argText);
        sessions::Value a = sessions::Value::object({{"key", key}});
        return fromSessionReply(c.host.command(Command{down ? kKeyDownCommand : kKeyUpCommand, std::move(a)}));
    };
}

Reply lcdCmd(CommandContext& c) {
    const DisplayText t = c.host.displayText();
    return doneReply(lcdValue(t), lcdLines(t));
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
    if (!t) return refusedReply("There is no topic " + c.line.argText + ".");   // EC-5
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
    if (!d->hasSound) return refusedReply("This machine has no sound.");   // EC-6
    return c.host.record(c.line.argText);
}

Reply stateSaveCmd(CommandContext& c) {
    if (c.line.argText.empty()) return needs("state-save <file>");
    const ReplyMessage r = c.host.command(Command{kStateSaveCommand, {}});
    if (!r.done) return refusedReply(r.reason);
    const auto* b = payloadAs<StateBytes>(r.result);
    if (!b) return refusedReply("The session gave no state.");
    std::ofstream out(pathOf(c.line.argText), std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(b->bytes.data()), static_cast<std::streamsize>(b->bytes.size()));
    if (!out) return refusedReply("Cannot write the file " + c.line.argText + ".");
    return doneReply();
}

Reply stateRestoreCmd(CommandContext& c) {
    if (c.line.argText.empty()) return needs("state-restore <file>");
    std::ifstream in(pathOf(c.line.argText), std::ios::binary);
    if (!in) return refusedReply("Cannot read the file " + c.line.argText + ".");
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    sessions::Value args = sessions::Value::object({{"bytes", sessions::Value(std::move(bytes))}});
    return fromSessionReply(c.host.command(Command{kStateRestoreCommand, std::move(args)}));
}

} // namespace

void addFirstCommands(CommandRegistry& r) {
    r.add("wait", waitCmd, "wait [<duration>]        let emulated time pass: 2s, 1.5s, 500ms; none: until idle");
    r.add("key-down", keyCmd(true), "key-down <key ID>        the key down");
    r.add("key-up", keyCmd(false), "key-up <key ID>          the key up");
    addTypingCommands(r);
    r.add("lcd", lcdCmd, "lcd                      the display as text");
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
