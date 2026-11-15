#include "roms/Lookup.hpp"
#include <algorithm>

namespace calcu::roms {

// The only maker of a RomSet.
struct RomSetMaker {
    static std::shared_ptr<RomSet> make() { return std::shared_ptr<RomSet>(new RomSet()); }
    static void add(RomSet& s, const std::string& name, std::vector<std::uint8_t> bytes) {
        s.names_.push_back(name);
        s.files_.emplace(name, std::move(bytes));
    }
};

namespace {

const char* unitName(machine::ContentUnit u) {
    switch (u) {
    case machine::ContentUnit::Bytes: return "bytes";
    case machine::ContentUnit::Words: return "words";
    case machine::ContentUnit::Rows:  return "rows";
    }
    return "";
}

std::string amountText(std::uint64_t n, machine::ContentUnit u) { return std::to_string(n) + " " + unitName(u); }

} // namespace

LookupResult lookUp(const FullSet& full, const Bundle& bundle, const ProductRomReader* reader) {
    LookupResult result;
    result.missing.where = bundle.where();
    auto& problems = result.missing.files;
    auto set = RomSetMaker::make();

    for (const auto& f : full.files) {
        const auto& e = f.entry;
        auto problem = [&](Problem p, std::string detail) { problems.push_back({e.fileName, p, std::move(detail), f.neededBy}); };
        if (std::find(full.catalogueErrors.begin(), full.catalogueErrors.end(), e.fileName) != full.catalogueErrors.end()) {
            problem(Problem::Catalogue, "two ROM entries of this name differ");
            continue;
        }
        auto bytes = bundle.read(e.fileName);
        if (!bytes) {
            problem(Problem::Missing, {});
            continue;
        }
        if (e.encoding == machine::RomEncoding::Binary) {
            if (e.unit != machine::ContentUnit::Bytes || bytes->size() != e.decodedAmount) {
                problem(Problem::WrongAmount, amountText(bytes->size(), machine::ContentUnit::Bytes) + ", expected " +
                                                  amountText(e.decodedAmount, e.unit));
                continue;
            }
        } else {
            if (!reader) {
                problem(Problem::Unreadable, "no reader for a text ROM file");
                continue;
            }
            auto d = reader->decode(e, *bytes);
            if (!d.error.empty()) {
                problem(Problem::Unreadable, d.error);
                continue;
            }
            if (d.amount != e.decodedAmount) {
                problem(Problem::WrongAmount,
                        "yields " + amountText(d.amount, e.unit) + ", expected " + amountText(e.decodedAmount, e.unit));
                continue;
            }
        }
        RomSetMaker::add(*set, e.fileName, std::move(*bytes));
    }

    if (problems.empty() && reader) {
        problems = reader->checkSet(*set);
        for (auto& p : problems) {
            if (!p.neededBy.empty()) continue;
            for (const auto& f : full.files)
                if (f.entry.fileName == p.fileName) p.neededBy = f.neededBy;
        }
    }
    if (problems.empty()) result.set = std::move(set);
    return result;
}

std::string describe(const RomsMissing& m) {
    std::string s = m.files.size() == 1 ? "1 ROM file is missing or bad:\n"
                                         : std::to_string(m.files.size()) + " ROM files are missing or bad:\n";
    for (const auto& f : m.files) {
        s += "  " + f.fileName + ": ";
        switch (f.problem) {
        case Problem::Missing:      s += "missing"; break;
        case Problem::WrongAmount:  s += "wrong size"; break;
        case Problem::Unreadable:   s += "cannot be read"; break;
        case Problem::ProductCheck: s += "fails its check"; break;
        case Problem::Catalogue:    s += "listed twice in different forms"; break;
        }
        if (!f.detail.empty()) s += " (" + f.detail + ")";
        if (!f.neededBy.empty()) {
            s += "; needed by ";
            for (std::size_t i = 0; i < f.neededBy.size(); ++i) s += (i ? ", " : "") + describe(f.neededBy[i]);
        }
        s += "\n";
    }
    s += "Looked in: " + m.where + "\n";
    s += "The full set of ROM files is needed; no machine starts without it. In a local build, run the "
         "product's roms/fetch_roms.sh to fetch them, then build again.";
    return s;
}

} // namespace calcu::roms
