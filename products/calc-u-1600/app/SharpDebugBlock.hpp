#pragma once
#include <memory>
#include <string>
#include <variant>

#include "formats/Yaml.hpp"
#include "presets/Reader.hpp"
#include "presets/Vocabulary.hpp"

// ── A preset's `debug:` block (C1600-4 AC-11, AC-12, AC-14, AC-20, EC-4) ─
//
// The header key `debug:` of a project preset: the debugger's settings next
// to the machine the preset sets up (docs/formats/preset.md §3.4). The keys
// are the attach keys of core/Debug/AttachKeys.hpp in their camelCase; the
// block is checked when the preset is read (an error starts with `debug:`),
// its paths resolved from the preset's folder, and kept for the debugger. A
// load does nothing with it: the debugger asks for it (debugBlockOf).

namespace shared {

inline constexpr const char* kDebugKey = "debug";

struct DebugBlock final : calcu::presets::ProductValue {
    calcu::formats::YamlNode node;   // the block, every path resolved
};

// Checks the block; nullptr with the error given through the context.
std::shared_ptr<const DebugBlock> readDebugBlock(const calcu::formats::YamlNode&, int line,
                                                 calcu::presets::ReadContext&);

// The `debug:` block of the preset `ref` for the debugger (C1600-3; design
// §3.4): read by the library's reader with the product's vocabulary. The
// block (null when the preset has none), or the read error.
std::variant<std::shared_ptr<const DebugBlock>, std::string> debugBlockOf(const std::string& ref,
                                                                          const calcu::presets::ReadSetup&,
                                                                          const calcu::presets::PresetBundle* bundle);

} // namespace shared
