// Headless C++ tests for HostDirectoryDrive -- the file-level half of the
// PC-1600 host-directory drive (S3: / Y:). Each test works on a fresh
// temporary directory. Same no-framework, assert-and-tally style as
// lh5801_tests.cpp -- see that file's header comment.
//
// Build & run: see tools/run_tests.sh

#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "Files.hpp"
#include "TempFolder.hpp"
#include "../core/Connector/HostDirectoryDrive.hpp"
#include "../core/Connector/PC1600HostDriveCard.hpp"
#include "../core/PC1600/PC1600BusDrive.hpp"
#include "TestHarness.hpp"

namespace {

namespace fs = std::filesystem;
using Drive = HostDirectoryDrive;

using TempDir = calcu::testing::TempFolder;

Drive::Request request(uint8_t fn, const std::string& name83, uint16_t fcbAddress = 0xF3C7) {
    Drive::Request req;
    req.function = fn;
    req.fcbAddress = fcbAddress;
    req.device = 0x42;
    std::memcpy(&req.fcb[Drive::kFcbName], name83.data(), 11);
    return req;
}

uint32_t size32(const std::array<uint8_t, Drive::kFcbImageSize>& fcb) {
    return fcb[0x25] | (fcb[0x26] << 8) | (fcb[0x27] << 16) | (static_cast<uint32_t>(fcb[0x28]) << 24);
}

std::vector<std::string> listNames(Drive& drive, const std::string& pattern) {
    std::vector<std::string> names;
    auto req = request(Drive::kSearchFirst, pattern);
    auto r = drive.execute(req);
    while (r.status == 0) {
        names.emplace_back(reinterpret_cast<const char*>(r.payload.data()), 11);
        req.function = Drive::kSearchNext;
        req.fcb = r.fcb;
        r = drive.execute(req);
    }
    CHECK(r.status == 0x01);  // end of listing: "not found", what FILES expects
    return names;
}

TEST(test_name_mapping) {
    CHECK(Drive::toFcbName("prog.bas") == "PROG    BAS");
    CHECK(Drive::toFcbName("MAKEFILE") == "MAKEFILE   ");
    CHECK(Drive::toFcbName("A-B_{1}.$$$") == "A-B_{1} $$$");
    CHECK(Drive::toFcbName("toolongname.bas").empty());
    CHECK(Drive::toFcbName("x.basic").empty());
    CHECK(Drive::toFcbName(".hidden").empty());
    CHECK(Drive::toFcbName("a.b.c").empty());
    CHECK(Drive::toFcbName("sp ace.txt").empty());
    CHECK(Drive::toFcbName("wild?.txt").empty());
    CHECK(Drive::toFcbName("caf\xC3\xA9.txt").empty());
    CHECK(Drive::toHostName("PROG    BAS") == "PROG.BAS");
    CHECK(Drive::toHostName("MAKEFILE   ") == "MAKEFILE");
}

TEST(test_timestamp_counts_years_from_1980) {
    std::tm tm{};
    tm.tm_year = 2026 - 1900;
    tm.tm_mon = 8;   // September
    tm.tm_mday = 29;
    tm.tm_hour = 14;
    tm.tm_min = 35;
    tm.tm_sec = 42;
    tm.tm_isdst = -1;
    uint16_t time = 0, date = 0;
    Drive::packTimestamp(std::mktime(&tm), time, date);
    CHECK(time == ((14 << 11) | (35 << 5) | 21));
    CHECK(date == ((46 << 9) | (9 << 5) | 29));
    tm.tm_year = 1975 - 1900;  // before FAT's 1980: year field 0
    Drive::packTimestamp(std::mktime(&tm), time, date);
    CHECK((date >> 9) == 0);
}

TEST(test_no_directory_is_no_media) {
    Drive drive;
    auto r = drive.execute(request(Drive::kOpen, "PROG    BAS"));
    CHECK(r.status == 0x20);
    CHECK(r.erl == Drive::kErlNoMedia);
    drive.setDirectory("/nonexistent/calcu-hostdrive");
    r = drive.execute(request(Drive::kSearchFirst, "???????????"));
    CHECK(r.status == 0x20);
}

TEST(test_listing_filters_and_sorts) {
    TempDir dir;
    calcu::testing::writeText(dir.path() / "zeta.txt", "z");
    calcu::testing::writeText(dir.path() / "ALPHA.BAS", "a");
    calcu::testing::writeText(dir.path() / "a-very-long-name.bas", "x");
    calcu::testing::writeText(dir.path() / ".DS_Store", "x");
    fs::create_directories(dir.path() / "SUBDIR");
    Drive drive;
    drive.setDirectory(dir.path());
    const auto names = listNames(drive, "???????????");
    CHECK(names.size() == 2);
    CHECK(names.size() == 2 && names[0] == "ALPHA   BAS");
    CHECK(names.size() == 2 && names[1] == "ZETA    TXT");
    const auto bas = listNames(drive, "????????BAS");
    CHECK(bas.size() == 1);
}

TEST(test_directory_entry_fields) {
    TempDir dir;
    calcu::testing::writeText(dir.path() / "DATA.TXT", std::string(70000, 'x'));
    Drive drive;
    drive.setDirectory(dir.path());
    auto r = drive.execute(request(Drive::kSearchFirst, "DATA    TXT"));
    CHECK(r.status == 0);
    CHECK(r.payload.size() == Drive::kDirEntrySize);
    if (r.payload.size() == Drive::kDirEntrySize) {
        CHECK(std::memcmp(r.payload.data(), "DATA    TXT", 11) == 0);
        CHECK(r.payload[0x0B] == Drive::kAttrArchive);
        std::tm now{};
        const std::time_t t = std::time(nullptr);
        localtime_r(&t, &now);
        CHECK((r.payload[0x19] >> 1) == now.tm_year + 1900 - 1980);  // the file's real year
        const uint32_t size = r.payload[0x1C] | (r.payload[0x1D] << 8) | (r.payload[0x1E] << 16);
        CHECK(size == 70000);
    }
    CHECK(r.fcb[Drive::kFcbDirIndex] != 0xFF);
    auto next = request(Drive::kSearchNext, "DATA    TXT");
    next.fcb = r.fcb;
    r = drive.execute(next);
    CHECK(r.status == 0x01);
    CHECK(r.fcb[Drive::kFcbDirIndex] == 0xFF);
    next.fcb = r.fcb;
    r = drive.execute(next);
    CHECK(r.erl == Drive::kErlEof);  // SNEXT after the end, like FDSNEXT
}

TEST(test_case_collision_keeps_one_file) {
    TempDir dir;
    calcu::testing::writeText(dir.path() / "Prog.bas", "lower");
    calcu::testing::writeText(dir.path() / "PROG.BAS", "upper");
    std::error_code ec;
    const bool caseSensitive = fs::file_size(dir.path() / "Prog.bas", ec) == 5 && calcu::testing::readText(dir.path() / "PROG.BAS") == "upper" &&
                               calcu::testing::readText(dir.path() / "Prog.bas") == "lower";
    Drive drive;
    drive.setDirectory(dir.path());
    const auto names = listNames(drive, "???????????");
    CHECK(names.size() == 1);
    auto r = drive.execute(request(Drive::kOpen, "PROG    BAS"));
    CHECK(r.status == 0);
    CHECK(size32(r.fcb) == 5);
    if (caseSensitive) {
        auto rd = request(Drive::kSeqRead, "PROG    BAS");
        rd.fcb = r.fcb;
        r = drive.execute(rd);
        CHECK(r.status == 0 && std::memcmp(r.payload.data(), "upper", 5) == 0);
    }
}

TEST(test_open_fills_the_fcb_like_the_floppy) {
    TempDir dir;
    calcu::testing::writeText(dir.path() / "prog.bas", std::string(300, 'p'));
    Drive drive;
    drive.setDirectory(dir.path());
    auto req = request(Drive::kOpen, "PROG    BAS");
    req.fcb[Drive::kFcbCount] = 0x55;
    req.fcb[Drive::kFcbRecord] = 0x12;
    auto r = drive.execute(req);
    CHECK(r.status == 0);
    CHECK(r.fcb[Drive::kFcbCount] == 0);
    CHECK(r.fcb[Drive::kFcbAttr] == Drive::kAttrArchive);
    CHECK(size32(r.fcb) == 300);
    CHECK(r.fcb[Drive::kFcbRecLen] == 0x00 && r.fcb[Drive::kFcbRecLen + 1] == 0x01);
    CHECK(r.fcb[Drive::kFcbRecord] == 0 && r.fcb[Drive::kFcbRecord + 1] == 0);
    CHECK(std::memcmp(&r.fcb[Drive::kFcbName], "PROG    BAS", 11) == 0);  // left untouched
    r = drive.execute(request(Drive::kOpen, "MISSING BAS"));
    CHECK(r.status == 0x01);
    CHECK(r.erl == Drive::kErlNotFound);
}

// BASIC's own sequence: full records with FCB+06H = 0, the last partial
// one flushed as a whole record, then CLOSE with FCB+06H = its byte count.
std::array<uint8_t, Drive::kFcbImageSize> writeRecord(Drive& drive, const std::string& name,
                                                     std::array<uint8_t, Drive::kFcbImageSize> fcb,
                                                     uint8_t fill) {
    auto wr = request(Drive::kSeqWrite, name);
    wr.fcb = fcb;
    wr.fcb[Drive::kFcbCount] = 0;
    for (size_t i = 0; i < Drive::kRecordSize; ++i) wr.data[i] = static_cast<uint8_t>(fill + i);
    const auto r = drive.execute(wr);
    CHECK(r.status == 0);
    return r.fcb;
}

Drive::Response closeWithCount(Drive& drive, const std::string& name,
                               std::array<uint8_t, Drive::kFcbImageSize> fcb, uint8_t count) {
    auto cl = request(Drive::kClose, name);
    cl.fcb = fcb;
    cl.fcb[Drive::kFcbCount] = count;
    return drive.execute(cl);
}

TEST(test_write_then_read_records_beyond_32k) {
    TempDir dir;
    Drive drive;
    drive.setDirectory(dir.path());
    auto r = drive.execute(request(Drive::kCreate, "BIG     DAT"));
    CHECK(r.status == 0);
    CHECK(fs::exists(dir.path() / "BIG.DAT"));
    // 200 full records (51200 bytes, past the 7-bit record field) + 10 bytes.
    std::array<uint8_t, Drive::kFcbImageSize> fcb = r.fcb;
    for (int rec = 0; rec <= 200; ++rec) fcb = writeRecord(drive, "BIG     DAT", fcb, static_cast<uint8_t>(rec));
    CHECK(fs::file_size(dir.path() / "BIG.DAT") == 201 * 256);
    CHECK((fcb[Drive::kFcbModified] & 0x03) == 0x03);
    r = closeWithCount(drive, "BIG     DAT", fcb, 10);
    CHECK(r.status == 0);
    CHECK(fs::file_size(dir.path() / "BIG.DAT") == 200 * 256 + 10);
    CHECK(size32(r.fcb) == 200 * 256 + 10);
    CHECK(r.fcb[Drive::kFcbModified] == 0);

    r = drive.execute(request(Drive::kOpen, "BIG     DAT"));
    fcb = r.fcb;
    bool allMatch = true;
    for (int rec = 0; rec <= 200; ++rec) {
        auto rd = request(Drive::kSeqRead, "BIG     DAT");
        rd.fcb = fcb;
        r = drive.execute(rd);
        if (r.status != 0 || r.payload.size() != Drive::kRecordSize) { allMatch = false; break; }
        const size_t n = rec == 200 ? 10 : Drive::kRecordSize;
        for (size_t i = 0; i < n; ++i)
            if (r.payload[i] != static_cast<uint8_t>(rec + i)) allMatch = false;
        fcb = r.fcb;
    }
    CHECK(allMatch);
    // Record 201 = the 7-bit field wrapped once: 201 = 0x49 + 1 * 128.
    CHECK(fcb[Drive::kFcbRecord] == 0x49 && fcb[Drive::kFcbRecord + 1] == 1);
    auto rd = request(Drive::kSeqRead, "BIG     DAT");
    rd.fcb = fcb;
    r = drive.execute(rd);
    CHECK(r.status == 0x04);  // bit 2: end of file, what FREADREC and COPY test
    CHECK(r.erl == Drive::kErlEof);
}

TEST(test_close_without_writes_keeps_the_file) {
    TempDir dir;
    calcu::testing::writeText(dir.path() / "R.TXT", std::string(300, 'r'));
    Drive drive;
    drive.setDirectory(dir.path());
    auto r = drive.execute(request(Drive::kOpen, "R       TXT"));
    r = closeWithCount(drive, "R       TXT", r.fcb, 5);  // read-only FCB: no trim
    CHECK(r.status == 0);
    CHECK(fs::file_size(dir.path() / "R.TXT") == 300);
}

TEST(test_append_rewrite_truncates_after_the_last_record) {
    TempDir dir;
    calcu::testing::writeText(dir.path() / "LOG.TXT", std::string("abc\x1A", 4));
    Drive drive;
    drive.setDirectory(dir.path());
    auto r = drive.execute(request(Drive::kOpen, "LOG     TXT"));
    // BASIC's APPENDPOS rewrites record 0 with the old bytes minus the 1AH.
    auto wr = request(Drive::kSeqWrite, "LOG     TXT");
    wr.fcb = r.fcb;
    std::memcpy(wr.data.data(), "abd", 3);
    r = drive.execute(wr);
    CHECK(r.status == 0);
    r = closeWithCount(drive, "LOG     TXT", r.fcb, 3);
    CHECK(calcu::testing::readText(dir.path() / "LOG.TXT") == "abd");
}

TEST(test_create_existing_file_keeps_its_host_spelling) {
    TempDir dir;
    calcu::testing::writeText(dir.path() / "notes.txt", "old contents");
    Drive drive;
    drive.setDirectory(dir.path());
    auto r = drive.execute(request(Drive::kCreate, "NOTES   TXT"));
    CHECK(r.status == 0);
    CHECK(size32(r.fcb) == 0);
    CHECK(fs::file_size(dir.path() / "notes.txt") == 0);
    int count = 0;
    for (auto& e : fs::directory_iterator(dir.path())) { (void)e; ++count; }
    CHECK(count == 1);
    r = drive.execute(request(Drive::kCreate, "BAD?    TXT"));
    CHECK(r.erl == Drive::kErlWildcard);
}

TEST(test_protect_attribute_round_trips_through_permissions) {
    TempDir dir;
    calcu::testing::writeText(dir.path() / "KEEP.BAS", "k");
    calcu::testing::writeText(dir.path() / "DROP.BAS", "d");
    Drive drive;
    drive.setDirectory(dir.path());
    auto set = request(Drive::kSetAttr, "KEEP    BAS", 0xF400);
    set.fcb[Drive::kFcbAttr] = Drive::kAttrArchive | Drive::kAttrProtected;
    CHECK(drive.execute(set).status == 0);
    auto r = drive.execute(request(Drive::kOpen, "KEEP    BAS"));
    CHECK(r.fcb[Drive::kFcbAttr] & Drive::kAttrProtected);

    // Protected: CREATE and WRITE refused; DELETE of *.BAS keeps it, removes the rest.
    CHECK(drive.execute(request(Drive::kCreate, "KEEP    BAS")).erl == Drive::kErlProtected);
    r = drive.execute(request(Drive::kDelete, "????????BAS"));
    CHECK(r.erl == Drive::kErlProtected);
    CHECK(r.status == 0x48);
    CHECK(fs::exists(dir.path() / "KEEP.BAS"));
    CHECK(!fs::exists(dir.path() / "DROP.BAS"));

    set.fcb[Drive::kFcbAttr] = Drive::kAttrArchive;
    CHECK(drive.execute(set).status == 0);
    CHECK(drive.execute(request(Drive::kDelete, "KEEP    BAS")).status == 0);
    CHECK(!fs::exists(dir.path() / "KEEP.BAS"));
    CHECK(drive.execute(request(Drive::kDelete, "KEEP    BAS")).erl == Drive::kErlNotFound);
    auto wild = request(Drive::kSetAttr, "????????BAS");
    CHECK(drive.execute(wild).erl == Drive::kErlWildcard);
}

TEST(test_rename) {
    TempDir dir;
    calcu::testing::writeText(dir.path() / "old.bas", "1");
    calcu::testing::writeText(dir.path() / "TAKEN.BAS", "2");
    Drive drive;
    drive.setDirectory(dir.path());
    auto req = request(Drive::kRename, "OLD     BAS");
    std::memcpy(&req.fcb[Drive::kFcbNewName], "TAKEN   BAS", 11);
    CHECK(drive.execute(req).erl == Drive::kErlExists);
    std::memcpy(&req.fcb[Drive::kFcbNewName], "NEW     ???", 11);  // '?' keeps the old character
    CHECK(drive.execute(req).status == 0);
    CHECK(fs::exists(dir.path() / "NEW.BAS"));
    CHECK(!fs::exists(dir.path() / "old.bas"));
    CHECK(calcu::testing::readText(dir.path() / "NEW.BAS") == "1");
    CHECK(drive.execute(req).erl == Drive::kErlNotFound);

    // Its own name and a folder's are taken too (FDRENAME finds the file itself).
    fs::create_directories(dir.path() / "FOLDER");
    auto same = request(Drive::kRename, "NEW     BAS");
    std::memcpy(&same.fcb[Drive::kFcbNewName], "NEW     BAS", 11);
    CHECK(drive.execute(same).erl == Drive::kErlExists);
    std::memcpy(&same.fcb[Drive::kFcbNewName], "FOLDER     ", 11);
    CHECK(drive.execute(same).erl == Drive::kErlExists);
    CHECK(fs::exists(dir.path() / "NEW.BAS"));
}

TEST(test_get_alloc_fits_dskf) {
    TempDir dir;
    Drive drive;
    drive.setDirectory(dir.path());
    auto r = drive.execute(request(Drive::kGetAlloc, "           "));
    CHECK(r.status == 0);
    CHECK(r.bc == 512);
    CHECK((r.de & 0xFF) == 64);
    CHECK(static_cast<uint32_t>(r.bc) * (r.de & 0xFF) <= 0xFFFF);  // DSKF's 16-bit shift
    CHECK(r.hl > 0);
}

TEST(test_get_length_in_records) {
    TempDir dir;
    calcu::testing::writeText(dir.path() / "L.DAT", std::string(513, 'l'));
    Drive drive;
    drive.setDirectory(dir.path());
    auto r = drive.execute(request(Drive::kGetLength, "L       DAT"));
    CHECK(r.status == 0);
    CHECK(r.de == 0 && r.hl == 3);
}

TEST(test_reset_ends_searches) {
    TempDir dir;
    calcu::testing::writeText(dir.path() / "A.TXT", "a");
    calcu::testing::writeText(dir.path() / "B.TXT", "b");
    Drive drive;
    drive.setDirectory(dir.path());
    auto r = drive.execute(request(Drive::kSearchFirst, "???????????"));
    CHECK(r.status == 0);
    drive.reset(Drive::kResetNew);
    auto next = request(Drive::kSearchNext, "???????????");
    next.fcb = r.fcb;
    CHECK(drive.execute(next).erl == Drive::kErlEof);
}

std::string promptOf(const Drive::Response& r) {
    const std::string p(r.payload.begin(), r.payload.end());
    return p.substr(0, p.find('\r'));
}

TEST(test_change_directory_paths) {
    TempDir dir;
    fs::create_directories(dir.path() / "Dev" / "asm");
    fs::create_directories(dir.path() / "GAMES");
    fs::create_directories(dir.path() / "a-very-long-folder");
    fs::create_directories(dir.path() / ".hidden");
    Drive drive;
    CHECK(drive.changeDirectory("/").erl == Drive::kErlNoMedia);
    drive.setDirectory(dir.path());

    auto r = drive.changeDirectory(".");
    CHECK(r.status == 0 && promptOf(r) == "S3:>");
    CHECK(r.payload.size() == Drive::kPromptSize);
    CHECK(r.payload[4] == 0x0D);
    r = drive.changeDirectory("dev");  // relative, any case
    CHECK(r.status == 0 && promptOf(r) == "S3:/DEV>");
    r = drive.changeDirectory("../GAMES");
    CHECK(r.status == 0 && promptOf(r) == "S3:/GAMES>");
    r = drive.changeDirectory("/DEV/ASM/");  // absolute, trailing slash (FILEX)
    CHECK(r.status == 0 && promptOf(r) == "S3:/DEV/ASM>");
    CHECK(drive.currentDirectory() == (std::vector<std::string>{"Dev", "asm"}));
    r = drive.changeDirectory("..");
    CHECK(r.status == 0 && promptOf(r) == "S3:/DEV>");
    r = drive.changeDirectory("\\DEV\\ASM");  // backslash separates too
    CHECK(r.status == 0 && promptOf(r) == "S3:/DEV/ASM>");
    r = drive.changeDirectory("../..");
    CHECK(r.status == 0 && promptOf(r) == "S3:>");
    r = drive.changeDirectory("");
    CHECK(r.status == 0 && promptOf(r) == "S3:>");

    // Above the root is an error, as on the MEP: a program goes up until it fails.
    for (const char* up : {"..", "/..", "/DEV/../.."}) {
        r = drive.changeDirectory(up);
        CHECK(r.status == 0x01 && r.erl == Drive::kErlNotFound);
    }
    CHECK(drive.changeDirectory("DEV/ASM").status == 0);
    CHECK(drive.changeDirectory("../../..").erl == Drive::kErlNotFound);
    CHECK(drive.currentDirectory() == (std::vector<std::string>{"Dev", "asm"}));
    CHECK(drive.changeDirectory("/").status == 0);

    // Only visible 8.3 folders: a failure leaves the directory unchanged.
    CHECK(drive.changeDirectory("GAMES").status == 0);
    for (const char* bad : {"NOPE", "/a-very-long-folder", "/.hidden", "/GAMES/X"}) {
        r = drive.changeDirectory(bad);
        CHECK(r.status == 0x01);
        CHECK(r.erl == Drive::kErlNotFound);
        CHECK(r.payload.empty());
    }
    CHECK(drive.currentDirectory() == std::vector<std::string>{"GAMES"});
}

TEST(test_change_directory_skips_symlinks) {
    TempDir dir, outside;
    fs::create_directories(dir.path() / "IN");
    std::error_code ec;
    fs::create_directory_symlink(outside.path(), dir.path() / "OUT", ec);
    if (ec) return;  // no symlinks here
    Drive drive;
    drive.setDirectory(dir.path());
    CHECK(drive.changeDirectory("OUT").erl == Drive::kErlNotFound);
    CHECK(drive.changeDirectory("IN").status == 0);
}

TEST(test_long_prompt_keeps_the_end) {
    TempDir dir;
    fs::create_directories(dir.path() / "AAAAAAAA" / "BBBBBBBB" / "CCCCCCCC" / "DDDDDDDD");
    fs::create_directories(dir.path() / "AAAAAAAA" / "BBBBBBBB" / "CCC");
    Drive drive;
    drive.setDirectory(dir.path());
    auto r = drive.changeDirectory("/AAAAAAAA/BBBBBBBB/CCC");  // 26 characters: fits
    CHECK(r.status == 0 && promptOf(r) == "S3:/AAAAAAAA/BBBBBBBB/CCC>");
    r = drive.changeDirectory("/AAAAAAAA/BBBBBBBB/CCCCCCCC/DDDDDDDD");
    CHECK(r.status == 0);
    // The MEP's form: "...", the last 22 characters of the path, no drive.
    CHECK(promptOf(r) == "...BBBB/CCCCCCCC/DDDDDDDD>");
}

TEST(test_files_live_in_the_current_directory) {
    TempDir dir;
    calcu::testing::writeText(dir.path() / "ROOT.TXT", "r");
    fs::create_directories(dir.path() / "SUB");
    calcu::testing::writeText(dir.path() / "SUB" / "IN.TXT", "abc");
    Drive drive;
    drive.setDirectory(dir.path());
    CHECK(drive.changeDirectory("SUB").status == 0);
    CHECK(listNames(drive, "???????????") == std::vector<std::string>{"IN      TXT"});
    CHECK(drive.execute(request(Drive::kOpen, "IN      TXT")).status == 0);
    CHECK(drive.execute(request(Drive::kOpen, "ROOT    TXT")).erl == Drive::kErlNotFound);
    CHECK(drive.execute(request(Drive::kCreate, "NEW     TXT")).status == 0);
    CHECK(fs::exists(dir.path() / "SUB" / "NEW.TXT"));
    auto ren = request(Drive::kRename, "NEW     TXT");
    std::memcpy(&ren.fcb[Drive::kFcbNewName], "OLD     TXT", 11);
    CHECK(drive.execute(ren).status == 0);
    CHECK(fs::exists(dir.path() / "SUB" / "OLD.TXT"));
    CHECK(drive.execute(request(Drive::kDelete, "OLD     TXT")).status == 0);
    CHECK(!fs::exists(dir.path() / "SUB" / "OLD.TXT"));
    CHECK(fs::exists(dir.path() / "ROOT.TXT"));
    // A file may not take a folder's name.
    fs::create_directories(dir.path() / "SUB" / "INNER");
    CHECK(drive.execute(request(Drive::kCreate, "INNER      ")).erl == Drive::kErlExists);
}

TEST(test_vanished_directory_fails_until_cdir) {
    TempDir dir;
    fs::create_directories(dir.path() / "GONE");
    Drive drive;
    drive.setDirectory(dir.path());
    CHECK(drive.changeDirectory("GONE").status == 0);
    fs::remove_all(dir.path() / "GONE");
    auto r = drive.execute(request(Drive::kCreate, "X       TXT"));
    CHECK(r.erl == Drive::kErlNotFound);
    CHECK(!fs::exists(dir.path() / "X.TXT"));  // nothing lands in the root
    CHECK(drive.changeDirectory("/").status == 0);
    CHECK(drive.execute(request(Drive::kCreate, "X       TXT")).status == 0);
}

TEST(test_directory_mode_lists_subdirectories) {
    TempDir dir;
    calcu::testing::writeText(dir.path() / "A.TXT", "a");
    fs::create_directories(dir.path() / "zeta");
    fs::create_directories(dir.path() / "ALPHA.V1");
    fs::create_directories(dir.path() / "a-very-long-folder");
    Drive drive;
    drive.setDirectory(dir.path());
    drive.setListDirectories(true);
    auto r = drive.execute(request(Drive::kSearchFirst, "???????????"));
    CHECK(r.payload.size() == Drive::kDirEntrySize);
    CHECK(r.payload[0x0B] == Drive::kAttrDirectory);
    CHECK(r.payload[0x1C] == 0 && r.payload[0x1D] == 0);
    // Without a SEARCH NEXT, directory mode stays.
    CHECK(listNames(drive, "???????????") == (std::vector<std::string>{"ALPHA   V1 ", "ZETA       "}));
    drive.setListDirectories(true);
    CHECK(listNames(drive, "Z??????????") == std::vector<std::string>{"ZETA       "});
    drive.setListDirectories(false);
    CHECK(listNames(drive, "???????????") == std::vector<std::string>{"A       TXT"});
}

// As on the MEP: after a SEARCH NEXT, the next other call ends directory
// mode, so MEP software that never calls FILEMODE gets files again.
TEST(test_directory_mode_ends_after_search_next) {
    TempDir dir;
    calcu::testing::writeText(dir.path() / "A.TXT", "a");
    fs::create_directories(dir.path() / "D1");
    fs::create_directories(dir.path() / "D2");
    Drive drive;
    drive.setDirectory(dir.path());
    drive.setListDirectories(true);
    CHECK(listNames(drive, "???????????") == (std::vector<std::string>{"D1         ", "D2         "}));
    CHECK(listNames(drive, "???????????") == std::vector<std::string>{"A       TXT"});

    drive.setListDirectories(true);
    CHECK(listNames(drive, "???????????").size() == 2);
    CHECK(drive.execute(request(Drive::kOpen, "A       TXT")).status == 0);  // a file call ends it
    CHECK(listNames(drive, "???????????") == std::vector<std::string>{"A       TXT"});

    drive.setListDirectories(true);
    CHECK(listNames(drive, "???????????").size() == 2);
    CHECK(drive.changeDirectory("D1").status == 0);  // CDIR ends it too
    CHECK(listNames(drive, "???????????").empty());
}

TEST(test_reset_functions) {
    TempDir dir;
    fs::create_directories(dir.path() / "SUB");
    Drive drive;
    drive.setDirectory(dir.path());
    for (uint8_t fn = 0; fn < 8; ++fn) {
        CHECK(drive.changeDirectory("/SUB").status == 0);
        drive.setListDirectories(true);
        drive.reset(fn);
        const bool toRoot = fn == Drive::kResetPowerOn || fn == Drive::kResetResume || fn == Drive::kResetReset;
        CHECK(drive.currentDirectory().empty() == toRoot);
        CHECK(listNames(drive, "???????????").empty());  // back in file mode: SUB has no files
    }
    CHECK(drive.changeDirectory("/SUB").status == 0);
    drive.setDirectory(dir.path());  // a new mount starts at the root
    CHECK(drive.currentDirectory().empty());
}

// The MEP fixed entries' bus protocol, as the driver ROM speaks it.
TEST(test_card_mep_commands) {
    TempDir dir;
    calcu::testing::writeText(dir.path() / "A.TXT", "a");
    fs::create_directories(dir.path() / "DOCS");
    PC1600HostDriveCard card;
    card.drive().setDirectory(dir.path());
    auto out = [&card](uint8_t port, uint8_t value) {
        CHECK(card.respondsToWrite(PC1600BusDrive::z80IoPins(port, /*forWrite=*/true), value));
    };
    auto in = [&card]() {
        uint8_t v = 0;
        CHECK(card.respondsToRead(PC1600BusDrive::z80IoPins(PC1600HostDriveCard::kDataPort, /*forWrite=*/false), v));
        return v;
    };
    auto cdir = [&](const std::string& path, uint8_t& status, uint8_t& erl, std::string& prompt) {
        out(PC1600HostDriveCard::kCommandPort, PC1600HostDriveCard::kChangeDirCommand);
        out(PC1600HostDriveCard::kDataPort, static_cast<uint8_t>(path.size()));
        for (char c : path) out(PC1600HostDriveCard::kDataPort, static_cast<uint8_t>(c));
        status = in();
        erl = in();
        prompt.clear();
        if (status == 0)
            for (size_t i = 0; i < Drive::kPromptSize; ++i) prompt += static_cast<char>(in());
    };
    uint8_t status = 0xFF, erl = 0xFF;
    std::string prompt;
    cdir("/DOCS", status, erl, prompt);
    CHECK(status == 0 && erl == 0);
    CHECK(prompt.substr(0, 11) == "S3:/DOCS>\r ");
    cdir("NOPE", status, erl, prompt);
    CHECK(status == 0x01 && erl == Drive::kErlNotFound);
    CHECK(in() == 0xFF);  // no prompt after an error
    cdir("..", status, erl, prompt);
    CHECK(status == 0 && prompt.rfind("S3:>\r", 0) == 0);

    out(PC1600HostDriveCard::kCommandPort, PC1600HostDriveCard::kDirModeCommand);
    CHECK(listNames(card.drive(), "???????????") == std::vector<std::string>{"DOCS       "});
    out(PC1600HostDriveCard::kCommandPort, PC1600HostDriveCard::kFileModeCommand);
    CHECK(listNames(card.drive(), "???????????") == std::vector<std::string>{"A       TXT"});

    // Module reset: the function code follows. NEW keeps the directory,
    // power on goes back to the root.
    cdir("DOCS", status, erl, prompt);
    out(PC1600HostDriveCard::kCommandPort, PC1600HostDriveCard::kResetCommand);
    out(PC1600HostDriveCard::kDataPort, Drive::kResetNew);
    CHECK(card.drive().currentDirectory() == std::vector<std::string>{"DOCS"});
    out(PC1600HostDriveCard::kCommandPort, PC1600HostDriveCard::kResetCommand);
    out(PC1600HostDriveCard::kDataPort, Drive::kResetPowerOn);
    CHECK(card.drive().currentDirectory().empty());
}

TEST(test_unsupported_function) {
    TempDir dir;
    Drive drive;
    drive.setDirectory(dir.path());
    auto r = drive.execute(request(0x21, "           "));
    CHECK(r.status == 0x10);
    CHECK(r.erl == Drive::kErlUnsupported);
}

}  // namespace
