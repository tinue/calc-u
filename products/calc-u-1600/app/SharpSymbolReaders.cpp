#include "app/SharpSymbolReaders.hpp"

#include <string>
#include <string_view>

#include "Debug/Listing/Listing.hpp"

namespace shared {

namespace {

enum class Format { Sdas, Zasm, Symbols };

std::vector<std::string> linesOf(const std::vector<std::uint8_t>& bytes) {
    std::vector<std::string> out;
    std::string line;
    for (std::uint8_t b : bytes) {
        if (b == '\n') {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            out.push_back(line);
            line.clear();
        } else {
            line += static_cast<char>(b);
        }
    }
    if (!line.empty()) out.push_back(line);
    return out;
}

bool hasSymbolTable(const std::vector<std::string>& text) {
    for (const std::string& raw : text) {
        const std::string t = debug::normalizeSourceText(raw);
        if (t == ".SYMBOLS:" || t == ".symbols:") return true;
    }
    return false;
}

class ListingSymbols final : public calcu::debugger::SymbolReader {
public:
    explicit ListingSymbols(Format f) : format_(f) {}

    bool reads(std::string_view, const std::vector<std::uint8_t>& bytes) const override {
        const std::vector<std::string> text = linesOf(bytes);
        switch (format_) {
        case Format::Symbols: return hasSymbolTable(text);
        case Format::Sdas: return !hasSymbolTable(text) && debug::detectSdas(text);
        case Format::Zasm: return !hasSymbolTable(text) && !debug::detectSdas(text) && debug::detectZasm(text);
        }
        return false;
    }

    std::optional<calcu::debugger::SymbolTable> read(const std::vector<std::uint8_t>& bytes, std::string* error) const override {
        // The file's lines as a file of their own; the sources it names are
        // read from disk if they are there (they only place lines, which a
        // table of symbols does not need).
        static const std::string kPath = "/calcu-symbols-input";
        const std::vector<std::string> text = linesOf(bytes);
        const debug::SourceReader disk = debug::diskReader();
        const debug::SourceReader reader = [&text, &disk](const std::string& path, std::vector<std::string>* lines) {
            if (path == kPath) {
                *lines = text;
                return true;
            }
            return disk(path, lines);
        };
        debug::Listing listing;
        std::string why;
        const bool ok = format_ == Format::Symbols ? debug::loadSymbolFile(kPath, &listing, &why, reader)
                                                   : debug::loadListing(kPath, &listing, &why, {}, reader);
        if (!ok) {
            if (error) *error = why.empty() ? std::string("The file has no symbols.") : why;
            return std::nullopt;
        }
        calcu::debugger::SymbolTable table;
        for (const auto& [name, value] : listing.symbols) table.add(name, value);
        return table;
    }

private:
    Format format_;
};

} // namespace

std::vector<const calcu::debugger::SymbolReader*> sharpSymbolReaders() {
    static const ListingSymbols kSymbols(Format::Symbols), kSdas(Format::Sdas), kZasm(Format::Zasm);
    return {&kSymbols, &kSdas, &kZasm};
}

} // namespace shared
