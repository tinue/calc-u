#pragma once

#include <string>

#ifdef __APPLE__
// Turns off AppKit's window restoration for this app (registers
// ApplePersistenceIgnoreState = YES). Call before QApplication exists.
//
// The app restores nothing itself, and AppKit's own "reopen windows?"
// prompt -- shown after a run was killed -- was fatal in 0.8.1: it arrived
// with the launch's open-application event while the startup default
// preset's synchronous load pumped events (user input excluded), and each
// waited for the other. The start now runs in parts, but the app
// still restores nothing, so the prompt stays off.
void macDisableWindowRestoration();

// Turns off the press-and-hold accent picker for this app (registers
// ApplePressAndHoldEnabled = NO). The main window takes part in input-method
// composition (so dead keys compose), which would otherwise let a held
// letter open the picker instead of simply being held. Call before
// QApplication exists.
void macDisablePressAndHold();

// The app's Application Support folder (inside the sandbox container for a
// sandboxed app): the base of the settings file on macOS.
// Empty if macOS gives none.
std::string macApplicationSupportFolder();

// Keeps App Nap from stretching the timers of the process while its window
// is not in front, for the whole run: the emulation keeps real-time pace in
// the background, as Calc-U-59's Swift app does. The --pacing-probe relies
// on it too.
void macKeepTimersPrecise();
#endif
