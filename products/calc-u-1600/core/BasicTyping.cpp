#include "BasicTyping.hpp"

#include <sstream>
#include <vector>

#include "Basic/BasicLineStoreCheck.hpp"
#include "Machine/SharpProductKinds.hpp"
#include "PC1600/PC1600ProgramPlacement.hpp"
#include "sessions/LibraryTopics.hpp"
#include "sessions/SessionPart.hpp"
#include "typing/Awaitable.hpp"
#include "typing/TypingSupport.hpp"

namespace sharp::loaders {

namespace {

using calcu::machine::Ticks;

std::uint16_t readBE16(const LoaderMemory& m, std::uint16_t a) {
    return static_cast<std::uint16_t>((m.peek(a) << 8) | m.peek(static_cast<std::uint16_t>(a + 1)));
}

// The selected program area as the typed line sees it: the start, the end
// mark and the end "key" that folds the PC-1600's bank index in, so a line
// that moves the end into another bank still counts as a change. PC-1500:
// BASPRG_ST / BASPRG_END ($7865 / $7867). PC-1600 S0: F865 / F867 (LH5803
// addresses) with the bank F02CH; S1 / S2: the slot descriptor.
struct Area {
    std::uint32_t start = 0, end = 0, endKey = 0;
};

Area areaOf(const LoaderMemory& m) {
    Area a;
    if (!m.pc1600()) {
        a.start = readBE16(m, 0x7865);
        a.end = readBE16(m, 0x7867);
        a.endKey = a.end;
        return a;
    }
    const int title = m.programArea();
    if (title == 1 || title == 2) {
        const pc1600::SlotDescriptor d = pc1600::readSlotDescriptor([&m](std::uint16_t at) { return m.peek(at); }, title);
        a.start = d.start;
        a.end = d.end;
        a.endKey = a.end | (static_cast<std::uint32_t>(d.endIndex) << 16);
    } else {
        a.start = pc1600::lh5803ToZ80(readBE16(m, 0xF865));
        a.end = pc1600::lh5803ToZ80(readBE16(m, 0xF867));
        a.endKey = a.end | (static_cast<std::uint32_t>(m.peek(0xF02C)) << 16);
    }
    return a;
}

int codePoints(const std::string& line) {
    int n = 0;
    for (char c : line)
        if ((static_cast<unsigned char>(c) & 0xC0) != 0x80) n++;
    return n;
}

std::vector<std::string> linesOf(const std::string& listing) {
    std::vector<std::string> out;
    std::istringstream in(listing);
    std::string line;
    while (std::getline(in, line)) {
        while (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) out.push_back(line);   // an empty line is left out
    }
    return out;
}

} // namespace

calcu::sessions::Task<BasicTypeResult> typeProgram(calcu::sessions::OperationContext& ctx,
                                                   calcu::typing::TypingEngine& engine,
                                                   calcu::machine::Machine& machine, std::string listing) {
    BasicTypeResult result;
    LoaderMemory* mem = calcu::machine::first<LoaderMemory>(machine);
    const calcu::typing::TypingProfile* profile = engine.profile();
    if (!mem || !profile || !profile->charKeys) {
        result.error = calcu::sessions::kNoMachine;
        co_return result;
    }
    const std::vector<std::string> lines = linesOf(listing);
    // D-6: every character of the listing has a key, or nothing is typed.
    for (const std::string& line : lines)
        for (char32_t c : calcu::typing::decodeUtf8(line))
            if (!profile->charKeys(c, nullptr)) {
                const std::string shown = c < 0x20 || c == 0x7F ? "U+" + std::to_string(static_cast<std::uint32_t>(c))
                                                                 : calcu::typing::encodeUtf8(c);
                result.error = "No key types the character \"" + shown + "\".";
                co_return result;
            }

    const calcu::machine::TickRate rate = machine.descriptor().tickRate;
    const Ticks frame = calcu::sessions::OperationContext::runPiece(rate);
    const Ticks promptBound = static_cast<Ticks>(rate) * 12;
    const Ticks stableBound = static_cast<Ticks>(rate) * 6;
    auto peek = [mem](std::uint32_t a) { return mem->peek(static_cast<std::uint16_t>(a)); };

    int typed = 0, stored = 0;
    for (const std::string& line : lines) {
        if (mem->pc1600() && codePoints(line) > kMaxTypedLineLength) {
            result.rejectedLines.push_back(line);   // not typed: the editor would cut it
            continue;
        }
        const Area before = areaOf(*mem);
        const basic::LineStoreCheck check = basic::LineStoreCheck::capture(peek, before.start, before.end, line, before.endKey);
        const calcu::typing::TypeResult t = co_await calcu::typing::typeTextAndWait(ctx, engine, line, /*enter=*/true);
        if (!t.done) {
            result.error = t.reason;
            co_return result;
        }
        typed++;
        // Prompt readiness, at most 12 s.
        auto* support = calcu::machine::first<calcu::typing::TypingSupport>(machine);
        for (Ticks spent = 0; spent < promptBound;) {
            if (!support || support->promptReady().value_or(true)) break;
            spent += co_await ctx.run(frame);
        }
        // Then the end of the program still for 30 frames, at most 6 s.
        std::uint32_t last = areaOf(*mem).endKey;
        int still = 0;
        for (Ticks spent = 0; still < 30 && spent < stableBound;) {
            spent += co_await ctx.run(frame);
            const std::uint32_t now = areaOf(*mem).endKey;
            if (now == last) still++;
            else {
                still = 0;
                last = now;
            }
        }
        if (check.changed(peek, areaOf(*mem).endKey)) stored++;
        else result.rejectedLines.push_back(line);
    }
    result.ok = result.rejectedLines.empty();
    if (!result.ok) {
        if (typed > 0 && stored == 0)
            result.error = "no program line was stored -- is the machine in PRO mode? "
                           "(a preset must `key: mode` into PRO before a program: block)";
        else
            result.error = std::to_string(result.rejectedLines.size()) +
                           " program line(s) not stored (too long, or rejected by the editor)";
    }
    co_return result;
}

} // namespace sharp::loaders
