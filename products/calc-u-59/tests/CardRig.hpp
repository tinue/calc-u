#pragma once
#include "SessionRig.hpp"
#include "../app/TIMagneticCardKind.hpp"
#include "media/MediaPart.hpp"
#include "opening/OpenPart.hpp"
#include "media/MediaTopics.hpp"
#include "startup/StartupPart.hpp"
#include "typing/TypingTopics.hpp"
#include <chrono>
#include <string>

// Calc-U-59's magnetic cards on the app session: SessionRig with its own save
// folder, plain files (the cards are the library's media part), and the card
// reader's commands as a
// frontend sends them: media.choose (Arm Card), media.start-new (Arm Fresh
// Card), media.feed, media.eject, c59.tap-card.

namespace testroms {

struct CardRig : SessionRig {
    using ReplyMessage = calcu::sessions::ReplyMessage;
    static constexpr const char* kPlace = "card-reader";

    explicit CardRig(cu59::Model model = cu59::Model::TI59) : SessionRig(model) {}

    /// A preset of format 2 by its content (the library's load-preset).
    ReplyMessage load(const std::string& text, const std::string& name = "test.ti59",
                      std::chrono::milliseconds limit = std::chrono::minutes(30)) {
        const auto id = post(calcu::presets::kLoadPresetCommand, Value::object({{"content", text}, {"name", name}}));
        until([&] { return reply(id) != nullptr; }, limit);
        if (const auto* r = reply(id)) return *r;
        return ReplyMessage{id, false, "no reply", nullptr};
    }
    /// A preset for the TI-59 with the Master Library module; `body` after the header.
    static std::string preset(const std::string& body, const std::string& header = {}) {
        return "format-version: 2\nmodel: TI-59\nslot-1: { name: ML }\n" + header + body;
    }
    ReplyMessage armSaved(const std::string& file) {
        return send(calcu::media::kChooseCommand, Value::object({{"place", kPlace}, {"entry", "saved:" + file}}));
    }
    ReplyMessage armFresh(const std::string& name, bool confirmed = false) {
        return send(calcu::media::kStartNewCommand, Value::object({{"place", kPlace},
                                                                   {"template", std::string("implicit:") + cu59::kFreshCardTemplate},
                                                                   {"name", name},
                                                                   {"confirmed", confirmed}}));
    }
    ReplyMessage feed(int side) { return send(calcu::media::kFeedCommand, Value::object({{"place", kPlace}, {"side", side}})); }
    ReplyMessage eject() { return send(calcu::media::kEjectCommand, Value::object({{"place", kPlace}})); }
    bool tap(double x, double y) {
        const auto r = send(cu59::commands::kTapCard, Value::object({{"x", x}, {"y", y}}));
        const auto* f = calcu::sessions::payloadAs<cu59::TIFlag>(r.result);
        return r.done && f && f->value;
    }
    /// `n 2nd Write` (or `n INV 2nd Write`) as keys; true when the ROM waits for a card then.
    bool keyWrite(int bank, bool inv = false) {
        Value::Array names;
        names.emplace_back(std::to_string(bank));
        if (inv) names.emplace_back("INV");
        names.emplace_back("Write");
        const auto r = operation(calcu::typing::kKey, Value::object({{"names", Value(std::move(names))}}), std::chrono::seconds(30));
        runFor(std::chrono::milliseconds(500));
        return r.done && waits();
    }
    bool waits() {
        auto* p = cardPlace(machine());
        return p && p->waitsForMedium();
    }
    bool passing() {
        auto* p = cardPlace(machine());
        return p && p->side().has_value();
    }
    /// Until the card in the reader has passed (and was written), then a little more.
    bool untilPassed(std::chrono::milliseconds limit = std::chrono::seconds(20)) {
        const bool ok = until([&] { return !passing(); }, limit);
        runFor(std::chrono::milliseconds(200));
        return ok;
    }
    bool untilKeyReady() { return until([&] { return !busy() && machine().isKeyReady(); }, std::chrono::seconds(5)); }
    const calcu::media::Binding* fitted() { return part().mediaPart().bindingOf(kPlace); }
    const calcu::media::MediaState* media() const { return state<calcu::media::MediaState>(calcu::media::kMediaTopic); }
    std::string cardFile(const std::string& name) const {
        return readFile((folder.path() / (name + ".magnetcard.yaml")).string()).value_or("");
    }
    bool hasCardFile(const std::string& name) const { return fs::exists(folder.path() / (name + ".magnetcard.yaml")); }
    void writeCardFile(const std::string& name, const std::string& text) const { folder.write(name + ".magnetcard.yaml", text); }
    /// A side of a card file as read by the kind's codec.
    static std::optional<std::vector<std::uint8_t>> sideOf(const std::string& text, int side) {
        const auto r = cu59::magneticCardKind().codec->read(text, {});
        const auto c = cu59::magneticCardOf(r.info.content);
        if (!c) return std::nullopt;
        if (const auto* b = std::get_if<std::vector<std::uint8_t>>(&c->sides[static_cast<std::size_t>(side)])) return *b;
        return std::nullopt;
    }
    /// A file dropped on the calculator: the library's `open` (a path: where it
    /// lies; none: an upload, as the Web). True when the open was done.
    bool drop(const std::string& name, const std::string& path, const std::optional<std::string>& bytes = std::nullopt) {
        calcu::opening::OpenRequest q;
        calcu::opening::OpenedFile f;
        f.name = name;
        if (!path.empty()) f.path = path;
        if (bytes) f.bytes = std::vector<std::uint8_t>(bytes->begin(), bytes->end());
        else if (!path.empty()) {
            const auto t = readFile(path);
            if (t) f.bytes = std::vector<std::uint8_t>(t->begin(), t->end());
        }
        q.files = {f};
        q.way = calcu::opening::Way::Drop;
        q.upload = path.empty();   // the Web: imported into the save folder
        return operation(calcu::opening::kOpenCommand, q.toValue()).done;
    }
    int progStep(int s) { return int(mem(machine()).readProgramStep(s)); }
    double reg(int n) { return mem(machine()).readDataReg(n); }
    std::vector<std::string> logLines() const {
        std::vector<std::string> out;
        for (const auto& m : fe.log)
            if (const auto* s = std::get_if<calcu::sessions::StreamMessage>(&m); s && s->topic == calcu::sessions::kLogTopic)
                for (const auto& n : s->appended)
                    if (const auto* x = calcu::sessions::payloadAs<calcu::sessions::LogLine>(n)) out.push_back(x->text);
        return out;
    }
    bool logged(const std::string& part) const {
        for (const auto& l : logLines())
            if (l.find(part) != std::string::npos) return true;
        return false;
    }
    bool noticed(const std::string& part) const {
        for (const auto& l : notices())
            if (l.find(part) != std::string::npos) return true;
        return false;
    }
};

/// Lbl A 1 2 3 R/S as a program section.
inline const std::string kProgramA = "program:\n  keycodes: |\n    76 11 01 02 03 91\n";
/// Lbl B 4 5 6 R/S.
inline const std::string kProgramB = "program:\n  keycodes: |\n    76 12 04 05 06 91\n";

} // namespace testroms
