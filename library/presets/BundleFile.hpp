#pragma once
#include "machine/Catalogue.hpp"
#include "machine/Ids.hpp"
#include "presets/ProductRules.hpp"
#include "presets/Reader.hpp"
#include "presets/Source.hpp"
#include <string>
#include <variant>
#include <vector>

// The preset list: the product's bundle file `bundle.yaml` at the root of its
// bundled presets (docs/formats/bundle.md), read with the same YAML. The
// order of the file is the order of the list.

namespace calcu::presets {

inline constexpr const char* kBundleFile = "bundle.yaml";

struct BundleEntry {
    std::string preset;                // the path inside the bundle
    std::vector<std::string> files;    // bundle.md §2
    bool startup = false;
    std::string group;                 // examples only
    int line = 0;
};

// The entries in order, without reading a preset; or the first error.
std::variant<std::vector<BundleEntry>, ReadError> readBundleFile(const PresetBundle&);

struct PresetListEntry {
    std::string bundlePath;   // examples:<path>
    machine::ModelId model;
    std::string name, description;
    enum class Kind { Startup, Example } kind = Kind::Example;
    std::string group;        // examples only
    std::vector<std::string> files;
};

// The entries with only the header keys of each preset read; an entry
// whose preset is missing or whose header cannot be read is left out, and
// its sentence goes to `left`.
std::vector<PresetListEntry> presetList(const PresetBundle&, const ProductRules&, const machine::Catalogue&,
                                        std::vector<std::string>* left);

// bundle.md §3: one line per problem: a listed file that is missing, a
// preset listed twice or both as startup and example, a path that leaves the
// bundle, a file a listed preset names that is not listed with it (a file
// of a step that reads a file; checked by reading the preset).
std::vector<std::string> checkBundle(const PresetBundle&, const ReadSetup&);

} // namespace calcu::presets
