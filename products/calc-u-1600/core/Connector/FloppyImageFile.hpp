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
// Specification: docs/formats/media.md §2 at the repository root (other tools, e.g.
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
constexpr long kFloppyFormatVersion = 2;

struct FloppyFile {
    std::string diskName;
    std::string description;  // `description:`, one line for a tooltip; "" = none
    std::string insertNotice; // `insert-notice:`, shown when the template is inserted; "" = none
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

}  // namespace floppy_detail

// What the header of a disk's file must be.
inline calcu::media::HeaderRules floppyHeaderRules() {
    return {floppy_detail::kFormatTag, "floppy disk", kFloppyFormatVersion, "disk-name"};
}

// Serializes `image` (must be CE1600FCard::kImageSize bytes) as a complete
// version-2 file. `diskName` is written double-quoted verbatim (the YAML
// reader does no escape processing), so it must not contain '"' or a newline.
// `asTemplate` writes `template: true` (a preset's `saveas: template
// floppy:...`); the app itself only ever writes instances. A non-empty
// `description` is written too (single-quoted if it holds a '"'). `saved`
// is the time of the save, ISO 8601 UTC (the session's host time).
inline std::string formatFloppyFile(const std::string& diskName, const std::vector<uint8_t>& image,
                                    bool asTemplate, const std::string& description,
                                    const std::string& saved, const std::string& insertNotice = {}) {
    std::string out;
    out.reserve(4096);
    calcu::media::Header header{"Calc-U-1600 CE-1600F floppy disk image", floppy_detail::kFormatTag,
                                kFloppyFormatVersion, "disk-name", diskName, description, asTemplate, saved};
    header.insertNotice = insertNotice;
    out += calcu::media::headerText(header);
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
    calcu::media::Header header;
    if (!calcu::media::readHeader(root, floppyHeaderRules(), &header, error)) return false;
    if (!root.requireOnlyKeys({"format", "format-version", "disk-name", "description", "insert-notice", "template", "saved", "sides"}, error))
        return false;
    out->diskName = header.name;
    out->description = header.description;
    out->insertNotice = header.insertNotice;
    out->isTemplate = header.isTemplate;
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
