// LIB-7, the Calc-U-1600 column (design §10.6 P1, P5): the three kinds of
// medium and their codecs (app/SharpMediaKinds; AC-37, AC-41, AC-42) and
// the tapes through the harness (the media part on the run's save folder).
// The files are read and written through the codecs, as the library does.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "../app/SharpMediaKinds.hpp"
#include "../core/Audio/WavFile.hpp"
#include "../core/Connector/CE1600FCard.hpp"
#include "../core/Connector/FloppyImageFile.hpp"
#include "../core/Connector/SoftwareDefinedCard.hpp"
#include "../core/Machine/SharpCatalogue.hpp"
#include "../core/Machine/SharpMediaPlaces.hpp"
#include "../app/tests/TestSettings.hpp"
#include "HarnessPreset.hpp"
#include "TempFolder.hpp"
#include "TestHarness.hpp"
#include "TestRoms.hpp"
#include "media/Kind.hpp"
#include "media/MediaPlace.hpp"
#include "media/Shelf.hpp"

namespace {

namespace fs = std::filesystem;
namespace media = calcu::media;
using media::ReadOutcome;

const char* kCards = "frontends/qt6/resources/cards";

struct Kinds {
    calcu::machine::Catalogue catalogue = sharp::sharpCatalogue();
    std::vector<media::KindDecl> kinds = shared::mediaKinds(kCards, catalogue);
    const media::KindDecl& operator[](const char* id) const {
        for (const auto& k : kinds)
            if (k.id == id) return k;
        return kinds.front();
    }
};

std::string text(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

ReadOutcome readFile(const media::KindDecl& k, const fs::path& p) {
    return k.codec->read(text(p), media::companionsBeside(p.generic_string()));
}

std::string writeOne(const media::KindDecl& k, const media::MediumCodec::Write& w) {
    std::string problem;
    const auto files = k.codec->write(w, problem);
    if (files.empty()) std::fprintf(stderr, "  write: %s\n", problem.c_str());
    return files.empty() ? std::string() : files.front().bytes;
}

// ── P1: the codecs (AC-37, AC-41) ───────────────────────────────────────

TEST(lib7_ac37_three_kinds_suffixes_versions_name_keys) {
    const Kinds k;
    CHECK_EQ(k.kinds.size(), std::size_t{3});
    CHECK_EQ(k[shared::kModuleKind].suffix, std::string(".card.yaml"));
    CHECK_EQ(k[shared::kModuleKind].currentVersion, 1);
    CHECK(k[shared::kModuleKind].heldBy == media::HeldBy::ModuleSlot);
    CHECK_EQ(k[shared::kFloppyKind].suffix, std::string(".floppy.yaml"));
    CHECK_EQ(k[shared::kFloppyKind].currentVersion, 2);
    CHECK_EQ(k[shared::kFloppyKind].words.insertStep, std::string("floppy"));
    CHECK_EQ(k[shared::kTapeKind].suffix, std::string(".tape.yaml"));
    CHECK_EQ(k[shared::kTapeKind].currentVersion, 1);
    CHECK(k[shared::kTapeKind].folder == calcu::settings::Folder::Tapes);
    CHECK_EQ(k[shared::kTapeKind].words.ejectWord, std::string("tape"));
    // The name tags: module-name, disk-name, tape-name; template: true.
    const ReadOutcome m = readFile(k[shared::kModuleKind], fs::path(kCards) / "ce1600m.card.yaml");
    CHECK(m.status == ReadOutcome::Status::Read && m.info.name == "CE-1600M" && m.info.isTemplate);
    const ReadOutcome d = readFile(k[shared::kFloppyKind], fs::path(kCards) / "formatted.floppy.yaml");
    CHECK(d.status == ReadOutcome::Status::Read && d.info.name == "Formatted" && d.info.isTemplate);
    CHECK_EQ(d.info.description, std::string("Formatted, both sides empty"));
    const ReadOutcome t = readFile(k[shared::kTapeKind], fs::path(kCards) / "empty.tape.yaml");
    CHECK(t.status == ReadOutcome::Status::Read && t.info.name == "Empty" && t.info.isTemplate);
}

TEST(lib7_ac37_floppy_v2_sides_0_1_v1_outdated_with_hint) {
    const Kinds k;
    const media::KindDecl& floppy = k[shared::kFloppyKind];
    std::vector<std::uint8_t> image(CE1600FCard::kImageSize, 0);
    image[3] = 0x11;
    image[CE1600FCard::kSideSize + 5] = 0x22;
    const std::string v2 = formatFloppyFile("Disk", image);
    CHECK(v2.find("format-version: 2") != std::string::npos);
    CHECK(v2.find("\n  0:\n") != std::string::npos && v2.find("\n  1:\n") != std::string::npos);
    const ReadOutcome r = floppy.codec->read(v2, {});
    CHECK(r.status == ReadOutcome::Status::Read);
    const auto* c = dynamic_cast<const sharp::FloppyContent*>(r.info.content.get());
    CHECK(c && c->image == image);
    // Version 1 (0.8.x, sides a and b) is outdated, its name still read.
    std::string v1 = v2;
    v1.replace(v1.find("format-version: 2"), 17, "format-version: 1");
    v1.replace(v1.find("\n  0:\n"), 6, "\n  a:\n");
    v1.replace(v1.find("\n  1:\n"), 6, "\n  b:\n");
    const ReadOutcome o = floppy.codec->read(v1, {});
    CHECK(o.status == ReadOutcome::Status::Outdated && o.info.formatVersion == 1 && o.info.name == "Disk");
    const std::string hint = media::Shelf::outdatedHint(floppy, 1);
    CHECK(hint.find("version 1 is outdated") != std::string::npos && hint.find("current version is 2") != std::string::npos);
    // Sides a and b at version 2: not read.
    std::string mixed = v1;
    mixed.replace(mixed.find("format-version: 1"), 17, "format-version: 2");
    CHECK(floppy.codec->read(mixed, {}).status == ReadOutcome::Status::Unreadable);
}

TEST(lib7_ac37_card_v1_read_and_written) {
    const Kinds k;
    const media::KindDecl& module = k[shared::kModuleKind];
    const std::string source = text(fs::path(kCards) / "ce1600m.card.yaml");
    const ReadOutcome r = module.codec->read(source, {});
    CHECK(r.status == ReadOutcome::Status::Read && r.info.formatVersion == 1);
    media::MediumCodec::Write w;
    w.name = "Mine";
    w.fileStem = "Mine";
    w.content = r.info.content;
    w.source = source;
    w.savedAt = "2026-11-16T10:00:00Z";
    const std::string saved = writeOne(module, w);
    const ReadOutcome again = module.codec->read(saved, {});
    CHECK(again.status == ReadOutcome::Status::Read && again.info.name == "Mine" && !again.info.isTemplate);
    CHECK(saved.find("# last-saved: 2026-11-16T10:00:00Z") != std::string::npos);
    // A definition without a battery is a module, not a medium (D-9).
    CHECK(module.codec->read(text(fs::path(kCards) / "ce151.card.yaml"), {}).status == ReadOutcome::Status::NotMedium);
}

TEST(lib7_ac41_card_file_kept_but_content_name_template_mark) {
    const Kinds k;
    const media::KindDecl& module = k[shared::kModuleKind];
    const std::string source = text(fs::path(kCards) / "ce1600m.card.yaml");
    const ReadOutcome r = module.codec->read(source, {});
    const auto* content = dynamic_cast<const sharp::ModuleContent*>(r.info.content.get());
    CHECK(content != nullptr);
    if (!content) return;
    // The RAM as the machine left it: one byte changed.
    SoftwareDefinedCard card(content->definition);
    std::vector<std::uint8_t> image = card.debugImage();
    image[0x123] = 0xA5;
    media::MediumCodec::Write w;
    w.name = "Kept";
    w.fileStem = "Kept";
    w.content = std::make_shared<sharp::ModuleContent>(sharp::definitionWithContent(content->definition, image, false));
    w.source = source;
    w.savedAt = "2026-11-16T10:00:00Z";
    const std::string saved = writeOne(module, w);
    // Everything but the content, the name and the template mark is the file's.
    CHECK(saved.find("module-name: \"Kept\"") != std::string::npos);
    CHECK(saved.find("template:") == std::string::npos);
    CHECK(saved.find("description:") != std::string::npos);
    const ReadOutcome again = module.codec->read(saved, {});
    const auto* back = dynamic_cast<const sharp::ModuleContent*>(again.info.content.get());
    CHECK(back && SoftwareDefinedCard(back->definition).debugImage() == image);
    // Saved as a template: the mark is written.
    w.asTemplate = true;
    CHECK(writeOne(module, w).find("template: true") != std::string::npos);
}

TEST(lib7_ac41_disk_written_whole_description_kept) {
    const Kinds k;
    const media::KindDecl& floppy = k[shared::kFloppyKind];
    const std::string source = text(fs::path(kCards) / "formatted.floppy.yaml");
    const ReadOutcome r = floppy.codec->read(source, {});
    media::MediumCodec::Write w;
    w.name = "Work";
    w.fileStem = "Work";
    w.content = r.info.content;
    w.source = source;
    w.savedAt = "2026-11-16T10:00:00Z";
    const std::string saved = writeOne(floppy, w);
    const ReadOutcome again = floppy.codec->read(saved, {});
    CHECK(again.status == ReadOutcome::Status::Read && again.info.name == "Work" && !again.info.isTemplate);
    CHECK_EQ(again.info.description, std::string("Formatted, both sides empty"));
    CHECK(saved.find("saved: 2026-11-16T10:00:00Z") != std::string::npos);
    const auto* a = dynamic_cast<const sharp::FloppyContent*>(r.info.content.get());
    const auto* b = dynamic_cast<const sharp::FloppyContent*>(again.info.content.get());
    CHECK(a && b && a->image == b->image);
}

// ── P5: tapes (AC-42) ───────────────────────────────────────────────────

TEST(lib7_ac42_definition_with_wav_is_a_medium) {
    const Kinds k;
    const media::KindDecl& tape = k[shared::kTapeKind];
    calcu::testing::TempFolder tmp("calcu1600-tape-codec");
    fs::create_directories(tmp.path() / "sounds");
    const std::vector<std::uint8_t> wav = wavMono16Bytes(std::vector<std::int16_t>(480, 1000), 48000);
    std::ofstream(tmp.path() / "sounds" / "games.wav", std::ios::binary).write(reinterpret_cast<const char*>(wav.data()),
                                                                                static_cast<std::streamsize>(wav.size()));
    const std::string def = "format: cassette-tape\nformat-version: 1\ntape-name: \"Games\"\n"
                            "description: \"Three games\"\naudio: { file: sounds/games.wav }\nposition: 0\n";
    std::ofstream(tmp.path() / "Games.tape.yaml") << def;
    const ReadOutcome r = readFile(tape, tmp.path() / "Games.tape.yaml");
    CHECK(r.status == ReadOutcome::Status::Read && r.info.name == "Games" && !r.info.isTemplate);
    CHECK_EQ(r.info.description, std::string("Three games"));
    CHECK(r.info.companions.size() == 1 && r.info.companions[0] == "sounds/games.wav");
    const auto* c = dynamic_cast<const sharp::TapeContent*>(r.info.content.get());
    CHECK(c && c->wav == wav);
    // Written back in place: the same sound file (D-26); a copy: beside it.
    media::MediumCodec::Write w;
    w.name = "Games";
    w.fileStem = "Games";
    w.content = r.info.content;
    w.source = def;
    std::string problem;
    auto files = tape.codec->write(w, problem);
    CHECK(files.size() == 2 && files[1].relativePath == "sounds/games.wav");
    w.name = "Copy";
    w.fileStem = "Copy";
    files = tape.codec->write(w, problem);
    CHECK(files.size() == 2 && files[1].relativePath == "Copy.wav");
    CHECK(files.size() == 2 && files[0].bytes.find("description: \"Three games\"") != std::string::npos);
    // `position` is reserved: 0; the sound file must be there.
    std::string moved = def;
    moved.replace(moved.find("position: 0"), 11, "position: 3");
    CHECK(tape.codec->read(moved, media::companionsBeside((tmp.path() / "x.tape.yaml").generic_string())).status ==
          ReadOutcome::Status::Unreadable);
    std::ofstream(tmp.path() / "Lost.tape.yaml") << "format: cassette-tape\nformat-version: 1\ntape-name: \"Lost\"\naudio: { file: none.wav }\n";
    const ReadOutcome lost = readFile(tape, tmp.path() / "Lost.tape.yaml");
    CHECK(lost.status == ReadOutcome::Status::Unreadable && lost.problem.find("none.wav") != std::string::npos);
}

// The cassette place: a blank tape records, the sound stays in the place
// and is written when the motor stops (takeWriteNow) -- the deck's part.
TEST(lib7_ac42_cassette_place_blank_records_tape_with_sound_plays) {
    TapeDeck deck(1'000'000.0);
    sharp::CassettePlace place(deck);
    CHECK(place.writesWhenAsked());
    CHECK(!place.side());
    CHECK(place.insert(std::make_shared<sharp::TapeContent>(wavMono16Bytes({}, 48000)), 0).ok);
    CHECK(deck.mode() == TapeDeck::Mode::Record && place.side() == 0);
    deck.setMotor(true);
    deck.setOutputLevel(true);
    deck.advance(500'000);   // half a second
    CHECK(!place.takeWriteNow());
    const std::uint64_t during = place.revision();
    CHECK(during > 0);
    deck.setMotor(false);
    CHECK(place.takeWriteNow());
    CHECK(!place.takeWriteNow());
    const auto recorded = std::dynamic_pointer_cast<const sharp::TapeContent>(place.content());
    CHECK(recorded && recorded->wav.size() > 44);
    // That sound inserted again plays.
    CHECK(place.eject() != nullptr && !place.side());
    CHECK(place.insert(recorded, 0).ok);
    CHECK(deck.mode() == TapeDeck::Mode::Play && place.content() == recorded);
    CHECK(!place.insert(std::make_shared<sharp::TapeContent>(std::vector<std::uint8_t>{1, 2, 3}), 0).ok);
}

// Through the harness: the empty tape, a recording kept in the place,
// Name & Save (`saveas`) writing the definition and the WAV side by side,
// a WAV alone not a tape, a hand-written definition around an existing
// WAV, `tape:` and `eject: tape` (AC-42; LIB-7 AC-26).
TEST(lib7_ac42_tape_flow_through_the_harness) {
    SKIP_UNLESS(fullRomSetPresent(), "no ROMs in roms/");
    presettest::HarnessPreset h;
    CHECK(h.ok());
    if (!h.ok()) return;
    CHECK(h.line("model PC-1500A").rfind("done", 0) == 0);
    CHECK(h.line("attach printer CE-150").rfind("done", 0) == 0);
    CHECK(h.line("key cl cl").rfind("done", 0) == 0);   // NEW0? :CHECK after the power cycle
    CHECK(h.line("type NEW0").rfind("done", 0) == 0);
    std::string m = h.line("media");
    CHECK(m.find("\"empty.tape.yaml\"") != std::string::npos);   // the bundled template, offered
    // A hand-written definition around an existing WAV (the fixture).
    const fs::path wav = fs::absolute("tests/fixtures/tape/pc1500_tape.wav");
    const fs::path def = h.write("fixture.tape.yaml", "format: cassette-tape\nformat-version: 1\ntape-name: \"Fixture\"\n"
                                                      "template: true\naudio: { file: \"" +
                                                          fs::relative(wav, h.folder.path()).generic_string() + "\" }\n");
    CHECK(h.line("tape { file: " + def.generic_string() + " }").rfind("done", 0) == 0);
    CHECK(h.line("type CLOAD").rfind("done", 0) == 0);
    CHECK(h.line("wait").rfind("done", 0) == 0);
    CHECK(h.line("lcd").find("ERROR") == std::string::npos);
    CHECK(h.line("eject tape").rfind("done", 0) == 0);
    m = h.line("media");
    CHECK(m.find("\"place\":\"tape\"") != std::string::npos && m.find("\"holds\"") == std::string::npos);
    // CSAVE onto the empty tape: the sound lives in the place until saved.
    CHECK(h.line("tape { name: Empty }").rfind("done", 0) == 0);
    CHECK(h.line("type CSAVE \"RT\"").rfind("done", 0) == 0);
    CHECK(h.line("wait").rfind("done", 0) == 0);
    m = h.line("media");
    CHECK(m.find("\"unsaved\":true") != std::string::npos);
    CHECK(h.line("saveas { place: tape, as: live, name: Rec }").rfind("done", 0) == 0);
    m = h.line("media");
    CHECK(m.find("\"Rec.tape.yaml\"") != std::string::npos);
    // Its files: the definition and, beside it, its WAV (D-13: exported as they lie).
    const fs::path out = h.folder.path() / "out" / "Rec.tape.yaml";
    fs::create_directories(out.parent_path());
    CHECK(h.line("media export cassette Rec " + out.generic_string()).rfind("done", 0) == 0);
    CHECK(text(out).find("audio: { file: \"Rec.wav\" }") != std::string::npos);
    CHECK(fs::exists(out.parent_path() / "Rec.wav") && fs::file_size(out.parent_path() / "Rec.wav") > 10u * 96000u);
    // A WAV on its own in the tape folder is no tape (row 1600-18).
    CHECK(m.find(".wav\"") == std::string::npos);
    // The saved tape plays back what was recorded.
    CHECK(h.line("eject tape").rfind("done", 0) == 0);
    CHECK(h.line("type NEW").rfind("done", 0) == 0);
    CHECK(h.line("tape { name: Rec }").rfind("done", 0) == 0);
    CHECK(h.line("type CLOAD \"RT\"").rfind("done", 0) == 0);
    CHECK(h.line("wait").rfind("done", 0) == 0);
    CHECK(h.line("lcd").find("ERROR") == std::string::npos);
}

} // namespace
