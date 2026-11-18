#pragma once
#include "machine/Ids.hpp"
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Symbols of an assembler the product supports: a product names its readers
// (Calc-U-1600's listings); the library's tests use a table of their own
// (TestDebugParts.hpp). Loaded by the command `symbols <file> [<target>]`, a name then stands wherever an
// address of that target is taken, and `history` and `disassemble` show
// `name` or `name+offset`.

namespace calcu::debugger {

class SymbolTable {
public:
    void add(std::string name, std::uint64_t address);
    std::optional<std::uint64_t> address(std::string_view name) const;
    // "name", "name+0x3" for the nearest name at or below the address, or none.
    std::optional<std::string> nameAt(std::uint64_t address) const;
    std::size_t size() const { return byName_.size(); }
    bool empty() const { return byName_.empty(); }

private:
    std::map<std::string, std::uint64_t, std::less<>> byName_;
    std::map<std::uint64_t, std::string> byAddress_;
};

class SymbolReader {
public:
    virtual ~SymbolReader() = default;
    // Whether the file is of this reader's format (by its name and content).
    virtual bool reads(std::string_view fileName, const std::vector<std::uint8_t>& bytes) const = 0;
    // The table, or none with `error`.
    virtual std::optional<SymbolTable> read(const std::vector<std::uint8_t>& bytes, std::string* error) const = 0;
};

} // namespace calcu::debugger
