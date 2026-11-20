#pragma once
#include "MediaFixture.hpp"
#include "TempFolder.hpp"
#include "TestParts.hpp"
#include "opening/Classify.hpp"
#include "opening/FileKind.hpp"
#include "opening/OpenPart.hpp"
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

// The rig of the opening tests: the media tests' fixture (MediaFixture.hpp:
// the real presets and media parts over the media test catalogue, a save
// folder of the test's own) with the open part over them, the sessions'
// test part (`tp.op` starts an operation) and two file kinds built for the
// tests.
//
//   presets   `.test-seg`, `.TEST-DOT` (any case)
//   media     `.testcard.yaml` (the pass-through `reader` of test-seg),
//             `.testdisk.yaml` (the holding drive `disk` of test-dot),
//             `.testmodule.yaml` (a module: `slot-1` of test-seg, `card-1`
//             and `card-2` of test-dot); the kinds of media/tests/TestKinds.hpp
//   kinds     `test.listing` (name `.lst`, Done) and `test.blob` (content
//             that starts with `BLOB`, an operation of 100 ticks; by name it
//             takes nothing)

namespace calcu::opening::test {

using namespace calcu::media::test;
using calcu::sessions::CallId;
using calcu::sessions::CommandOutcome;
using calcu::sessions::Done;
using calcu::sessions::Refused;
using calcu::sessions::SessionContext;
using calcu::sessions::StartOperation;
using calcu::sessions::Value;

inline std::vector<std::uint8_t> bytesOf(std::string_view s) { return std::vector<std::uint8_t>(s.begin(), s.end()); }
inline std::string textOf(const OpenedFile& f) { return f.bytes ? std::string(f.bytes->begin(), f.bytes->end()) : std::string(); }

inline OpenedFile file(std::string name, std::string text, std::optional<std::string> path = std::nullopt) {
    return OpenedFile{std::move(name), std::move(path), bytesOf(text)};
}

// A preset whose keys are `steps`.
inline std::string keys(const std::string& model, const std::string& steps) { return header(model) + "keys:\n" + steps; }

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
        return StartOperation{calcu::sessions::test::runningFor(100)};
    }
    std::vector<std::string> opened;
};

struct OpenRigOptions {
    std::string model = "test-seg";
    std::function<void(OpenSetup&)> adjust;   // the open part's setup, changed before it is made
    bool start = true;                        // false: no machine until the case sends `start`
    // Paced: operations run over host calls, so a case can open a file while one runs.
    calcu::sessions::Hosting hosting = calcu::sessions::Hosting::Paced;
    std::function<void(calcu::testing::TempFolder& save, calcu::testing::TempFolder& bundled)> prepare;
};

struct OpenRig {
    explicit OpenRig(OpenRigOptions o = {}) : fixture(options(std::move(o))) {}
    explicit OpenRig(std::string model) : OpenRig(OpenRigOptions{std::move(model)}) {}

    FixtureOptions options(OpenRigOptions o) {
        FixtureOptions f;
        f.model = o.model;
        f.start = o.start;
        f.hosting = o.hosting;
        f.prepare = o.prepare;
        f.moreParts = [this, adjust = o.adjust](MediaFixture& m) -> std::vector<calcu::sessions::SessionPart*> {
            OpenSetup s;
            s.productName = "Test-Product";
            s.presets = m.presetsPart.get();
            s.presetExtensions = {".test-seg", ".TEST-DOT"};
            s.media = m.media.get();
            s.kinds = {&listing, &blob};
            if (adjust) adjust(s);
            part = std::make_unique<OpenPart>(std::move(s));
            return {part.get(), &tp};
        };
        return f;
    }

    // The open part's classification of a file, with its setup.
    Classified classify(const OpenedFile& f, Way w = Way::Drop) const { return opening::classify(f, w, part->setup()); }

    CallId open(OpenRequest r) { return fixture.send("open", r.toValue()); }
    CallId open(OpenedFile f, Way w = Way::Drop, bool upload = false, bool confirmed = false) {
        return open(OpenRequest{{std::move(f)}, w, upload, confirmed});
    }
    // Opened, and every host call made until the session is free.
    const calcu::sessions::ReplyMessage* opened(OpenedFile f, Way w = Way::Drop, bool upload = false, bool confirmed = false) {
        const CallId c = open(std::move(f), w, upload, confirmed);
        fixture.callWhileBusy();
        return fixture.fe.reply(c);
    }
    // A file in `dir`, as a host hands it over: its path and bytes.
    OpenedFile onDisk(const calcu::testing::TempFolder& dir, const std::string& name) const {
        const auto bytes = calcu::testing::readText(dir.path() / name);
        return OpenedFile{name, dir / name, bytesOf(bytes)};
    }
    const OpenReply* replyOf(const calcu::sessions::ReplyMessage* r) const {
        return r ? calcu::sessions::payloadAs<OpenReply>(r->result) : nullptr;
    }
    std::vector<std::string> notices() { return fixture.notices(); }
    bool wouldTake(const std::string& name, std::optional<std::int64_t> size = std::nullopt) {
        Value args = size ? Value::object({{"name", name}, {"size", *size}}) : Value::object({{"name", name}});
        const CallId c =
            fixture.session->ask(fixture.id, calcu::sessions::Request{calcu::sessions::RequestName{"open-would-take"}, std::move(args)});
        const auto* r = fixture.fe.reply(c);
        const auto* w = r ? calcu::sessions::payloadAs<WouldTake>(r->result) : nullptr;
        return w && w->takes;
    }
    // The running machine's model.
    std::string model() { return fixture.machine() ? fixture.machine()->descriptor().model.value : std::string(); }
    // `tp.op`: an operation of `ticks` runs.
    CallId runOperation(machine::Ticks ticks) {
        tp.operation = calcu::sessions::test::runningFor(ticks);
        return fixture.send("tp.op");
    }

    ListingKind listing;
    BlobKind blob;
    calcu::sessions::test::TestPart tp;
    std::unique_ptr<OpenPart> part;
    MediaFixture fixture;   // last: built after the parts it is handed
};

inline bool hasText(const std::vector<std::string>& lines, const std::string& text) {
    for (const auto& l : lines)
        if (l.find(text) != std::string::npos) return true;
    return false;
}

} // namespace calcu::opening::test
