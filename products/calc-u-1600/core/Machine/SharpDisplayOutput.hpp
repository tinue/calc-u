#pragma once
#include <cstdint>
#include <vector>

#include "../Display/StatusLine.hpp"
#include "machine/Descriptor.hpp"
#include "machine/Display.hpp"

// The dot matrix and status line of a Sharp display as display output.
// The indicator names are the descriptor's, from kStatusSymbolNames (core/Display/StatusLine.hpp). A
// display that is off shows nothing: no dot, no indicator.

namespace sharp {

calcu::machine::DisplayOutput displayOutput(const calcu::machine::MachineDescriptor&, bool on, int width,
                                            int height, std::vector<std::uint8_t> dots, const StatusLine&);

// The status line an output's indicators give (the indicators the model
// lacks stay off), for the LCD text readers.
StatusLine statusLineOf(const calcu::machine::DisplayOutput&);

} // namespace sharp
