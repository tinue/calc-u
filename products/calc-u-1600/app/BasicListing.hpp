#pragma once
#include <string>
#include <vector>

#include "Machine/SharpProductKinds.hpp"

namespace cu1600 {

// `c1600 list-basic`: the program as listing lines, the detokenizer's,
// with the keyword table of the model. On the PC-1600 the program area
// TITLE selects, read across its module banks and internal RAM as the
// loader lays it down. Refused (`problem`) in MODE 1.
struct BasicListingReply {
    std::vector<std::string> lines;
    std::string problem;
};

BasicListingReply basicListing(const sharp::LoaderMemory&);

} // namespace cu1600
