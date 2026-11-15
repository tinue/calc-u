#pragma once
#include "app/MachineModel.hpp"
#include "pacing/PacingProfile.hpp"

namespace shared {

// The pacing profile of a Sharp model (LIB-17 AC-20; design D-8): the slice
// is the host's call, no jitter, and full speed is never paced -- as 0.8.1's
// pacer (Pacer.cpp:21-61). The same for the PC-1500, PC-1500A and PC-1600;
// handed to the machine session with each new machine.
calcu::pacing::PacingProfile pacingProfile(Model model);

} // namespace shared
