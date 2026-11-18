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
    std::vector<media::KindDecl> kinds = cu1600::mediaKinds(kCards, catalogue);
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
    CHECK_EQ(k[cu1600::kModuleKind].suffix, std::string(".module.yaml"));
    CHECK_EQ(k[cu1600::kModuleKind].currentVersion, 1);
    CHECK(k[cu1600::kModuleKind].heldBy == media::HeldBy::ModuleSlot);
    CHECK_EQ(k[cu1600::kFloppyKind].suffix, std::string(".floppy.yaml"));
    CHECK_EQ(k[cu1600::kFloppyKind].currentVersion, 2);
    CHECK_EQ(k[cu1600::kFloppyKind].words.insertStep, std::string("floppy"));
    CHECK_EQ(k[cu1600::kCassetteKind].suffix, std::string(".cassette.yaml"));
    CHECK_EQ(k[cu1600::kCassetteKind].currentVersion, 1);
    CHECK(k[cu1600::kCassetteKind].folder == calcu::settings::Folder::Cassettes);
    CHECK_EQ(k[cu1600::kCassetteKind].words.ejectWord, std::string("cassette"));
    // The name tags: module-name, disk-name, cassette-name; template: true.
    const ReadOutcome m = readFile(k[cu1600::kModuleKind], fs::path(kCards) / "ce1600m.module.yaml");
    CHECK(m.status == ReadOutcome::Status::Read && m.info.name == "CE-1600M" && m.info.isTemplate);
    const ReadOutcome d = readFile(k[cu1600::kFloppyKind], fs::path(kCards) / "formatted.floppy.yaml");
    CHECK(d.status == ReadOutcome::Status::Read && d.info.name == "Formatted" && d.info.isTemplate);
    CHECK_EQ(d.info.description, std::string("Formatted, both sides empty"));
    const ReadOutcome t = readFile(k[cu1600::kCassetteKind], fs::path(kCards) / "empty.cassette.yaml");
    CHECK(t.status == ReadOutcome::Status::Read && t.info.name == "Empty" && t.info.isTemplate);
}

TEST(lib7_ac37_floppy_v2_sides_0_1_v1_outdated_with_hint) {
    const Kinds k;
    const media::KindDecl& floppy = k[cu1600::kFloppyKind];
    std::vector<std::uint8_t> image(CE1600FCard::kImageSize, 0);
    image[3] = 0x11;
    image[CE1600FCard::kSideSize + 5] = 0x22;
    const std::string v2 = formatFloppyFile("Disk", image, false, "", "2026-01-01T00:00:00Z");
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
    const media::KindDecl& module = k[cu1600::kModuleKind];
    const std::string source = text(fs::path(kCards) / "ce1600m.module.yaml");
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
    CHECK(saved.find("\nsaved: 2026-11-16T10:00:00Z\n") != std::string::npos);   // ledger 1600-51
    CHECK(saved.find("\nformat: memory-module\nformat-version: 1\n") != std::string::npos);
    CHECK(saved.rfind("# Calc-U-1600 memory module instance -- generated from ", 0) == 0);
    // A definition without a battery is a module, not a medium (D-9).
    CHECK(module.codec->read(text(fs::path(kCards) / "ce151.module.yaml"), {}).status == ReadOutcome::Status::NotMedium);
}

TEST(lib7_ac41_card_file_kept_but_content_name_template_mark) {
    const Kinds k;
    const media::KindDecl& module = k[cu1600::kModuleKind];
    const std::string source = text(fs::path(kCards) / "ce1600m.module.yaml");
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
    const media::KindDecl& floppy = k[cu1600::kFloppyKind];
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

// AC-9 (ledger 1600-54): the outdated-file notice names the User Guide's
// page and heading where each format is described.
TEST(lib7_ac9_outdated_notice_names_the_user_guide) {
    const Kinds k;
    CHECK_EQ(media::Shelf::outdatedHint(k[cu1600::kModuleKind], 0),
             std::string("The module definition format version 0 is outdated: the current version is 1, described in the "
                         "User Guide, \"Module definition format\"."));
    CHECK_EQ(media::Shelf::outdatedHint(k[cu1600::kFloppyKind], 1),
             std::string("The floppy disk format version 1 is outdated: the current version is 2, described in the "
                         "User Guide, \"Media files\", \"Floppy disk\" (sides 0: and 1: in place of a: and b:)."));
    CHECK_EQ(media::Shelf::outdatedHint(k[cu1600::kCassetteKind], 0),
             std::string("The cassette definition format version 0 is outdated: the current version is 1, described in the "
                         "User Guide, \"Media files\", \"Cassette definition\"."));
}

// AC-41 (ledger 1600-54): a cassette or disk name with a '"' or a line end
// would make the file unreadable; the writer refuses it, as the magnetic
// card's does.
TEST(lib7_ac41_cassette_and_disk_name_without_quote) {
    const Kinds k;
    media::MediumCodec::Write w;
    w.fileStem = "Bad";
    for (const char* name : {"Say \"hi\"", "Two\nlines"}) {
        w.name = name;
        std::string problem;
        w.content = std::make_shared<sharp::TapeContent>(wavMono16Bytes(std::vector<std::int16_t>(48, 0), 48000));
        CHECK(k[cu1600::kCassetteKind].codec->write(w, problem).empty());
        CHECK_EQ(problem, std::string("A cassette name has no \" and no line end."));
        problem.clear();
        w.content = std::make_shared<sharp::FloppyContent>(std::vector<std::uint8_t>(CE1600FCard::kImageSize, 0));
        CHECK(k[cu1600::kFloppyKind].codec->write(w, problem).empty());
        CHECK_EQ(problem, std::string("A disk name has no \" and no line end."));
    }
}

// ── P5: tapes (AC-42) ───────────────────────────────────────────────────

TEST(lib7_ac42_definition_with_wav_is_a_medium) {
    const Kinds k;
    const media::KindDecl& tape = k[cu1600::kCassetteKind];
    calcu::testing::TempFolder tmp("calcu1600-tape-codec");
    fs::create_directories(tmp.path() / "sounds");
    const std::vector<std::uint8_t> wav = wavMono16Bytes(std::vector<std::int16_t>(480, 1000), 48000);
    std::ofstream(tmp.path() / "sounds" / "games.wav", std::ios::binary).write(reinterpret_cast<const char*>(wav.data()),
                                                                                static_cast<std::streamsize>(wav.size()));
    const std::string def = "format: cassette\nformat-version: 1\ncassette-name: \"Games\"\n"
                            "description: \"Three games\"\naudio: { file: sounds/games.wav }\nposition: 0\n";
    std::ofstream(tmp.path() / "Games.cassette.yaml") << def;
    const ReadOutcome r = readFile(tape, tmp.path() / "Games.cassette.yaml");
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
    CHECK(tape.codec->read(moved, media::companionsBeside((tmp.path() / "x.cassette.yaml").generic_string())).status ==
          ReadOutcome::Status::Unreadable);
    std::ofstream(tmp.path() / "Lost.cassette.yaml") << "format: cassette\nformat-version: 1\ncassette-name: \"Lost\"\naudio: { file: none.wav }\n";
    const ReadOutcome lost = readFile(tape, tmp.path() / "Lost.cassette.yaml");
    CHECK(lost.status == ReadOutcome::Status::Unreadable && lost.problem.find("none.wav") != std::string::npos);
}

// AC-42 (ledger 1600-51): the old cassette names are not read.
TEST(lib7_ac42_old_format_cassette_tape_refused) {
    const Kinds k;
    const media::KindDecl& tape = k[cu1600::kCassetteKind];
    const ReadOutcome r =
        tape.codec->read("format: cassette-tape\nformat-version: 1\ntape-name: \"Old\"\naudio: { file: old.wav }\n", {});
    CHECK(r.status == ReadOutcome::Status::Unreadable);
    CHECK_EQ(r.problem, std::string("not a cassette definition (expected 'format: cassette')."));
    CHECK_EQ(tape.label, std::string("cassette"));
    CHECK_EQ(tape.words.insertStep, std::string("cassette"));
}

// AC-48: `format: memory-module` -- written always, read when there, another
// value refused, a file without it (the CE-502B's) still read.
TEST(lib7_ac48_module_definition_format_key) {
    const Kinds k;
    const media::KindDecl& module = k[cu1600::kModuleKind];
    const std::string source = text(fs::path(kCards) / "ce1600m.module.yaml");
    CHECK(source.find("\nformat: memory-module\nformat-version: 1\n") != std::string::npos);
    CHECK(module.codec->read(source, {}).status == ReadOutcome::Status::Read);
    std::string without = source;
    without.erase(without.find("format: memory-module\n"), std::string("format: memory-module\n").size());
    CHECK(module.codec->read(without, {}).status == ReadOutcome::Status::Read);
    std::string other = source;
    other.replace(other.find("format: memory-module"), std::string("format: memory-module").size(), "format: memory-card");
    const ReadOutcome refused = module.codec->read(other, {});
    CHECK(refused.status == ReadOutcome::Status::Unreadable &&
          refused.problem.find("not a module definition (expected 'format: memory-module')") != std::string::npos);
    // Written from a file without it: the instance has it.
    const ReadOutcome r = module.codec->read(without, {});
    media::MediumCodec::Write w;
    w.name = "Mine";
    w.fileStem = "Mine";
    w.content = r.info.content;
    w.source = without;
    w.savedAt = "2026-11-17T10:00:00Z";
    CHECK(writeOne(module, w).find("\nformat: memory-module\nformat-version: 1\n") != std::string::npos);
}

// AC-49: every medium's file starts with its comment line and carries
// `saved:`, the time of the write.
TEST(lib7_ac49_cassette_comment_and_saved) {
    const Kinds k;
    const media::KindDecl& tape = k[cu1600::kCassetteKind];
    media::MediumCodec::Write w;
    w.name = "Rec";
    w.fileStem = "Rec";
    w.content = std::make_shared<sharp::TapeContent>(wavMono16Bytes(std::vector<std::int16_t>(48, 0), 48000));
    w.savedAt = "2026-11-17T10:00:00Z";
    std::string problem;
    const auto files = tape.codec->write(w, problem);
    CHECK(!files.empty());
    if (files.empty()) return;
    const std::string& t = files.front().bytes;
    CHECK(t.rfind("# Calc-U-1600 cassette\nformat: cassette\nformat-version: 1\ncassette-name: \"Rec\"\n", 0) == 0);
    CHECK(t.find("\nsaved: 2026-11-17T10:00:00Z\n") != std::string::npos);
    CHECK(tape.codec->read(t, [&](const std::string&) -> std::optional<std::string> {
                  return std::string(files[1].bytes);
              }).status == ReadOutcome::Status::Read);
    const std::string floppy = formatFloppyFile("Disk", std::vector<std::uint8_t>(CE1600FCard::kImageSize, 0), false, "",
                                                "2026-11-17T10:00:00Z");
    CHECK(floppy.rfind("# Calc-U-1600 CE-1600F floppy disk image\n", 0) == 0 &&
          floppy.find("\nsaved: 2026-11-17T10:00:00Z\n") != std::string::npos);
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
// WAV, `cassette:` and `eject: cassette` (AC-42; LIB-7 AC-26).
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
    CHECK(m.find("\"empty.cassette.yaml\"") != std::string::npos);   // the bundled template, offered
    // A hand-written definition around an existing WAV (the fixture).
    const fs::path wav = fs::absolute("tests/fixtures/tape/pc1500_tape.wav");
    const fs::path def = h.write("fixture.cassette.yaml", "format: cassette\nformat-version: 1\ncassette-name: \"Fixture\"\n"
                                                      "template: true\naudio: { file: \"" +
                                                          fs::relative(wav, h.folder.path()).generic_string() + "\" }\n");
    CHECK(h.line("cassette { file: " + def.generic_string() + " }").rfind("done", 0) == 0);
    CHECK(h.line("type CLOAD").rfind("done", 0) == 0);
    CHECK(h.line("wait").rfind("done", 0) == 0);
    CHECK(h.line("display-text").find("ERROR") == std::string::npos);
    CHECK(h.line("eject cassette").rfind("done", 0) == 0);
    m = h.line("media");
    CHECK(m.find("\"place\":\"cassette\"") != std::string::npos && m.find("\"holds\"") == std::string::npos);
    // CSAVE onto the empty tape: the sound lives in the place until saved.
    CHECK(h.line("cassette { name: Empty }").rfind("done", 0) == 0);
    CHECK(h.line("type CSAVE \"RT\"").rfind("done", 0) == 0);
    CHECK(h.line("wait").rfind("done", 0) == 0);
    m = h.line("media");
    CHECK(m.find("\"unsaved\":true") != std::string::npos);
    CHECK(h.line("saveas { place: cassette, as: live, name: Rec }").rfind("done", 0) == 0);
    m = h.line("media");
    CHECK(m.find("\"Rec.cassette.yaml\"") != std::string::npos);
    // Its files: the definition and, beside it, its WAV (D-13: exported as they lie).
    const fs::path out = h.folder.path() / "out" / "Rec.cassette.yaml";
    fs::create_directories(out.parent_path());
    CHECK(h.line("media export cassette Rec " + out.generic_string()).rfind("done", 0) == 0);
    CHECK(text(out).find("audio: { file: \"Rec.wav\" }") != std::string::npos);
    CHECK(fs::exists(out.parent_path() / "Rec.wav") && fs::file_size(out.parent_path() / "Rec.wav") > 10u * 96000u);
    // A WAV on its own in the tape folder is no tape (row 1600-18).
    CHECK(m.find(".wav\"") == std::string::npos);
    // The saved tape plays back what was recorded.
    CHECK(h.line("eject cassette").rfind("done", 0) == 0);
    CHECK(h.line("type NEW").rfind("done", 0) == 0);
    CHECK(h.line("cassette { name: Rec }").rfind("done", 0) == 0);
    CHECK(h.line("type CLOAD \"RT\"").rfind("done", 0) == 0);
    CHECK(h.line("wait").rfind("done", 0) == 0);
    CHECK(h.line("display-text").find("ERROR") == std::string::npos);
}

} // namespace
