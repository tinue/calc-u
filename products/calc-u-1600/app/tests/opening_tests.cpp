// LIB-13, the Calc-U-1600 column (design Part D, D6 T16-1 to T16-6):
// opening a file on the product's app session -- its preset extensions,
// media suffixes and the two program kinds (app/SharpFileKinds) in the
// library's open part, what follows for a program file, media, the limits,
// the start argument and the dialog folders; and today's cases of telling a
// dropped file (the old test_drop_targets of tests/program_file_tests.cpp),
// through the product's open part. The classification cases build the
// session but start no machine; the others start one (real ROMs, SKIP
// without them).

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

#include "Machine/SharpMediaPlaces.hpp"
#include "SessionFixture.hpp"
#include "TestHarness.hpp"
#include "app/SharpFileKinds.hpp"
#include "media/MediaPlace.hpp"
#include "opening/Classify.hpp"
#include "opening/OpenPart.hpp"
#include "settings/OpenFolders.hpp"

namespace {

namespace fs = std::filesystem;
namespace opening = calcu::opening;
namespace sessions = calcu::sessions;
using opening::OpenedFile;
using opening::Way;
using sessionfixture::SessionFixture;

using Bytes = std::vector<std::uint8_t>;

// ---- Program files (as tests/program_file_tests.cpp builds them) ----

Bytes text(const std::string& s) { return Bytes(s.begin(), s.end()); }

Bytes basicPayload() {   // 10 END / 20 PRINT "AB" / 180 <tok>:END
    return {0x00, 0x0A, 0x03, 0xF1, 0x8E, 0x0D, 0x00, 0x14, 0x07, 0xF0, 0x97, 0x22, 0x41, 0x42,
            0x22, 0x0D, 0x00, 0xB4, 0x06, 0xF1, 0xB3, 0x3A, 0xF1, 0x8E, 0x0D};
}

const Bytes kCode = {0x3E, 0x41, 0xC9, 0x00, 0x11};   // 5 bytes

// CE-158 header: '@' BASIC, 'B' machine code; the length stored as length - 1.
Bytes ce158(const Bytes& payload, std::uint8_t type, std::uint16_t load = 0) {
    Bytes f(27, 0x00);
    f[0] = 0x01;
    f[1] = type;
    f[2] = 'C';
    f[3] = 'O';
    f[4] = 'M';
    const char* name = "SAMPLE";
    for (int i = 0; name[i]; i++) f[5 + i] = static_cast<std::uint8_t>(name[i]);
    const auto wire = static_cast<std::uint16_t>(payload.size() - 1);
    f[0x15] = static_cast<std::uint8_t>(load >> 8);
    f[0x16] = static_cast<std::uint8_t>(load & 0xFF);
    f[0x17] = static_cast<std::uint8_t>(wire >> 8);
    f[0x18] = static_cast<std::uint8_t>(wire & 0xFF);
    f.insert(f.end(), payload.begin(), payload.end());
    return f;
}

// PC-1600 header: 0x21 BASIC, 0x10 machine code; 24-bit little-endian fields.
Bytes pc1600(const Bytes& payload, std::uint8_t type, std::uint32_t load = 0) {
    Bytes f = {0xFF, 0x10, 0x00, 0x00, type};
    for (std::uint32_t v : {static_cast<std::uint32_t>(payload.size()), load, std::uint32_t{0}}) {
        f.push_back(static_cast<std::uint8_t>(v & 0xFF));
        f.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
        f.push_back(static_cast<std::uint8_t>((v >> 16) & 0xFF));
    }
    f.push_back(0x00);
    f.push_back(0x0F);
    f.insert(f.end(), payload.begin(), payload.end());
    return f;
}

Bytes z80Code() {   // SharpDataExchange's Z80 sample (src/cpu_guess.rs)
    static const std::uint8_t kBlock[] = {0x21, 0x00, 0x80, 0x06, 0x10, 0x7E, 0xFE, 0x20, 0x28,
                                          0x01, 0x23, 0x10, 0xF8, 0xCD, 0x00, 0x10, 0xC9};
    Bytes out;
    for (int i = 0; i < 8; i++) out.insert(out.end(), std::begin(kBlock), std::end(kBlock));
    return out;
}

Bytes pseudoRandom(std::size_t len, std::uint32_t seed) {
    Bytes out(len);
    for (std::uint8_t& b : out) {
        seed = seed * 1664525u + 1013904223u;
        b = static_cast<std::uint8_t>(seed >> 24);
    }
    return out;
}

Bytes fileBytes(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    return Bytes(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

// The fixture tape twice in one WAV: its data chunk's samples doubled.
Bytes doubledTape(const Bytes& wav) {
    for (std::size_t i = 12; i + 8 <= wav.size();) {
        const std::uint32_t size = wav[i + 4] | wav[i + 5] << 8 | wav[i + 6] << 16 | std::uint32_t(wav[i + 7]) << 24;
        if (std::equal(wav.begin() + static_cast<std::ptrdiff_t>(i), wav.begin() + static_cast<std::ptrdiff_t>(i + 4), "data")) {
            Bytes out(wav.begin(), wav.begin() + static_cast<std::ptrdiff_t>(i + 8 + size));
            out.insert(out.end(), wav.begin() + static_cast<std::ptrdiff_t>(i + 8), wav.begin() + static_cast<std::ptrdiff_t>(i + 8 + size));
            const std::uint32_t doubled = size * 2, riff = static_cast<std::uint32_t>(out.size() - 8);
            for (int b = 0; b < 4; ++b) {
                out[i + 4 + static_cast<std::size_t>(b)] = static_cast<std::uint8_t>(doubled >> (8 * b));
                out[4 + static_cast<std::size_t>(b)] = static_cast<std::uint8_t>(riff >> (8 * b));
            }
            return out;
        }
        i += 8 + size + (size & 1);
    }
    return {};
}

// A WAV with no tape on it: one second of silence.
Bytes silentWav() {
    Bytes w = {'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ', 16, 0,  0,   0,   1, 0,
               1,   0,   0x40, 0x1F, 0, 0, 0x40, 0x1F, 0, 0, 1,   0,   8,   0,   'd', 'a', 't', 'a', 0x40, 0x1F, 0, 0};
    w.insert(w.end(), 8000, 0x80);
    return w;
}

const char* kCards = "frontends/qt6/resources/cards";

// ---- The product's open part ----

OpenedFile named(const std::string& name, Bytes bytes, std::optional<std::string> path = std::nullopt) {
    return OpenedFile{name, std::move(path), std::move(bytes)};
}
OpenedFile onDisk(const fs::path& p) {
    std::optional<Bytes> b;
    if (fs::is_regular_file(p)) b = fileBytes(p);
    return OpenedFile{p.filename().generic_string(), fs::absolute(p).generic_string(), std::move(b)};
}

// What the product's open part takes the file for (AC-1's tests in order):
// "refused", "preset", "medium", or the file kind's name.
std::string takenAs(SessionFixture& f, const OpenedFile& file, Way way = Way::Drop) {
    const opening::Classified c = opening::classify(file, way, f.part().openPart().setup());
    switch (c.what) {
        case opening::Classified::What::Refused: return "refused";
        case opening::Classified::What::Preset: return "preset";
        case opening::Classified::What::Medium: return "medium";
        case opening::Classified::What::Kind: return std::string(c.kind->name());
    }
    return "?";
}
std::string takenAs(SessionFixture& f, const Bytes& bytes, const std::string& name = "dropped.bin") {
    return takenAs(f, named(name, bytes));
}

// A session built (its parts), not started.
struct Built {
    SessionFixture f{"calcu-opening-test"};
    Built() { f.open(); }
};

// The command `open` as a frontend sends it, run to its end.
sessions::ReplyMessage openFile(SessionFixture& f, const OpenedFile& file, Way way, bool upload = false) {
    opening::OpenRequest r;
    r.files = {file};
    r.way = way;
    r.upload = upload;
    return f.run(opening::kOpenCommand, r.toValue());
}
const opening::OpenReply* replyOf(const sessions::ReplyMessage& r) { return sessions::payloadAs<opening::OpenReply>(r.result); }

std::vector<const shared::ProgramOpenedPayload*> programsOpened(SessionFixture& f,
                                                               std::vector<sessions::PayloadPtr>& keep) {
    keep = f.listener.mirror().takeAppended(shared::topics::kProgramOpened);
    std::vector<const shared::ProgramOpenedPayload*> out;
    for (const auto& p : keep)
        if (const auto* o = sessions::payloadAs<shared::ProgramOpenedPayload>(p)) out.push_back(o);
    return out;
}

bool contains(const std::vector<std::string>& v, const std::string& part) {
    for (const auto& s : v)
        if (s.find(part) != std::string::npos) return true;
    return false;
}

calcu::media::MediaPlace* placeCap(SessionFixture& f, const char* id) {
    calcu::machine::Machine* m = f.part().machine();
    if (!m) return nullptr;
    for (const auto& e : m->capabilities(calcu::media::MediaPlace::kKind))
        if (auto* p = calcu::machine::capabilityAs<calcu::media::MediaPlace>(*m, e.id); p && p->id().value == id) return p;
    return nullptr;
}

std::string fileText(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

// ---- T16-1: the product's data, the two kinds (AC-17) ----

TEST(lib13_ac17_preset_extensions_media_suffixes) {
    Built b;
    const opening::OpenSetup& s = b.f.part().openPart().setup();
    CHECK_EQ(s.productName, std::string("Calc-U-1600"));
    CHECK(s.presets && s.presets->extensions() == std::vector<std::string>({".pc1500", ".pc1500a", ".pc1600"}));
    CHECK(s.media && s.media->suffixes() == std::vector<std::string>({".card.yaml", ".floppy.yaml", ".tape.yaml"}));
    // A preset by its extension in any case, whatever it holds (EC-1).
    CHECK_EQ(takenAs(b.f, text("10 PRINT 1\n"), "Hanoi.PC1500A"), std::string("preset"));
    CHECK_EQ(takenAs(b.f, Bytes{}, "x.pc1600"), std::string("preset"));
    CHECK_EQ(takenAs(b.f, text("anything"), "Mine.FLOPPY.yaml"), std::string("medium"));
    CHECK_EQ(takenAs(b.f, text("anything"), "x.card.yaml"), std::string("medium"));
    CHECK_EQ(takenAs(b.f, text("anything"), "x.tape.yaml"), std::string("medium"));
}

TEST(lib13_ac17_kinds_asked_basic_then_code) {
    Built b;
    const opening::OpenSetup& s = b.f.part().openPart().setup();
    CHECK(s.kinds.size() == 2);
    CHECK(s.kinds.size() == 2 && s.kinds[0]->name() == "basic-program" && s.kinds[1]->name() == "machine-code-program");
}

TEST(lib13_ac17_listing_and_tokenized_are_basic) {
    Built b;
    CHECK_EQ(takenAs(b.f, text("10 PRINT \"HI\"\n20 END\n")), std::string("basic-program"));
    CHECK_EQ(takenAs(b.f, ce158(basicPayload(), 0x40)), std::string("basic-program"));
    CHECK_EQ(takenAs(b.f, pc1600(basicPayload(), 0x21)), std::string("basic-program"));
}

TEST(lib13_ac17_headered_code_is_code) {
    Built b;
    CHECK_EQ(takenAs(b.f, ce158(kCode, 0x42, 0x40C5)), std::string("machine-code-program"));
    CHECK_EQ(takenAs(b.f, pc1600(kCode, 0x10, 0xC0C5)), std::string("machine-code-program"));
    CHECK_EQ(takenAs(b.f, pc1600(kCode, 0x10, 0xC0C5), "x.dat"), std::string("machine-code-program"));   // a header needs no name
}

TEST(lib13_ac17_headerless_code_only_with_bin) {
    Built b;
    CHECK_EQ(takenAs(b.f, z80Code(), "code.bin"), std::string("machine-code-program"));
    CHECK_EQ(takenAs(b.f, z80Code(), "CODE.BIN"), std::string("machine-code-program"));
    CHECK_EQ(takenAs(b.f, z80Code(), "code.jpg"), std::string("refused"));
    CHECK_EQ(takenAs(b.f, z80Code(), "code"), std::string("refused"));
    CHECK_EQ(takenAs(b.f, kCode, "short.bin"), std::string("refused"));   // too short to look like code
}

TEST(lib13_ac17_rom_is_not_code) {
    Built b;
    CHECK_EQ(takenAs(b.f, z80Code(), "CODE.ROM"), std::string("refused"));
    CHECK_EQ(takenAs(b.f, z80Code(), "code.rom"), std::string("refused"));
    CHECK_EQ(takenAs(b.f, pc1600(kCode, 0x10, 0xC0C5), "code.rom"), std::string("machine-code-program"));   // by its header
}

TEST(lib13_ac17_wav_goes_to_first_files_kind) {
    Built b;
    const Bytes wav = fileBytes("tests/fixtures/tape/pc1500_tape.wav");
    SKIP_UNLESS(!wav.empty(), "tests/fixtures/tape/pc1500_tape.wav missing");
    CHECK_EQ(takenAs(b.f, wav, "tape.wav"), std::string("basic-program"));
    CHECK_EQ(takenAs(b.f, doubledTape(wav), "two.wav"), std::string("basic-program"));
    // The file-kind request names each file's kind, for the flow (AC-18).
    const sessions::ReplyMessage r =
        b.f.listener.ask(shared::requests::kFileKind, sessions::Value::object({{"bytes", sessions::Value(doubledTape(wav))}}));
    const auto* k = sessions::payloadAs<shared::FileKindReply>(r.result);
    CHECK(k && k->tapeKinds == std::vector<std::string>({"basic-pc1500", "basic-pc1500"}));
}

TEST(lib13_ac17_unreadable_wav_goes_to_basic_loader_which_says_why) {
    SKIP_UNLESS(sessionfixture::romsPresent(), "no ROMs in roms/");
    SessionFixture f("calcu-opening-test");
    CHECK(f.start("PC-1500A"));
    CHECK_EQ(takenAs(f, silentWav(), "x.wav"), std::string("basic-program"));
    const sessions::ReplyMessage r = openFile(f, named("x.wav", silentWav()), Way::Harness);
    CHECK(!r.done);
    CHECK(r.reason.rfind("x.wav: ", 0) == 0);
    CHECK(contains(f.notices(), "x.wav: "));
}

TEST(lib13_ac17_empty_and_other_taken_by_neither) {
    Built b;
    CHECK_EQ(takenAs(b.f, Bytes{}), std::string("refused"));
    CHECK_EQ(takenAs(b.f, pseudoRandom(4096, 7), "data.bin"), std::string("refused"));
    CHECK_EQ(takenAs(b.f, text("Just some notes,\nnot a program.\n"), "notes.txt"), std::string("refused"));
    // Test 5 follows: a model line makes it a preset.
    CHECK_EQ(takenAs(b.f, text("model: PC-1600\n"), "x.bin"), std::string("preset"));
}

// ---- T16-2: what follows (AC-18) ----

TEST(lib13_ac18_gui_way_sends_program_opened_topic) {
    SKIP_UNLESS(sessionfixture::romsPresent(), "no ROMs in roms/");
    SessionFixture f("calcu-opening-test");
    CHECK(f.start("PC-1600"));
    std::vector<sessions::PayloadPtr> keep;
    (void)programsOpened(f, keep);
    for (Way w : {Way::Drop, Way::OpenWith, Way::Start, Way::Dialog}) {
        const sessions::ReplyMessage r = openFile(f, named("prog.bas", text("10 PRINT 1\n"), "/x/prog.bas"), w);
        CHECK(r.done);
        const opening::OpenReply* o = replyOf(r);
        CHECK(o && o->as == opening::OpenedAs::FileKind && o->detail == "basic-program");
        const auto opened = programsOpened(f, keep);
        CHECK(opened.size() == 1);
        if (opened.size() == 1) {
            CHECK_EQ(opened[0]->name, std::string("prog.bas"));
            CHECK_EQ(opened[0]->kind, std::string("basic-program"));
            CHECK_EQ(opened[0]->path, std::string("/x/prog.bas"));
            CHECK(opened[0]->bytes == text("10 PRINT 1\n"));
        }
    }
    const sessions::ReplyMessage code = openFile(f, named("c.bin", z80Code()), Way::Drop);
    CHECK(code.done && replyOf(code) && replyOf(code)->detail == "machine-code-program");
    const auto opened = programsOpened(f, keep);
    CHECK(opened.size() == 1 && opened[0]->kind == "machine-code-program");
}

TEST(lib13_ac18_kept_file_also_sends_topic) {
    SKIP_UNLESS(sessionfixture::romsPresent(), "no ROMs in roms/");
    SessionFixture f("calcu-opening-test");
    f.settings.setStartupModelPreference("PC-1500A");
    f.open();
    // Before the start (no machine): kept (AC-7), as Open With at a cold launch.
    opening::OpenRequest req;
    req.files = {named("prog.bas", text("10 PRINT 1\n"))};
    req.way = Way::OpenWith;
    const sessions::CallId call = f.send(opening::kOpenCommand, req.toValue());
    const auto kept = f.takeReply(call);
    CHECK(kept && kept->done && replyOf(*kept) && replyOf(*kept)->as == opening::OpenedAs::Kept);
    CHECK(f.ok(shared::commands::kStart));
    f.call();
    std::vector<sessions::PayloadPtr> keep;
    const auto opened = programsOpened(f, keep);
    CHECK(opened.size() == 1 && opened[0]->name == "prog.bas");
}

// ---- T16-3: media (AC-19, EC-2) ----

TEST(lib13_ac19_disk_used_where_it_lies_written_back) {
    SKIP_UNLESS(sessionfixture::romsPresent(), "no ROMs in roms/");
    SessionFixture f("calcu-opening-test");
    CHECK(f.start("PC-1600"));
    CHECK(f.attach("CE-1600P"));
    // An instance (no `template: true`) outside the save folder.
    std::string disk = fileText(fs::path(kCards) / "formatted.floppy.yaml");
    const std::size_t t = disk.find("template: true\n");
    CHECK(t != std::string::npos);
    if (t != std::string::npos) disk.erase(t, 15);
    const fs::path where = f.root / "elsewhere" / "Mine.floppy.yaml";
    fs::create_directories(where.parent_path());
    std::ofstream(where, std::ios::binary) << disk;
    const sessions::ReplyMessage r = openFile(f, onDisk(where), Way::Drop);
    CHECK(r.done);
    CHECK(replyOf(r) && replyOf(r)->as == opening::OpenedAs::Medium && replyOf(r)->detail == "floppy");
    const calcu::media::Binding* held = f.held("floppy");
    CHECK(held && held->instance() && fs::path(held->path) == fs::absolute(where));
    CHECK(!fs::exists(fs::path(f.saveFolder()) / "Mine.floppy.yaml"));   // not copied
    // The drive writes: saved back where it lies.
    calcu::media::MediaPlace* p = placeCap(f, "floppy");
    const auto c = p ? std::dynamic_pointer_cast<const sharp::FloppyContent>(p->content()) : nullptr;
    CHECK(c != nullptr);
    if (p && c) {
        std::vector<std::uint8_t> image = c->image;
        image[100] ^= 0xFF;
        p->insert(std::make_shared<sharp::FloppyContent>(image), p->side().value_or(0));
    }
    for (int i = 0; i < 60; ++i) f.call();
    CHECK(fileText(where) != disk);
}

TEST(lib13_ec2_disk_without_drive_refused_with_notice) {
    SKIP_UNLESS(sessionfixture::romsPresent(), "no ROMs in roms/");
    SessionFixture f("calcu-opening-test");
    CHECK(f.start("PC-1600"));
    (void)f.notices();
    const sessions::ReplyMessage r = openFile(f, onDisk(fs::path(kCards) / "formatted.floppy.yaml"), Way::OpenWith);
    CHECK(!r.done);
    CHECK(r.reason.find("formatted.floppy.yaml") != std::string::npos);
    CHECK(contains(f.notices(), "has no place for formatted.floppy.yaml"));
    CHECK(f.held("floppy") == nullptr);
}

TEST(lib13_ac19_upload_disk_imported_then_inserted) {
    SKIP_UNLESS(sessionfixture::romsPresent(), "no ROMs in roms/");
    SessionFixture f("calcu-opening-test");
    CHECK(f.start("PC-1600"));
    CHECK(f.attach("CE-1600P"));
    // A disk of a name not on the shelf yet (the bundled Formatted is).
    std::string disk = fileText(fs::path(kCards) / "formatted.floppy.yaml");
    const std::string formatted = "disk-name: \"Formatted\"";
    const std::size_t n = disk.find(formatted);
    CHECK(n != std::string::npos);
    if (n != std::string::npos) disk.replace(n, formatted.size(), "disk-name: \"Uploaded\"");
    const OpenedFile up = named("Uploaded.floppy.yaml", text(disk));
    const sessions::ReplyMessage r = openFile(f, up, Way::Drop, /*upload=*/true);
    if (!r.done) std::fprintf(stderr, "  open refused: %s\n", r.reason.c_str());
    CHECK(r.done);
    CHECK(replyOf(r) && replyOf(r)->detail == "floppy");
    CHECK(f.held("floppy") != nullptr);
    bool imported = false;
    std::error_code ec;
    for (const auto& e : fs::directory_iterator(f.saveFolder(), ec))
        imported = imported || e.path().generic_string().find(".floppy.yaml") != std::string::npos;
    CHECK(imported);
}

TEST(lib13_ac19_module_one_place_plugged_pc1500_two_places_refused_pc1600) {
    SKIP_UNLESS(sessionfixture::romsPresent(), "no ROMs in roms/");
    SessionFixture f("calcu-opening-test");
    CHECK(f.start("PC-1500A"));
    const sessions::ReplyMessage one = openFile(f, onDisk(fs::path(kCards) / "ce161.card.yaml"), Way::Drop);
    CHECK(one.done);
    CHECK(replyOf(one) && replyOf(one)->as == opening::OpenedAs::Medium);
    const calcu::media::Binding* held = f.held(replyOf(one) ? replyOf(one)->detail : std::string());
    CHECK(held && held->peripheral.find("CE-161") != std::string::npos);

    CHECK(f.model("PC-1600"));
    (void)f.notices();
    const sessions::ReplyMessage two = openFile(f, onDisk(fs::path(kCards) / "ce1600m.card.yaml"), Way::Drop);
    CHECK(!two.done);
    CHECK(contains(f.notices(), "fits more than one place"));
}

TEST(lib13_ac19_tape_definition_into_cassette_wav_is_never_a_tape) {
    SKIP_UNLESS(sessionfixture::romsPresent(), "no ROMs in roms/");
    SessionFixture f("calcu-opening-test");
    CHECK(f.start("PC-1500A"));
    CHECK(f.attach("CE-150"));
    const sessions::ReplyMessage r = openFile(f, onDisk(fs::path(kCards) / "empty.tape.yaml"), Way::Drop);
    CHECK(r.done);
    CHECK(replyOf(r) && replyOf(r)->detail == "tape");
    CHECK(f.held("tape") != nullptr);
    // A WAV is a program file (docs/formats/media.md §3).
    const Bytes wav = fileBytes("tests/fixtures/tape/pc1500_tape.wav");
    if (!wav.empty()) {
        const sessions::ReplyMessage w = openFile(f, named("pc1500_tape.wav", wav), Way::Drop);
        CHECK(w.done && replyOf(w) && replyOf(w)->as == opening::OpenedAs::FileKind);
    }
}

// ---- T16-4: limits (AC-20) ----

TEST(lib13_ac20_over_1mib_refused_drop_openwith_start_not_dialog_harness) {
    Built b;
    Bytes big = text("10 PRINT 1\n");
    big.resize(shared::kMaxOpenBytes + 1, ' ');
    for (Way w : {Way::Drop, Way::OpenWith, Way::Start}) {
        const opening::Classified c = opening::classify(named("big.bas", big), w, b.f.part().openPart().setup());
        CHECK(c.what == opening::Classified::What::Refused);
        CHECK(c.refusal.find("big.bas is larger than 1 MiB") != std::string::npos);
    }
    for (Way w : {Way::Dialog, Way::Harness}) CHECK_EQ(takenAs(b.f, named("big.bas", big), w), std::string("basic-program"));
    Bytes exact = big;
    exact.resize(shared::kMaxOpenBytes);
    CHECK_EQ(takenAs(b.f, named("ok.bas", exact), Way::Drop), std::string("basic-program"));
}

// ---- T16-5: the ways in (AC-21, EC-7; startFile's case is the library's,
// library/opening/tests/start_file_tests.cpp) ----

TEST(lib13_ec7_open_by_any_way_keeps_dialog_folders) {
    SKIP_UNLESS(sessionfixture::romsPresent(), "no ROMs in roms/");
    SessionFixture f("calcu-opening-test");
    CHECK(f.start("PC-1500A"));
    using shared::OpenFolder;
    using shared::openFolderKind;
    calcu::settings::rememberOpenFolder(f.settings.file(), openFolderKind(OpenFolder::Samples), "/before/presets");
    calcu::settings::rememberOpenFolder(f.settings.file(), openFolderKind(OpenFolder::Basic), "/before/basic");
    calcu::settings::rememberOpenFolder(f.settings.file(), openFolderKind(OpenFolder::Assembly), "/before/code");
    const fs::path dir = f.root / "files";
    fs::create_directories(dir);
    std::ofstream(dir / "p.bas", std::ios::binary) << "10 PRINT 1\n";
    for (Way w : {Way::Drop, Way::OpenWith, Way::Start, Way::Dialog}) {
        CHECK(openFile(f, onDisk(dir / "p.bas"), w).done);
        CHECK(openFile(f, onDisk(fs::path(kCards) / "ce161.card.yaml"), w).done);
    }
    CHECK_EQ(f.settings.file().text("folders", openFolderKind(OpenFolder::Samples)), std::string("/before/presets"));
    CHECK_EQ(f.settings.file().text("folders", openFolderKind(OpenFolder::Basic)), std::string("/before/basic"));
    CHECK_EQ(f.settings.file().text("folders", openFolderKind(OpenFolder::Assembly)), std::string("/before/code"));
}

// ---- T16-6: today's tests of telling a dropped file (AC-22) ----

TEST(lib13_ac22_drop_targets) {
    Built b;
    auto& f = b.f;
    // Presets, by their top-level `model:` line (the library's test 5).
    CHECK_EQ(takenAs(f, text("# Hanoi\nmodel: PC-1600\nkeys:\n  - key: mode\n")), std::string("preset"));
    CHECK_EQ(takenAs(f, text("model: PC-1500:A04\n")), std::string("preset"));
    CHECK_EQ(takenAs(f, text("\xEF\xBB\xBFmodel: PC-1500\n")), std::string("preset"));   // BOM
    CHECK_EQ(takenAs(f, text("# \xE2\x80\x93" "empty\xE2\x80\x93\r\nmodel: PC-1600\r\n")), std::string("preset"));
    CHECK(takenAs(f, text("keys:\n  model: PC-1600\n")) != "preset");    // indented
    CHECK(takenAs(f, text("# model: PC-1600\nkeys:\n")) != "preset");    // commented
    CHECK(takenAs(f, text("plotter: CE-150\n")) != "preset");            // no model
    Bytes withNul = text("model: PC-1600\n");
    withNul.push_back(0x00);
    CHECK(takenAs(f, withNul) != "preset");

    // BASIC: listings and tokenized files.
    CHECK_EQ(takenAs(f, text("10 PRINT \"HI\"\n20 END\n")), std::string("basic-program"));
    CHECK_EQ(takenAs(f, ce158(basicPayload(), 0x40)), std::string("basic-program"));
    CHECK_EQ(takenAs(f, pc1600(basicPayload(), 0x21)), std::string("basic-program"));
    for (const char* path : {"tests/fixtures/basic/lissajou-1500.bas", "tests/fixtures/basic/lissajou-1600.bbin"}) {
        const Bytes bytes = fileBytes(path);
        if (!bytes.empty()) CHECK_EQ(takenAs(f, bytes), std::string("basic-program"));
    }

    // Machine code: headered, or headerless that looks like code in a .bin
    // (today also .rom, which goes: AC-17).
    CHECK_EQ(takenAs(f, ce158(kCode, 0x42, 0x40C5)), std::string("machine-code-program"));
    CHECK_EQ(takenAs(f, pc1600(kCode, 0x10, 0xC0C5)), std::string("machine-code-program"));
    CHECK_EQ(takenAs(f, z80Code()), std::string("machine-code-program"));
    CHECK_EQ(takenAs(f, z80Code(), "CODE.ROM"), std::string("refused"));   // changed by AC-17
    CHECK_EQ(takenAs(f, z80Code(), "code.jpg"), std::string("refused"));
    CHECK_EQ(takenAs(f, z80Code(), "code"), std::string("refused"));
    CHECK_EQ(takenAs(f, pc1600(kCode, 0x10, 0xC0C5), "x.dat"), std::string("machine-code-program"));
    CHECK_EQ(takenAs(f, text("model: PC-1600\n"), "x.bin"), std::string("preset"));

    // Everything else is refused (today: ignored without a word).
    CHECK_EQ(takenAs(f, Bytes{}), std::string("refused"));
    CHECK_EQ(takenAs(f, kCode), std::string("refused"));
    CHECK_EQ(takenAs(f, pseudoRandom(4096, 7)), std::string("refused"));
    CHECK_EQ(takenAs(f, text("Just some notes,\nnot a program.\n")), std::string("refused"));

    // A cassette WAV goes to the BASIC loader, also one without a readable file.
    for (const char* wav : {"tests/fixtures/tape/pc1500_tape.wav", "tests/fixtures/tape/pc1600_tape.wav"}) {
        const Bytes bytes = fileBytes(wav);
        if (!bytes.empty()) CHECK_EQ(takenAs(f, bytes, "tape.wav"), std::string("basic-program"));
    }
    CHECK_EQ(takenAs(f, silentWav(), "x.wav"), std::string("basic-program"));
}

} // namespace
