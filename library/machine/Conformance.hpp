#pragma once
#include "machine/Catalogue.hpp"
#include "machine/Machine.hpp"
#include <string>
#include <vector>

// A running machine against its descriptor (LIB-1 design §3.10), for the
// product tests of LIB-1 AC-33 and AC-40. It reads only what the machine
// reports at run time: variant(), the shape of readDisplay(), that
// keyDown/keyUp take every descriptor key and refuse a made-up one (every
// key it presses it releases again), the places in plugged() and the fit of
// each peripheral, and debugTargets(). The media place check is LIB-7's
// (D-1). Returns one line per difference; empty: conforms.

namespace calcu::machine {

std::vector<std::string> checkAgainstDescriptor(Machine&, const Catalogue&);

} // namespace calcu::machine
