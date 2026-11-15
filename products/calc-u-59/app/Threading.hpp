#pragma once

// Who runs the machine of an EmulatorSession (LIB-3 59 design §2; LIB-17
// design §5). There are no threads in the app layer: both run on the one
// thread of the session's host.
// - Cooperative: the Web frontend's SessionController::tick() pumps the
//   machine itself through CooperativePacing's own batches (2.3.0's pacer,
//   until M-34, when the Web moves onto the library session), and the tests
//   of SessionController.
// - Hosted: the library's machine session runs the machine with LIB-17's
//   pacer (cu59::TIAppPart); CooperativePacing keeps only its job queue and
//   its run flag.
// Goes in M-34 with the last cooperative user (LIB-15 59).

namespace cu59 {

enum class Threading { Cooperative, Hosted };

} // namespace cu59
