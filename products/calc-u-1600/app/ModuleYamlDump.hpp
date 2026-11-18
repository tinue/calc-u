#pragma once
#include <string>
#include <vector>

namespace cu1600 {

class SharpAppPart;

// The INSPECTOR's Module YAML: the RAM of the module in each slot as an
// `initial-content:` block for a module definition, each under a header line
// naming the module, its slot and its banks. One chunk per line group; a
// single message when no slot holds a module with dumpable RAM.
std::vector<std::string> moduleYamlDump(SharpAppPart& app);

} // namespace cu1600
