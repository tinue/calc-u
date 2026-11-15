#pragma once
#include "TestParts.hpp"
#include "opening/Classify.hpp"
#include "opening/FileKind.hpp"
#include "opening/OpenPart.hpp"
#include "opening/Openers.hpp"
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

// The stand-ins of the opening tests (LIB-13 design §6): LIB-6 and LIB-7 as
// they look to the open part, recording what they are asked; two file kinds
// built for the tests; the rig: LIB-3's test session with the open part.
//
//   presets   `.tseg`, `.tdot`; a preset whose text holds `outdated` is
//             refused with a hint, one loaded as its content that holds
//             `file:` is refused because it names other files (EC-3); the
//             others start an operation of 100 ticks.
//   media     `.tcard.yaml` (the pass-through `reader` of test-seg),
//             `.tdisk.yaml` (the holding `drive` of test-dot, unless
//             `noDrive`), `.tmod.yaml` (a module: `slot-1` of test-seg, two
//             slots `card-1`, `card-2` on test-dot). Text with `outdated`:
//             refused with LIB-7's hint; with `template`: a template.
//             `passing` holds the places passing a medium through now.
//             An import is refused with `asks:` when its name is saved
//             already and the request is not confirmed.
//   kinds     `test.listing` (name `.lst`, Done) and `test.blob` (content
//             that starts with `BLOB`, an operation of 100 ticks; by name it
//             takes nothing).

namespace calcu::opening::test {

using namespace calcu::sessions;
using namespace calcu::sessions::test;
namespace mt = calcu::machine::test;

inline std::vector<std::uint8_t> bytesOf(std::string_view s) { return std::vector<std::uint8_t>(s.begin(), s.end()); }
inline std::string textOf(const OpenedFile& f) { return f.bytes ? std::string(f.bytes->begin(), f.bytes->end()) : std::string(); }

inline OpenedFile file(std::string name, std::string text, std::optional<std::string> path = std::nullopt) {
    return OpenedFile{std::move(name), std::move(path), bytesOf(text)};
}

inline std::optional<std::string> readFile(const std::string& path) {
    std::ifstream in(std::filesystem::path(std::u8string(path.begin(), path.end())), std::ios::binary);
    if (!in) return std::nullopt;
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

inline Operation ticksOp(Ticks n) { return runningFor(n); }

class TestPresets final : public PresetOpening {
public:
    std::vector<std::string> extensions() const override { return {".tseg", ".tdot"}; }
    CommandOutcome loadFile(const std::string& path, SessionContext&) override {
        files.push_back(path);
        const auto text = readFile(path);
        const std::string name = std::filesystem::path(std::u8string(path.begin(), path.end())).filename().string();
        if (!text) return Refused{name + " could not be read."};
        return verdict(name, *text, false);
    }
    CommandOutcome loadContent(const OpenedFile& f, SessionContext&) override {
        contents.push_back(f.name);
        return verdict(f.name, textOf(f), true);
    }
    std::vector<std::string> files, contents;

private:
    static CommandOutcome verdict(const std::string& name, const std::string& text, bool alone) {
        if (text.find("outdated") != std::string::npos)
            return Refused{name + " is a preset of format 1: set format-version: 2 (see the preset format)."};
        if (alone && text.find("file:") != std::string::npos)
            return Refused{name + " names other files, which cannot be found when it is opened alone."};
        return StartOperation{ticksOp(100)};
    }
};

class TestMedia final : public MediaOpening {
public:
    struct Use {
        std::string how;     // "in-place", "imported"
        std::string file, place;
        bool writtenBack = false;
    };
    std::vector<std::string> suffixes() const override { return {".tcard.yaml", ".tdisk.yaml", ".tmod.yaml"}; }
    MediumFile read(const OpenedFile& f) const override {
        MediumFile m;
        const std::string n = lowerAscii(f.name);
        if (endsWith(n, ".tcard.yaml")) m.kind = "test-card", m.kindName = "test card";
        else if (endsWith(n, ".tdisk.yaml")) m.kind = "test-disk", m.kindName = "test disk";
        else m.kind = "test-module", m.kindName = "test module", m.isModule = true;
        const std::string t = textOf(f);
        if (t.find("outdated") != std::string::npos)
            m.refusal = "The " + m.kindName + " file " + f.name + " is outdated: set format-version: 2 (see the media format).";
        m.isTemplate = t.find("template") != std::string::npos;
        ++reads;
        return m;
    }
    std::vector<std::string> placesFor(const MediumFile& m, const OpenedFile&, const machine::Machine& mc) const override {
        const bool seg = mc.descriptor().model == mt::kSeg;
        if (m.kind == "test-card") return seg ? std::vector<std::string>{"reader"} : std::vector<std::string>{};
        if (m.kind == "test-disk") return !seg && !noDrive ? std::vector<std::string>{"drive"} : std::vector<std::string>{};
        return seg ? std::vector<std::string>{"slot-1"} : std::vector<std::string>{"card-1", "card-2"};
    }
    std::vector<std::string> placesInCatalogue(const MediumFile& m) const override {
        if (m.kind == "test-card") return {"the reader of the test-seg"};
        if (m.kind == "test-disk") return {"the drive of the test-dot"};
        return {"slot-1 of the test-seg", "card-1 of the test-dot", "card-2 of the test-dot"};
    }
    std::optional<std::string> refusesNow(const std::string& place, const machine::Machine&) const override {
        if (passing.count(place)) return "A medium is passing through " + place + " now.";
        return std::nullopt;
    }
    CommandOutcome useWhereItLies(const OpenedFile& f, const std::string& place, SessionContext&) override {
        const MediumFile m = read(f);
        if (held.count(place)) ejected.push_back(held[place]);   // a holding place: the old medium out first
        if (place != "reader") held[place] = f.name;
        uses.push_back(Use{"in-place", f.name, place, !m.isTemplate && f.path.has_value()});
        if (m.isModule) return StartOperation{ticksOp(50)};   // the attach of the slot's picker
        return Done{};
    }
    CommandOutcome importAndUse(const OpenedFile& f, const std::optional<std::string>& place, bool confirmed,
                                SessionContext&) override {
        if (saved.count(f.name) && !confirmed) return Refused{"asks: A medium named " + f.name + " exists. Replace it?"};
        saved.insert(f.name);
        ++imports;
        if (place) {
            if (held.count(*place)) ejected.push_back(held[*place]);
            if (*place != "reader") held[*place] = f.name;
        }
        uses.push_back(Use{"imported", f.name, place.value_or(""), false});
        return Done{};
    }

    bool noDrive = false;
    std::set<std::string> passing, saved;
    std::map<std::string, std::string> held;
    std::vector<std::string> ejected;
    std::vector<Use> uses;
    int imports = 0;
    mutable int reads = 0;
};

class ListingKind final : public FileKind {
public:
    std::string_view name() const override { return "test.listing"; }
    bool takesName(std::string_view n) const override { return endsWith(n, ".lst"); }
    bool takes(const OpenedFile& f) const override { return endsWith(lowerAscii(f.name), ".lst"); }
    CommandOutcome open(const OpenedFile& f, Way w, SessionContext&) override {
        opened.push_back(f.name);
        ways.push_back(w);
        return Done{};
    }
    std::vector<std::string> opened;
    std::vector<Way> ways;
};

class BlobKind final : public FileKind {
public:
    std::string_view name() const override { return "test.blob"; }
    bool takesName(std::string_view) const override { return false; }
    bool takes(const OpenedFile& f) const override { return textOf(f).rfind("BLOB", 0) == 0; }
    CommandOutcome open(const OpenedFile& f, Way, SessionContext&) override {
        opened.push_back(f.name);
        if (textOf(f).find("refuse") != std::string::npos) return Refused{f.name + " is a blob this product does not take."};
        return StartOperation{ticksOp(100)};
    }
    std::vector<std::string> opened;
};

inline OpenSetup standInSetup(TestPresets& p, TestMedia& m, ListingKind& l, BlobKind& b) {
    OpenSetup s;
    s.productName = "Test-Product";
    s.presets = &p;
    s.media = &m;
    s.kinds = {&l, &b};
    return s;
}

// LIB-3's test session with the open part over the stand-ins.
struct OpenRig {
    explicit OpenRig(std::function<void(OpenSetup&)> adjust = {}, Hosting hosting = Hosting::Paced)
        : part(makeSetup(adjust)), rig(RigOptions{hosting, {}, TestFrontend::Delivery::AtOnce, {&part}}) {}

    OpenSetup makeSetup(const std::function<void(OpenSetup&)>& adjust) {
        OpenSetup s = standInSetup(presets, media, listing, blob);
        if (adjust) adjust(s);
        return s;
    }

    CallId open(OpenRequest r) { return rig.send("open", r.toValue()); }
    CallId open(OpenedFile f, Way w = Way::Drop, bool upload = false, bool confirmed = false) {
        return open(OpenRequest{{std::move(f)}, w, upload, confirmed});
    }
    // Opened, and every host call made until the session is free.
    const ReplyMessage* opened(OpenedFile f, Way w = Way::Drop, bool upload = false, bool confirmed = false) {
        const CallId c = open(std::move(f), w, upload, confirmed);
        rig.callWhileBusy();
        return rig.fe.reply(c);
    }
    const OpenReply* replyOf(const ReplyMessage* r) const { return r ? payloadAs<OpenReply>(r->result) : nullptr; }
    std::vector<std::string> notices() { return rig.fe.noticeSentences(); }
    bool wouldTake(const std::string& name, std::optional<std::int64_t> size = std::nullopt) {
        Value args = size ? Value::object({{"name", name}, {"size", *size}}) : Value::object({{"name", name}});
        const CallId c = rig.ask("open-would-take", std::move(args));
        const auto* r = rig.fe.reply(c);
        const auto* w = r ? payloadAs<WouldTake>(r->result) : nullptr;
        return w && w->takes;
    }

    TestPresets presets;
    TestMedia media;
    ListingKind listing;
    BlobKind blob;
    OpenPart part;
    Rig rig;
};

inline bool hasText(const std::vector<std::string>& lines, const std::string& text) {
    for (const auto& l : lines)
        if (l.find(text) != std::string::npos) return true;
    return false;
}

} // namespace calcu::opening::test
