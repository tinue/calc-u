#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "formats/Yaml.hpp"
#include "media/Header.hpp"
#include "BatteryCardInstance.hpp"   // formatAddressedHexLines
#include "CE1600FCard.hpp"
#include "MemoryCardDefinition.hpp"  // mcd_detail::parseAddressedHex

// ── CE-1600F floppy-disk file (`<name>.floppy.yaml`) ───────────────────
//
// Specification: docs/developer/Floppy-Image-Format.md (other tools, e.g.
// SharpDataExchange, implement that document -- keep the two in step).
//
// A saved CE-1600F diskette: an explicit `disk-name` (what the GUI picker
// shows and a preset's `floppy:` key refers to -- the counterpart of a
// card's `module-name`) plus both 64 KB sides as `addressed-hex` blocks
// (help/10-module-definition-format.md §6), so an unused side or track
// run collapses to one line:
//
//   format: ce1600f-floppy
//   format-version: 2
//   disk-name: "Progs"
//   saved: 2026-09-19T12:34:56Z
//   sides:
//     0:
//       encoding: addressed-hex
//       bytes: |
//         $0000: ...
//     1:
//       ...
//
// `format-version` is mandatory; a reader rejects any version it doesn't
// know rather than guessing. The file is always written whole (no splice /
// comment preservation, unlike BatteryCardInstance.hpp's card instances).

constexpr const char* kFloppyFileSuffix = ".floppy.yaml";
constexpr long kFloppyFormatVersion = 2;   // version 1 (0.8.x) named the sides a and b (outdated, preset.md §9.4)

struct FloppyFile {
    std::string diskName;
    std::string description;  // `description:`, one line for a tooltip; "" = none
    // `template: true`: a read-only starting disk the app never writes to,
    // wherever the file lives; Name & Save copies it into an instance
    // (formatFloppyFile() writes the key only when asked to). No key = an instance,
    // autosaved in place.
    bool isTemplate = false;
    std::vector<uint8_t> image;  // CE1600FCard::kImageSize bytes, side 0 then side 1
};

namespace floppy_detail {

constexpr const char* kFormatTag = "ce1600f-floppy";
constexpr const char* kSideKeys[2] = {"0", "1"};

// Checks `format`/`format-version` and reads `disk-name`, `description`
// and `template` from a parsed file.
inline bool readHeader(const calcu::formats::YamlNode& root, std::string* diskName, std::string* description, bool* isTemplate,
                       std::string* error) {
    if (!root.isMap()) {
        *error = "not a floppy-disk file (expected a YAML mapping)";
        return false;
    }
    std::string format;
    const calcu::formats::YamlNode* formatNode = root.find("format");
    if (!formatNode || !formatNode->asString(&format, error) || format != kFormatTag) {
        *error = std::string("not a floppy-disk file (expected 'format: ") + kFormatTag + "')";
        return false;
    }
    if (!calcu::formats::requireFormatVersion(root, kFloppyFormatVersion, "floppy", error)) return false;
    if (!root.requireOnlyKeys({"format", "format-version", "disk-name", "description", "template", "saved", "sides"}, error))
        return false;
    const calcu::formats::YamlNode* nameNode = root.find("disk-name");
    if (!nameNode) {
        *error = "missing 'disk-name'";
        return false;
    }
    if (!nameNode->asString(diskName, error)) return false;
    if (diskName->empty()) {
        *error = "empty 'disk-name'";
        return false;
    }
    description->clear();
    if (const calcu::formats::YamlNode* descriptionNode = root.find("description")) {
        if (!descriptionNode->asString(description, error)) return false;
    }
    *isTemplate = false;
    if (const calcu::formats::YamlNode* templateNode = root.find("template")) {
        if (!templateNode->asBool(isTemplate, error)) return false;
    }
    return true;
}

}  // namespace floppy_detail

// Serializes `image` (must be CE1600FCard::kImageSize bytes) as a complete
// version-2 file. `diskName` is written double-quoted verbatim (the YAML
// reader does no escape processing), so it must not contain '"' or a newline.
// `asTemplate` writes `template: true` (a preset's `saveas: template
// floppy:...`); the app itself only ever writes instances. A non-empty
// `description` is written too (single-quoted if it holds a '"'). `saved`
// is the time of the save, ISO 8601 UTC (the session's host time).
inline std::string formatFloppyFile(const std::string& diskName, const std::vector<uint8_t>& image,
                                    bool asTemplate, const std::string& description,
                                    const std::string& saved) {
    std::string out;
    out.reserve(4096);
    out += calcu::media::headerText({"Calc-U-1600 CE-1600F floppy disk image", floppy_detail::kFormatTag,
                                     kFloppyFormatVersion, "disk-name", diskName, description, asTemplate, saved});
    out += "sides:\n";
    for (size_t side = 0; side < 2; ++side) {
        const auto first = image.begin() + static_cast<std::ptrdiff_t>(side * CE1600FCard::kSideSize);
        const std::vector<uint8_t> bytes(first, first + static_cast<std::ptrdiff_t>(CE1600FCard::kSideSize));
        out += std::string("  ") + floppy_detail::kSideKeys[side] + ":\n";
        out += "    encoding: addressed-hex\n";
        out += "    bytes: |\n";
        for (const auto& line : formatAddressedHexLines(bytes)) {
            out += "      ";
            out += line;
            out += '\n';
        }
    }
    return out;
}

inline bool parseFloppyFile(const std::string& text, FloppyFile* out, std::string* error) {
    calcu::formats::YamlNode root;
    if (!calcu::formats::parseYaml(text, &root, error)) return false;
    if (!floppy_detail::readHeader(root, &out->diskName, &out->description, &out->isTemplate, error)) return false;
    const calcu::formats::YamlNode* sides = root.find("sides");
    if (!sides || !sides->isMap()) {
        *error = "missing 'sides' mapping";
        return false;
    }
    if (!sides->requireOnlyKeys({"0", "1"}, error)) return false;
    out->image.clear();
    out->image.reserve(CE1600FCard::kImageSize);
    for (const char* key : floppy_detail::kSideKeys) {
        const std::string where = std::string("side '") + key + "': ";
        const calcu::formats::YamlNode* side = sides->find(key);
        if (!side || !side->isMap()) {
            *error = where + "missing";
            return false;
        }
        if (!side->requireOnlyKeys({"encoding", "bytes"}, error)) return false;
        std::string encoding, bytesText;
        const calcu::formats::YamlNode* encNode = side->find("encoding");
        const calcu::formats::YamlNode* bytesNode = side->find("bytes");
        if (!encNode || !encNode->asString(&encoding, error) || encoding != "addressed-hex") {
            *error = where + "expected 'encoding: addressed-hex'";
            return false;
        }
        if (!bytesNode || !bytesNode->asString(&bytesText, error)) {
            *error = where + "missing 'bytes'";
            return false;
        }
        std::vector<uint8_t> bytes;
        if (!mcd_detail::parseAddressedHex(bytesText, static_cast<uint32_t>(CE1600FCard::kSideSize), &bytes,
                                           error)) {
            *error = where + *error;
            return false;
        }
        out->image.insert(out->image.end(), bytes.begin(), bytes.end());
    }
    return true;
}
