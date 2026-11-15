# C59-5: QA report

Results per acceptance criterion of `spec.md`. Criteria not listed have
not been built yet.

## M-15 (2026-11-14): the XCFramework (AC-1 to AC-3)

Built on an M4 Pro, macOS 27.2, Xcode with the macOS 27.2 SDK; Homebrew
CMake and Ninja.

| AC | Result | How it was checked |
|---|---|---|
| AC-1 | Pass | `products/calc-u-59/frontends/swift/build_xcframework.sh` builds `build/xcframework-59/CalcU59.xcframework` from the root CMake project (`calcu59_core`, `calcu59_shared`, the library): slices `macos-arm64_x86_64`, `ios-arm64`, `ios-arm64_x86_64-simulator` (`lipo -archs`), headers under `calcu59/core/` and `calcu59/app/`. |
| AC-2 | Pass | The build is a step of the Xcode build: the aggregate target `CalcU59Core` runs the script and the app target depends on it, so it runs before Xcode processes the XCFramework (build log: `PhaseScriptExecution` before `ProcessXCFramework`). After `touch app/Bcd.cpp`, an `xcodebuild … build` of the app recompiled `Bcd.cpp` for the three slices, made the XCFramework again and relinked the app. Without CMake or Ninja the step stops the build with an `error:` that says the XCFramework cannot be built from the current sources. |
| AC-3 | Pass (build) | Minimum OS in the slices (`LC_BUILD_VERSION`): macOS 15.6, iOS 18.0, simulator 18.0; the Release Mac app is `x86_64 arm64` with `minos 15.6`; the app's deployment targets are unchanged (`IPHONEOS_DEPLOYMENT_TARGET = 18.0`, `MACOSX_DEPLOYMENT_TARGET = 15.6`). Built: macOS (Debug, and Release clean), iOS simulator, iOS device (`generic/platform=iOS`, unsigned). Not run on a device or an iOS 18 / macOS 15.6 system here: the app is not launched by an agent (`calc-u-59-swift` skill). |

Technical Requirements checked in the build: `CalcU59XcodeProjectCheck`
(Frequent, `frontends/swift/check_xcode_project.sh`): the project has no
folder or source of the library, the core or the app layer, links the
XCFramework, and the bridge includes only the XCFramework's headers. Xcode
compiles the five bridge `.mm` files and Swift, no `.cpp` (build logs of
all four builds above).

## M-33 (2026-11-15, branch `m33-sessions-59`): hosting, attach (AC-4 to AC-14, EC-1 to EC-4)

Built and run on an M4 Pro, macOS 27.2, Apple clang, CMake, Ninja
(RelWithDebInfo `build/tests`); the Swift app with Xcode (macOS 27.2 SDK),
the XCFramework in Debug; the UI test on the iPhone 18 Pro Max simulator
(iOS 27.2). The session thread is `products/calc-u-59/app/SessionThread.*`
(`cu59::SessionThread`, `HostedSession`, `AppSessionHost`; the Mailbox of
the design is its two queues), attach is `library/harness/attach/`
(`AttachEndpoint`, `AttachedHarness`, `attachPort`, `calcu attach` in
`AttachClient.*`), the bridge is `frontends/swift/Bridge/TIAppSession.*`.

| AC | Result | How it was checked |
|---|---|---|
| AC-1–3 | Pass (as M-15) | The Debug XCFramework built from the root project (slices as in M-15); the macOS Debug app built with Xcode; `CalcU59XcodeProjectCheck` passes. The Debug macOS and simulator slices now also carry `calcu59_harness` and attach (`build_xcframework.sh`) |
| AC-4 | Pass | `session_thread_tests.cpp`: `c59_5_ac4_about_60_calls_a_second` (60 calls in 1 s, three runs), `c59_5_ac4_session_built_called_destroyed_on_one_thread`, `c59_5_ac4_behind_more_than_100_ms_rebases`, `c59_5_ac4_ti59_on_the_session_thread`; `CalcU59NoOwnSession` (no tick, no `SessionController.hpp` in the bridge); the view model has no timer |
| AC-5 | Pass (thread); main-thread frame gaps not measured | `c59_5_ac5_post_returns_while_a_call_runs` (under 1 ms during a 200 ms call), `c59_5_ac5_command_during_slow_call_applied_after_it`. S5 (taps at full speed in the simulator) and the Instruments measurement of D-9 were not done in M-33 |
| AC-6 | Pass (C++) | `c59_5_ac6_posts_reach_the_session_in_order` (1,000 posts from two threads), `c59_5_ac6_messages_drained_in_order_one_wake_per_batch`. S1 (`MessageOrderTests.swift` in a Swift unit-test target) was not built: no unit-test target was added to the Xcode project |
| AC-7 | Built, not tested | `TIAppSession` states the services of AC-7 per platform when it listens (no audio sink). S2 (`HostServicesTests.swift`) not built, as AC-6 |
| AC-8 | Pass (C++, the session) | `c59_5_ac8_suspended_no_calls_commands_still_carried_out`, `c59_5_ac8_continue_calls_again_and_session_got_continue`; `session_tests.cpp` `c59_5_ac8_suspend_persists_and_stops_continue_runs` (the TI-58C's memory file written at suspend). The app sends suspend at `.background` (iOS inside a background task until the suspend was carried out) and continue at `.active`. S4 (Home in the simulator) not built |
| AC-9 | Not re-checked | The App Nap activity is unchanged; S6 (`tests/mac_attach_check.sh`, Checkpoint) not built |
| AC-10 | Pass | `c59_5_ac10_port_from_argument_else_59059`, `c59_5_ac10_bound_to_loopback_only`, `c59_5_ac10_calcu_attach_relays_lines`; by hand on the Mac: the Debug app with `--UITesting --attach-port 59171`, `calcu attach --port 59171` (`ticks`, `key-down 7`, `wait`, `lcd` shows 7, `wait 1s` 1.02 s); in the simulator: `AttachTests.testHarnessDrivesTheApp` (port 59160). The signed Debug app has `com.apple.security.network.server` (`codesign -d --entitlements`) |
| AC-11 | Pass | `c59_5_ac11_quit_ends_attach_not_host`, `c59_5_ac11_command_script_as_in_serve` (the same replies as `serve` for keys, `state`, a product command, refusals), `c59_5_ac11_options_that_do_not_apply_are_refused` (exit 2); the XCUITest's `quit` and a second attach |
| AC-12 | Pass | `c59_5_ac12_every_state_topic_once_on_attach`, `c59_5_ac12_reply_only_to_sender`, `c59_5_ac12_host_service_never_asked_of_harness`, `c59_5_ac12_frontend_keeps_receiving`; the XCUITest sees `busy` and `c59.model` on attach and the keys it sent in the window (screenshot attachment) |
| AC-13 | Pass | Release keeps `Calc-U-59.entitlements` and `ENABLE_INCOMING_NETWORK_CONNECTIONS = NO` (`CalcU59XcodeProjectCheck`). By hand: the Release macOS app (Xcode, unsigned) and the Release XCFramework slices (macos, ios, ios-simulator), and the Debug iOS device app and its slice, have no `AttachEndpoint` or `AttachedHarness` symbol (`nm -C`); the bridge's call is under `#if DEBUG && (TARGET_OS_OSX \|\| TARGET_OS_SIMULATOR)`. The script `check_no_attach.sh` (B1, Longer) was not written |
| AC-14 | Pass | `c59_5_ac14_wait_passes_on_the_app_clock` (300 ms: ≥ 0.29 s wall, 30,000 to 34,000 ticks), `c59_5_ac14_wait_at_full_speed_sooner`, `c59_5_ac14_harness_never_advances_the_machine`; the XCUITest's `wait 1s` took 0.95 to 1.5 s |
| EC-1 | Pass | `c59_5_ec1_port_in_use_host_runs_and_log_says_so` (bind fails, the host runs); the bridge writes "Attach is not available on port N: …" to the session's log |
| EC-2 | Pass | `c59_5_ec2_gone_without_quit_as_never_attached` (the key it held comes up, the next client attaches) |
| EC-3 | Pass | `c59_5_ec3_command_refused_while_suspended` (`refused: the app is suspended`), `c59_5_ec3_quit_still_works` |
| EC-4 | Pass | `c59_5_ec4_second_harness_one_line_and_closed` |

Also: `c59_5_operation_reply_when_it_ends` (a command that starts an
operation over attach is answered when it ends).

UI regression run (Green after of M-33): the whole plan `UIRegressionTests`
on the iPhone 18 Pro Max simulator (iOS 27.2), Debug: 16 tests, 15 passed,
1 skipped as before, 0 failed, 131 s; `AttachTests.testHarnessDrivesTheApp`
8.6 s (9.5 s alone). Its screenshot shows 123 typed by the harness. The
preset, file-picker and layout tests pass on the new session unchanged.

### Deviations from the design, chosen by the simpler structure

- **The bridge keeps the last message of each topic** on the main thread
  (a `TopicMirror` in `TIAppSession`) and keeps today's getter names for the
  view model, instead of `TIMessage` objects per topic that the view model
  stores (§3.3). It reads only messages, never the session; `TIChange` stays
  as the set of topics a batch changed.
- **No Mailbox class**: the two queues are members of `SessionThread`. A
  command's reply reaches its sender through a handler the bridge passes
  (`SessionThread::send(command, onReply)`), used for `fitCard`,
  `fitBlankCard` and `openDroppedFile`, which became asynchronous.
- **`CALCU_ATTACH` is on by default in a POSIX tree with the tests** (macOS,
  Linux), so the attach tests run in Frequent and in the CI `library` jobs
  without a workflow change; off elsewhere, and off in the XCFramework's
  Release and device slices.
- **Commands over attach that start an operation** reply when it ends
  through `HarnessHost`'s attached form (`kPendingReply`); the reply then has
  no result lines.
- **Not built in the first part:** the Swift unit-test target (S1, S2), S4, S5, S6,
  `check_no_attach.sh` (B1's Longer half; checked by hand instead). Built in
  the second part (below).

## M-33, second part (2026-11-15, branch `m33b-sessions-59`): S1, S2, S4 to S6, B1

The items the first part left open. Same machine and tools; the unit tests
and the XCUITests on the iPhone 18 Pro Max simulator (iOS 27.2) and on macOS
27.2. The unit-test target `Calc-U-59Tests` (plan `UnitTests`) is hosted by
the app (launched with `--UITesting`, so its settings stay in memory) and
makes a second session through the real bridge (`TIAppSession`) with a
temporary save folder (`Calc-U-59Tests/TestSession.swift`); the Debug-only
test hooks of the bridge (`pressKey:completion:`, `offeredHostServices:`,
`emulatedTime:`, `hostCallCount`) translate only.

This section supersedes the rows of the first part for the criteria it lists.

| AC | Result | How it was checked |
|---|---|---|
| AC-4 | Pass | as before; also `MessageOrderTests.testSessionThreadCallsAbout60TimesASecond` (50 to 70 calls a second through the bridge; iOS and macOS) |
| AC-5 | Pass | as before; S5 `AttachTests.testTapsReachSessionAtFullSpeed` (a program runs; a 3 s press on the display: the machine ran at 989 to 1,246 times wall time, so the touch reached the session as full speed; after its end 0.99 to 1.01, so the end did too; then the R/S key stops the program, `lcd` shows 1). D-9, the main thread's frame gaps: `FrameGapTests` (see below) |
| AC-6 | Pass | as before; S1 `MessageOrderTests.testMessagesArriveOnMainInOrder` (500 key commands, every reply on the main thread and in the order sent; iOS and macOS) |
| AC-7 | Pass | S2 `HostServicesTests`: `testStatedServicesIOS` (clipboard text and image, file picker, keep file, open URL, haptics), `testStatedServicesMacOS` (clipboard text and image, file picker, folder picker, keep file, open URL), `testNoAudioSink`; each checks the list the bridge states and the services the session sees offered (`SessionContext::hostOffers`, asked on the session thread) |
| AC-8 | Pass (iOS); macOS as D-10 records | `SuspendTests.testSuspendStopsCallsContinueCallsAgain` (after the suspend is carried out no host call and no emulated time for 0.5 s; after continue both again; iOS and macOS); S4 `AttachTests.testBackgroundSuspendsAndContinues` (Home: the next command is refused "the app is suspended"; 5.1 s of wall time from Home to back in front gave 1.8 s of emulated time, the time until the scene went to the background and after it came back; back in front `wait 500ms` passes on the app's clock). On the Mac (S6) a hidden app did not get `.background` from SwiftUI: a command while hidden was carried out (D-10: the app suspends when SwiftUI reports the background, which hiding does not) |
| AC-9 | Pass (after a fix) | S6 `tests/mac_attach_check.sh`: the window covered by a window over every screen for 30 s: emulated 30.043 s in 30.027 s of wall time. **Found:** before the fix the covered app ran 17.1 s in 30 s (13 s lost); macOS coalesced the session thread's waits past the pacer's lag limit of 100 ms (LIB-17 AC-4), which drops the lag. The App Nap activity now also has `.latencyCritical` (`EmulatorViewModel.swift`); with it the rate held in front, behind another app and covered. Not in the ledger: it keeps 2.3.0's behaviour (first step 12) |
| AC-10 | Pass | as before; S6: `calcu attach --port 59171` to the Debug Mac app started through Launch Services |
| AC-13 | Pass (after a fix) | `check_no_attach.sh`: `--sources` (Frequent, `CalcU59NoAttachSources`: attach only for Debug macos and ios-simulator slices in `build_xcframework.sh`, the bridge and Swift call sites under the Debug and platform guard); `--binary`, a build phase of the app target in every Release and device build (no `AttachEndpoint` or `AttachedHarness` symbol and none of attach's texts in the executable and its `.debug.dylib`; a file it cannot read fails); the full check (Longer, `CalcU59NoAttachInRelease`: the Release Mac app, the three Release XCFramework slices, the Debug iOS device app and its slice; the signed Release app's entitlements have the sandbox and no `network.server`). **Found:** a Release build right after a Debug one linked the Debug XCFramework, with attach: the script phase that makes the XCFramework declared no output, so Xcode copied the XCFramework (`ProcessXCFramework`) while the script was still replacing it. The phase now declares the XCFramework as its output; the scenario (Debug XCFramework, fresh derived data, Release build) passed three times since. This also holds AC-2 for a change of configuration |

D-9, the main thread's frame gaps (AC-5): `FrameGapTests.testMainThreadKeepsFramesWhileSessionRunsAtFullSpeed`,
a display link (iOS) or a 60 Hz timer (macOS) in the test bundle on the
host app's main thread, as first step 3 measured; idle, a program running at
regular speed, the program at full speed (the session thread busy for whole
calls: 1,279 to 1,395 times wall time), and 20 key commands sent then:

| | iOS simulator | macOS |
|---|---|---|
| idle | p50 16.7 ms, p95 16.7 ms, max 16.7 ms | p50 16.7, p95 18.5, max 18.6 |
| program, regular speed | 16.7 / 16.7 / 16.7 ms | 16.7 / 18.1 / 18.5 |
| program, full speed | 16.7 / 16.7 / 16.7 ms, no gap over 25 ms | 16.7 / 17.9 / 18.6, none over 25 ms |
| a key command's reply at full speed | at most 14.1 ms | at most 11.1 ms |

As first step 3 (p95 16.7 ms on iOS) and better than its 53 ms gap on macOS.
**Not with Instruments as D-9 says:** `xctrace record --template 'Animation
Hitches'` against the app in the simulator ends "Hitches is not supported on
this platform"; it needs a real iPhone. The probe is in the test bundle, not
the app (D-9: no probe in the app). For the user, on a device: run the app
from Xcode (Debug), Product → Profile → Animation Hitches, a program running
(`2nd Lbl A 1 SUM 00 GTO A`, `A`) and the display held for full speed.

Runs (2026-11-15): `UnitTests` 6 of 6 on the simulator and on macOS (14 s);
`UIRegressionTests` on the simulator 18 tests, 17 passed, 1 skipped as before,
178 s; `PacingCheckpoint` (LIB-17 AC-19) passed; `mac_attach_check.sh`
passed; `check_no_attach.sh` (full) passed; Frequent 27 of 27.

### Deviations from the design, chosen by the simpler structure

- **The unit tests make their own session through the bridge** instead of
  driving the app's: the view model has no seam for a test, and a second
  `TIAppSession` is the same bridge and session thread.
- **S5 has no `speed full` over attach** (LIB-11 has no such command): the
  test holds the display, the user's way to full speed (an accessibility
  identifier `led-display` on the display view), and taps R/S through the
  `--UITesting` aid `btn-key-rs`.
- **S6 and the Mac's AC-19 are one script** with `calcu attach`, not an
  XCUITest on macOS (that needs a signed runner and the Accessibility
  permission, `CalcU59UITestsMac`). The app is started through Launch
  Services (`open -n`): started directly from a shell it is no regular
  foreground app and lost 8 s a minute in front.
- **S2 on macOS is `CalcU59SwiftUnitTestsMac`** (Checkpoint), a run of the
  `UnitTests` plan, not part of `CalcU59UITestsMac`.
- **EC-3 in the simulator:** a command sent right after Home is still
  carried out until the scene has gone to the background; S4 repeats it
  until it is refused.
