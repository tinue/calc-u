#pragma once
#include "machine/Catalogue.hpp"
#include "machine/Machine.hpp"
#include <string>
#include <vector>

// LIB-1 D-1, AC-40 (LIB-7 design §2, §3.3): a running machine's media places
// against its descriptors, for the product tests that call LIB-1's
// checkAgainstDescriptor: one capability of the kind MediaPlace per media
// place of the machine's descriptor and of each plugged peripheral's, owned
// by the machine or that peripheral, its id() that place; and none besides.
// One line per difference; empty: conforms. (In library/media/, not in
// machine::checkAgainstDescriptor: the machine area names no kind.)

namespace calcu::media {

std::vector<std::string> checkMediaPlaces(machine::Machine&, const machine::Catalogue&);

} // namespace calcu::media
