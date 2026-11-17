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

// The configuration of a DAP session (C1600-3 design §3.4 S1; AC-5, EC-2):
// what the client sends with `attach` or `launch`, over the `debug:` block
// of the preset `project` names (C1600-4 AC-11), checked against
// core/Debug/AttachKeys.hpp. From frontends/qt6/app/debug/DapSession.cpp of
// 0.8.1, on calcu::sessions::Value instead of QJsonObject.

namespace cu1600::dap {

// The configuration in force: `args` alone, or with `project` that preset's
// `debug:` block as the base: the keys of `args` win, `program` is merged key
// by key, `listings` and `symbols` are added to the block's, and `project`
// is the `preset` unless `args` gives one. False with `error` set when the
// project cannot be read.
bool effectiveConfig(const Value& args, const calcu::presets::ReadSetup& read, const calcu::presets::PresetBundle* bundle,
                     Value* out, std::string* error);

// The keys of `table` in `o` (a configuration, a `debug:` block, a
// `program`, a `c1600/load`): `Bad "<key>": <what>` (EC-2). Keys the
// table does not know pass: a launch configuration carries the IDE's own.
bool checkAttach(const Value& o, const debug::attach::Table& table, std::string* error);

// A number by the preset's rule (LIB-6 AC-1): a JSON number, or text where
// `&`, `0x` or `$` is hexadecimal and a bare number decimal; 0..max.
bool numberOf(const Value& v, std::uint32_t max, std::uint32_t* out);
// The qualifier `bank`, `me`, `pu`, `pv` of `o` (-1 for a missing one).
debug::BankKey bankKeyOf(const Value& o);

} // namespace cu1600::dap
