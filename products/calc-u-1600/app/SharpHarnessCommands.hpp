#pragma once
#include <functional>

#include "harness/Commands.hpp"

// Calc-U-1600's product commands under `pc1600` (LIB-11 AC-13; harness.md
// H-2): the loaders of C1600-2 (`file-kind`, `load-basic`, `type-basic`,
// `load-code`) and the commands of C1600-5 (design §3): `dump-basic`,
// `inspect`, `tape record|status`, `serial <port> feed|capture|link|report`,
// `host-drive`, `call`. Every handler reaches the machine through the
// product kinds of C1600-1 and the generic kinds, or through the session's
// commands; none names a machine class.

namespace shared {

class SharpAppPart;
class SerialScriptsPart;

// The parts of the run the commands work on; each nullptr when the session
// is not the harness's own run (harness attach to a frontend): the commands
// that need it are refused then.
struct SharpHarnessParts {
    std::function<SharpAppPart*()> app;
    std::function<SerialScriptsPart*()> serial;
};

void addSharpHarnessCommands(calcu::harness::CommandRegistry&, SharpHarnessParts);

} // namespace shared
