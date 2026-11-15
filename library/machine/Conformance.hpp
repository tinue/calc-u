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

// LIB-10 (design §2): the machine's debug targets against its list. One
// target per listed ID, in the descriptor's order; each info() well formed
// (registers and cells of 1 to 64 bits, a code space that exists, a
// `-written` view named after a space, bank windows inside their space,
// qualifier fields with values); reading every register and the first and
// last cell of every space changes no tick count, no register and no other
// cell. A machine without debugging() and without listed targets conforms.
std::vector<std::string> checkDebugging(Machine&);

} // namespace calcu::machine
