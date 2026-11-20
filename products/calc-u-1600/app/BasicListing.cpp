#include "app/BasicListing.hpp"

#include <cstdint>

#include "Basic/BasicPointers.hpp"
#include "Basic/BasicProgramSource.hpp"
#include "PC1600/PC1600BasicLoader.hpp"

namespace cu1600 {

BasicListingReply basicListing(const sharp::LoaderMemory& mem) {
    BasicListingReply r;
    if (mem.mode() == sharp::LoaderMemory::Mode::Mode1) {
        r.problem = "list-basic reads a program in MODE 0 only.";
        return r;
    }
    std::vector<std::uint8_t> records;
    if (mem.pc1600()) {
        if (!pc1600ReadBasicRecords(mem, records, r.problem)) return r;
    } else {
        auto pk = [&](std::uint32_t a) { return mem.peek(static_cast<std::uint16_t>(a)); };
        const std::uint32_t start = sharp::readBE16(mem, sharp::kPC1500BasicPointers.programStart);
        const std::uint32_t end = sharp::readBE16(mem, sharp::kPC1500BasicPointers.programEnd);
        // [line hi][line lo][len][content], up to the FFH end mark.
        for (std::uint32_t a = start; a < end && pk(a) != 0xFF;) {
            const std::uint32_t size = 3u + pk(a + 2);
            for (std::uint32_t i = 0; i < size; ++i) records.push_back(pk(a + i));
            a += size;
        }
    }
    if (records.empty()) return r;   // no program: no lines
    const basic::BasicListing l =
        basic::detokenize(records, mem.pc1600() ? basic::TransferModel::PC1600 : basic::TransferModel::PC1500);
    if (!l.ok) {
        r.problem = "The program can't be read: " + l.error;
        return r;
    }
    r.lines = l.lines;
    return r;
}

} // namespace cu1600
