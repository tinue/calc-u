#include "ProgramLoader.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>

#include <variant>

#include "../Loaders.hpp"
#include "../Machine/SharpProductKinds.hpp"
#include "../MachineCodeFile.hpp"
#include "../PC1500/PC1500MachineCodeLoader.hpp"
#include "formats/Number.hpp"
#include "Listing/Listing.hpp"

namespace debug {

LoadResult loadProgram(calcu::machine::Machine& machine, const LoadRequest& req, SourceMap& map,
                       const BankMatch& match, const CodePeek& peek) {
    LoadResult r;
    sharp::LoaderMemory* memory = sharp::loaders::memoryOf(machine);
    if (!memory) {
        r.error = "No machine is running.";
        return r;
    }
    const bool pc1600 = memory->pc1600();
    std::ifstream in(req.bin, std::ios::binary);
    if (!in) {
        r.error = "can't read " + req.bin;
        return r;
    }
    const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    const machinecode::File file = machinecode::readFile(bytes);

    // The listing first: a headerless file without an "address" loads at the
    // listing's lowest address (the source's .org), so one launch
    // configuration serves any program.
    Listing listing;
    const bool haveListing = (!req.listing.empty() || !req.symbols.empty()) &&
                             loadListingWithSymbols(req.listing, req.source, req.symbols, &listing, &r.warnings);
    bool hasAddress = req.hasAddress;
    uint32_t address = req.address;
    if (!hasAddress && file.header == machinecode::File::Header::None && haveListing && !listing.lines.empty()) {
        address = listing.lines.front().addr;
        for (const ListingLine& l : listing.lines) address = std::min<uint32_t>(address, l.addr);
        hasAddress = true;
    }

    // Build & Load's rules: a header length mismatch still loads, the
    // request's address overrides, the CPU is the one the launch
    // configuration names (the toolchain knows it -- the one caller that
    // does), LH5803 code goes to the Z-80's 8000-FFFF, and the PC-1600
    // target follows the machine's MODE and TITLE like every other load.
    sharp::loaders::CodeRequest request;
    if (hasAddress) request.address = address;
    if (file.lengthMismatch) request.length = file.payload.size();
    if (pc1600) request.cpu = req.thread == 2 ? machinecode::Cpu::LH5803 : machinecode::Cpu::Z80;
    const sharp::loaders::CodePlan planned = sharp::loaders::planCode(*memory, bytes, request);
    if (planned.needsAddress()) {   // C1600-3 EC-3: C1600-2's refusal, with the hint
        r.error = sharp::loaders::refusal(req.bin, planned.refusal) + " Give \"address\", or a \"listing\" whose lowest "
                                                                     "line with bytes is where it goes.";
        return r;
    }
    if (!planned.holds()) {
        r.error = sharp::loaders::refusal(req.bin, planned.refusal);
        return r;
    }
    const auto written = sharp::loaders::loadCode(*memory, bytes, request);
    if (const std::string* refused = std::get_if<std::string>(&written)) {
        r.error = sharp::loaders::refusal(req.bin, *refused);
        return r;
    }
    const machinecode::LoadPlan& plan = planned.plan;
    const machinecode::PC1600State& state = planned.state;
    const uint32_t addr = plan.addr;
    r.lo = uint16_t(addr);
    r.hi = uint16_t(addr + plan.len - 1);
    // The entry: the request's (an address, or a symbol of the listing --
    // a name the listing defines wins over reading it as hex), else the
    // header's auto-run address, else the listing's ENTRY if it lies in the
    // loaded range, else the load address.
    const auto symbol = [&](const std::string& name, uint16_t* v) {
        const auto it = haveListing ? listing.symbols.find(name) : listing.symbols.end();
        if (it == listing.symbols.end()) return false;
        *v = it->second;
        return true;
    };
    uint16_t entry = 0;
    if (req.hasEntry) {
        r.entry = req.entry;
    } else if (!req.entrySymbol.empty()) {
        const auto parsed = calcu::formats::parseNumber(req.entrySymbol);
        if (symbol(req.entrySymbol, &entry)) r.entry = entry;
        else if (parsed && *parsed >= 0 && *parsed <= 0xFFFF) r.entry = uint16_t(*parsed);
        else {
            r.error = "\"entry\": " + req.entrySymbol + " is neither a symbol of the listing nor an address";
            return r;
        }
    } else if (file.autorunAddr) {
        r.entry = uint16_t(file.autorunAddr);
    } else if (symbol("ENTRY", &entry) && entry >= r.lo && entry <= r.hi) {
        r.entry = entry;
    } else {
        if (symbol("ENTRY", &entry))
            r.warnings.push_back("ENTRY is outside the loaded range, so the program starts at its load address");
        r.entry = r.lo;
    }

    // How BASIC starts it.
    uint32_t ramStart = 0, ramEnd = 0;
    if (!pc1600) pc1500UserRam(*memory, &ramStart, &ramEnd);
    // advice() starts at r.entry (the `entry` override, the header's
    // auto-run address, or the load address): CALL, with the plan's bank
    // (slot 2, or a header bank), for main-CPU code; XCALL (an LH5803
    // address) for the PC-1600's LH5803.
    const bool lh5803 = req.thread == 2;
    r.callCommand = machinecode::advice(planned.target, plan.slot, plan.bank, lh5803 ? plan.addr : plan.busAddr, plan.len, r.entry,
                                        ramStart, ramEnd, state,
                                        lh5803 ? machinecode::Cpu::LH5803 : machinecode::Cpu::Z80)
                        .callCommand;

    // The listing and symbols bind to the loaded range; symbols alone too.
    if (haveListing) {
        const std::string& name = req.listing.empty() ? req.symbols.front() : req.listing;
        r.binding = map.addLoaded(req.thread, std::move(listing), req.key, r.lo, r.hi, name);
        const int bad = map.verify(r.binding, match, peek);
        if (bad > 0)
            r.warnings.push_back(std::to_string(bad) + " listing lines don't match the loaded bytes -- is the listing from this build?");
    }
    r.ok = true;
    return r;
}

} // namespace debug
