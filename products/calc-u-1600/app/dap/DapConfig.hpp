#pragma once
#include <cstdint>
#include <string>

#include "Debug/AttachKeys.hpp"
#include "Debug/SourceMap.hpp"
#include "app/dap/DapJson.hpp"
#include "presets/Reader.hpp"

namespace calcu::presets {
class PresetBundle;
}

// The configuration of a DAP session: what the client sends with `attach`
// or `launch`, over the `debug:` block of the preset `project` names,
// checked against core/Debug/AttachKeys.hpp, on calcu::sessions::Value.

namespace cu1600::dap {

// The configuration in force: `args` alone, or with `project` that preset's
// `debug:` block as the base: the keys of `args` win, `program` is merged key
// by key, `listings` and `symbols` are added to the block's, and `project`
// is the `preset` unless `args` gives one. False with `error` set when the
// project cannot be read.
bool effectiveConfig(const Value& args, const calcu::presets::ReadSetup& read, const calcu::presets::PresetBundle* bundle,
                     Value* out, std::string* error);

// The keys of `table` in `o` (a configuration, a `debug:` block, a
// `program`, a `c1600/load`): `Bad "<key>": <what>`. Keys the
// table does not know pass: a launch configuration carries the IDE's own.
bool checkAttach(const Value& o, const debug::attach::Table& table, std::string* error);

// A number by the preset's rule: a JSON number, or text where
// `&`, `0x` or `$` is hexadecimal and a bare number decimal; 0..max.
bool numberOf(const Value& v, std::uint32_t max, std::uint32_t* out);
// The qualifier `bank`, `me`, `pu`, `pv` of `o` (-1 for a missing one).
debug::BankKey bankKeyOf(const Value& o);

} // namespace cu1600::dap
