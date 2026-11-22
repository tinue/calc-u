#pragma once

#include <string>

// The Qt6 component set's macOS host settings, which both products' apps
// apply at their start.
#ifdef __APPLE__
namespace calcu::qt6 {

// Turns off AppKit's window restoration for this app (registers
// ApplePersistenceIgnoreState = YES). Call before QApplication exists.
//
// The app restores nothing itself, so AppKit's own "reopen windows?"
// prompt -- shown after a run was killed -- has nothing to offer and stays
// off.
void macDisableWindowRestoration();

// Turns off the press-and-hold accent picker for this app (registers
// ApplePressAndHoldEnabled = NO). The main window takes part in input-method
// composition (so dead keys compose), which would otherwise let a held
// letter open the picker instead of simply being held. Call before
// QApplication exists.
void macDisablePressAndHold();

// Keeps App Nap from stretching the timers of the process while its window
// is not in front, for the whole run: the emulation keeps real-time pace in
// the background. The pacing probes rely on it too.
void macKeepTimersPrecise();

// The app's Application Support folder (inside the sandbox container for a
// sandboxed app): the base of the settings file on macOS.
// Empty if macOS gives none.
std::string macApplicationSupportFolder();

} // namespace calcu::qt6
#endif
