# Part C1: structural simplification before the boundary

Status: done (2026-11-15). The survey (§1) and plan (§3) were written and
committed (`8b99976`) before any code edit; §4 to §8 report the result.

Baseline: `main` at `46344b0`; Frequent 28/28 in 71 s.

## 1. Survey (read-only), ranked by payoff

Payoff = lines removed × concepts removed × what it saves the C2–E steps.
Files are relative to the repository root; `59/` is `products/calc-u-59/`,
`1600/` is `products/calc-u-1600/`.

### S-1. Talking to an app session is written six times

Every host of the library's app session builds its own listener with a
`TopicMirror`, its own reply bookkeeping ("send, then see whether the reply
came already"), its own `state<P>()`, and owns the parts and the session in
its own order:

| Where | What it holds |
|---|---|
| `1600/app/SharpHost.*` | parts + session (part first down), mirror, `m_replies`, `takeReply`, `runToEnd`, `ask`, `state<P>`, `display()` |
| `1600/frontends/qt6/app/MachineController.cpp:80-156` | a second pending-reply map on top of `SharpHost` |
| `59/app/TIAppPart.hpp` `TIAppSession` + `makeAppSession(before, after)` | parts + session (part first down) |
| `59/frontends/qt6/app/EmulatorController.*` | own sink and mirror, a one-slot `m_lastReply`, own `state<P>` |
| `59/app/SessionThread.*` | `HostedSession` interface (one implementation `AppSessionHost` + a test fake) and a reply-handler map |
| `59/frontends/swift/Bridge/TIAppSession.mm:318-376` | own mirror and `lastOf<P>` |
| `library/harness/HarnessHost.*` | mirror, `replies_`, `takeReply`, `takeReplyIfAny`, `display()` |
| `library/web/glue/WorkerHost.hpp` `WebSession{keep, session}` | the session destroyed **before** the parts |
| `1600/frontends/web/bindings.cpp`, `59/frontends/web/bindings.cpp` | the same `WebKeep` struct, the same `forgetSession()` workaround for that order, the same "Settings: problem" log line |

Both parts also need a second step after the session exists to put their
hook, gate and probe on the machine session (`SharpAppPart::bind`,
`TIAppPart::start`), because `SessionSetup` gives a part no access to the
machine session at `attach`. `AppSession::context()` exists only for that.

### S-2. Calc-U-59's app part still runs on the threaded runner's plumbing

Calc-U-59 was moved onto the session second, keeping 2.3.0's mechanics
inside the part:
- the 14-bit `TIAppPart::Change` flags, a `setChangeObserver` channel whose
  only subscriber is `TIWebPart`, and interim payloads compared by a serial
  (`TISnapshot`) instead of by value, so the topic's own equality is
  bypassed. Calc-U-1600's parts publish values and let the topic decide;
- `EmulatorSession`'s run flag, job queue and "run mark"
  (`EmulatorSession.cpp:108-128`): every `sync()` follows a `stop()`, so the
  mark never holds a job back;
- `EmulatorSession::Listener` (one implementation, `TIAppPart::Events`)
  whose every callback is wrapped in `later(generation, …)` and queued;
- locks with one thread: `KeySequencer` (two mutexes, two atomics),
  `FreezeController` (four atomics), `TraceWriter` (a mutex, six guards);
  1600's `core/CPU/TraceRing.hpp` mutex and the atomic trace flags of
  `LH5801`/`SC7852`;
- comments that describe threads that are gone (`EmulatorSession.hpp:82,
  85, 186`, `FreezeController.hpp:13-14`, `TraceWriter.hpp:15-16`,
  `AsmOverlay.hpp:31`, `StateApplier.hpp:23`, `core/TMC0501.*`, 1600's
  `SerialLink.hpp:14-18` and the "locked debugPeek" notes).

### S-3. Test helpers copied across the library and both products

- 14 temporary-folder helpers (several with fixed names or leaked
  `/tmp` folders), 8 scripted wall clocks, 5 hand-written 60 Hz pumps,
  12+ `readText` one-liners, 3 `problemOf`, 3 serial-link fakes;
- Calc-U-59 has two session rigs for one job (`tests/PartRig.hpp`,
  `tests/AppSessionRig.hpp`), and three sinks that re-implement the
  library's `TestFrontend`; the library's stand-ins sit in
  `library/<area>/tests/`, which the products cannot include, while
  `library/testing/` is on every test target's path;
- 116 hand-written SKIP blocks in 24 Calc-U-1600 test files that count as
  passes, so the runner's skip count is wrong (`testharness::skip()` is
  the only counter);
- tests that repeat library ACs through a product (LIB-9 first start,
  LIB-12 lookup mechanics, LIB-3 suspend), two tautological checks.

### S-4. Adapters past their removal step

| Adapter | Due | Where |
|---|---|---|
| `TIPacingHints` (`pacingHintsOf`, `takePolledHardware`) | M-33 | `59/core/TIAdapters.*`, `TIMachine.*` — no caller |
| LIB-11 A-3, the minimal `ProductDecl` | M-30, M-32 | both `app/HarnessProduct.cpp` |
| the Calc-U-1600 harness's own ROM lookup | M-27 | `1600/app/HarnessProduct.cpp:26-37, 95-98` (differs from `sharp::lookUpRoms`) |
| LIB-12 A-3 `moduleRomFile` | M-25 | `59/app/ModuleCatalog.cpp:55`, `EmulatorSession.cpp` — check against the design |

### S-5. Value and argument helpers copied between the products

`ValuePayload` and `text`/`integer`/`real`/`flag`/`bytes` are copied word
for word in `59/app/TIWebPart.cpp:31-67` and `1600/app/SharpWebPart.cpp:33-75`;
argument readers again in `SharpAppPart.cpp:98-116`, `TIAppPart.cpp:130-146`
and two library test products, with drifting rules (one `intArg` takes
reals, one does not). The steady wall clock is written four times
(`cu59::steadyWallTime`, `SharpHost.cpp`, `MachineController.cpp`,
`web::wallTimeNow` is the Web's own and stays). The host-service name table
is copied into `TIAppSession.mm` from `MessageCodec.cpp`. The harness has
`ticksFor`/`timeReply` twice (`FirstCommands.cpp`, `AttachedHarness.cpp`).

### S-6. CMake repeated per product

38 `add_test` + `set_tests_properties` pairs; the source checks copied
between the products; the Web engine link flags three times; warning flags
five ways (MSVC level differs); library area lists kept in three places;
dead copies of root settings (`CMAKE_OSX_DEPLOYMENT_TARGET` ×3 that never
apply, Homebrew prefix blocks that never run, `enable_testing`, C++
standard ×4, a second `project()`, an always-true `if(COMMAND …)`).

### S-7. Smaller items

- `[folders]` settings code copied verbatim (`1600/app/Settings.cpp:97-128`,
  `59/app/Settings.cpp:171-198`); the Qt `AppPaths` bootstrap copied.
- The start and the missing-ROM rule work two ways (1600: `start` command
  and `roms-missing`; 59: lookup before the session, `c59.start-machine`).
- 59 packs a title into a notice and every frontend splits it.
- Unused: `EmulatorSession::scriptedTrace()`, `SerialPortQueues::
  receivedWaiting()`, `CommandRegistry::commandNames()`; stale comments in
  `roms/FullSet.hpp:35`, `SessionThread.hpp:23`, `KeySequencer.hpp:41-42`.
- `SessionContext::display()` serves 1600's Web part only.
- Both Web pages have the same `engine.js` skeleton; 1600's page has a
  verbatim copy of the library's `calcuPickFiles`.
- Both Web parts publish a second, page-form copy of each product topic
  (`SharpWebPart.cpp` 631 lines, `TIWebPart.cpp` 663 lines).

## 2. The three largest changes if effort were free

1. **One way to host and talk to an app session (S-1).** A library
   `calcu::sessions::Client` (listener, mirror, reply correlation with
   optional handlers, `state<P>()`, `display()`) and a library owner of a
   session with its parts that destroys the parts first; `SessionSetup`
   gives `attach` the machine session, so `bind()`/`start()`'s hook
   installation, `AppSession::context()` and both `forgetSession()` go.
   `SharpHost`, `TIAppSession`/`makeAppSession`, `HostedSession`/
   `AppSessionHost`, `WebKeep` and the Qt controllers' reply maps shrink to
   the product's part list. LIB-5 (M-36) then adds `TypingPart` in one
   place per product instead of in every frontend's setup.
2. **Calc-U-59's part on the session model, not on 2.3.0's (S-2).** Topics
   that hold values and are refreshed where 1600's are; `TIWebPart`
   refreshes from the part like `SharpWebPart`; no change flags, no
   observer, no serials; `EmulatorSession` without the run mark, the
   listener interface and the `later` queue; no locks in the single-thread
   classes.
3. **One test toolkit (S-3).** `TestFrontend`, the scripted clock, the
   temporary folder, the file helpers and the 60 Hz pump in
   `library/testing/`, used by the library and both products; one
   Calc-U-59 rig; `SKIP_UNLESS` everywhere; the duplicate tests go.

What came of each is in §5.

## 3. Plan: independent commits, one short branch each

| # | Branch | Does one thing |
|---|---|---|
| P-1 | `c1s-overdue` | overdue adapters and dead code (S-4, S-7 unused accessors and stale comments) |
| P-2 | `c1s-one-thread` | locks and thread comments of single-thread classes (S-2, last two bullets) |
| P-3 | `c1s-session-client` | large change 1 (S-1) |
| P-4 | `c1s-59-topics` | large change 2 (S-2, first three bullets) |
| P-5 | `c1s-test-toolkit` | large change 3 (S-3) |
| P-6 | `c1s-values` | Value, argument, clock and name helpers in the library (S-5) |
| P-7 | `c1s-cmake` | CMake functions and dead settings (S-6) |

Each: build, Frequent (golden replays included), merge `--no-ff`. At the
end: the Longer group locally, then CI.


## 4. Changes made

Every change: build, Frequent 28/28, golden replays unchanged, merged
`--no-ff`. Lines are `git diff --shortstat` of each merge (code, tests and
docs together).

| # | Change and reason | Before → after | Lines |
|---|---|---|---|
| P-1 | Adapters past their step, dead code | `TIPacingHints` (no caller since M-33) gone, `idleAndKeyDetectRange()` kept as a free function (headless tool); both harness products return the product's `settingsDecl()` (LIB-11 A-3, due M-30/M-32: `--settings` on a missing file now writes the product's full first-start INI); the 1600 harness uses `sharp::lookUpRoms` (its own `FoldersBundle` due to go in M-27); `scriptedTrace()`, `commandNames()` gone; stale comments | +36 −96 |
| P-2 | Locks in classes used by one thread | `KeySequencer` (2 mutexes, 2 atomics), `FreezeController` (4 atomics), `TraceWriter` (mutex), 1600 `TraceRing` (mutex), `LH5801`/`SC7852` atomic trace flags → plain members; thread comments rewritten | +95 −148 |
| P-3 | Large change 1: one way to host and talk to a session | `library/sessions/Listener` (send with a reply handler or an at-once reply, `ask`, `runToEnd`, mirror, `state<P>`, `display()`); `AppSessionSetup::owned` released before the session; `SessionSetup::context()` at attach; `ProductSession<Part>`. Gone: `SharpHost` (→ `makeSharpSession`, 54 lines), `SharpAppPart::bind`, `TIAppPart::start`'s hook setup, `TIAppSession`, `HostedSession`/`AppSessionHost`, both `forgetSession()`, both `WebKeep` destructors, `WorkerHost`'s own holding and call-id map, the Qt controllers' reply maps, the Swift bridge's `lastOf` | +827 −768 |
| — | Bug: display payload pointed into a destroyed machine | `IndicatorOutput::name` `string_view` → `std::string` (ASan use-after-free in `sameDisplay` after a model switch, on `main` before this work) | +4 −4 |
| P-4 | `EmulatorSession` without the threaded runner's run mark | `start`/`stop`/`isRunning`/`post`/`sync`/`drainJobs` → a halt flag and a FIFO of the debugger's queued steps, run after the host call as before | +104 −135 |
| P-5a | Large change 3, first half: one test toolkit | `library/testing/TempFolder`, `Files`, `ScriptedClock` replace ~14 temp folders (fixed names, leaked `/tmp`), 8 clocks, 12 file one-liners; 93 hand-written skips → `SKIP_UNLESS` (now counted); one 1600 serial-link fake; ROM probes through the lookup; two tautological checks | +664 −1148 |
| P-5b | Large change 3, second half | one Calc-U-59 `SessionRig` (was `PartRig` + `AppSessionRig`); `TestFrontend` in `library/testing/`; `Pump.hpp`; `testroms::appSetup` replaces six setup blocks; 1600 `SessionFixture` on a `TempFolder` (parallel runs collided); `NoOwnRomLookup` exception gone | +306 −350 |
| P-6 | Helpers copied between products | `ValuePayload`, `sessions/Args.hpp`, `pacing::steadyWallTime`, `serviceName`/`serviceNamed`, harness `ticksFor`/`timeReply`/`payloadText`, `settings::fixedOpenFolder`, `sessions::kStartCommand`, each once | +328 −366 |
| P-7 | CMake per product | `calcu_add_test`, `calcu_product_source_checks`, `calcu_web_engine`, `calcu_warnings` (MSVC `/W4 /utf-8` everywhere), `calcu_debug_o1` at the root, one list of library areas, `TestMachines.cpp` compiled once, CLI cases read from `cli_cases.cmake`; dead copies of root settings gone; CTest listings (28 / 46 / 59 entries) identical before and after | +334 −429 |

Total: +2 698 −3 444.

At the end: Longer group locally, 19 of 20 pass (Playwright of the library
and both Web frontends, `CalcU59UIRegression`, `CalcU59SwiftUnitTests`,
`CalcU59NoAttachInRelease`, both ROM-missing builds, the WASM smoke, the
nine `CalcU1600Shots`, `CalcU1600DapSmoke`); `CalcU59ReferenceRuns`
differs in 34 of 57 examples, as after M-34 (§8, item 3). CI on `main`:
43739798405 (Calc-U-59), 43739801132 (Calc-U-1600).

## 5. The three large changes, and what came of them

1. **One way to host a session: done (P-3).** Six hand-written hosts
   became one library `Listener` plus the product's part list. LIB-5
   (M-36) adds `TypingPart` in each product's factory instead of in every
   frontend's setup. `AppSession::context()` stays for host log lines and
   tests (comment says so).
2. **Calc-U-59's part on the session model: in part (P-2, P-4).** Done:
   locks, the run mark, the gone-thread comments. Rejected: the 14 change
   flags, the observer to `TIWebPart` and the `TISnapshot` serials. They
   are the part's dirty tracking; replacing them by values compared each
   call makes `TIWebPart` scan the save folder (card list) and the trace
   folder and copy the debug listing on every host call, and M-53, M-55,
   M-56 and M-58 replace each interim topic with its owner's topic. Also
   kept: `EmulatorSession::Listener`/`later()` (M-36, M-53 remove them).
3. **One test toolkit: done (P-5a, P-5b).** Not merged: the 59 rig keeps
   its own clock (steps before it reads; `ScriptedClock` would shift every
   reading by 1 ms and change the reference runs); 1600's pump counts
   calls, not time; the library's CTest entries stay off
   `calcu_add_test` (no working directory).

## 6. Rejected candidates

| Candidate | Reason |
|---|---|
| One page-form topic set per product (`SharpWebPart` 631, `TIWebPart` 663 lines duplicate the product topics) | the interim topics go with their owners (LIB-4 M-49, LIB-8 M-53, LIB-7 M-55, C59-4 M-56, LIB-14 M-49/M-57); merging now changes both pages' contract for code those steps replace |
| A library `engine.js` skeleton for both pages | the pages move onto LIB-14's shell (M-49, M-57) |
| Products on the library's host services (pickers, clipboard) | the services are specified (LIB-3 AC-26, AC-27); moving the pages' own pickers onto them is LIB-13/LIB-14 work (M-48, M-49, M-56, M-57), not structure |
| 59 on 1600's part-owned `start` and `roms-missing` | LIB-18 (M-35, M-46, M-54) builds the start for both |
| Notices without a packed title in 59 | C59-8 (M-58) changes the alerts; a split is user-visible in the alerts today |
| `PtyHostLink` without threads (non-blocking poll) | changes its back-pressure; no measurement says it pays |
| F23/F25 test merges (ROM lookup tests, long-run call schedule) | the product tests assert product facts; the long-run schedules replay M-10's reference |
| Shared `textArg`/`runFor` in three library test targets | would add test include paths for ~10 lines |
| `/tmp` paths in ~8 preset tests | the tests assert `/tmp/…` results of the preset parser's relative paths |

## 7. Departures from approved `design.md` files

- LIB-1 §3.6: `IndicatorOutput::name` is a `std::string`.
- LIB-3 §3.1 (`ValuePayload`, `Args.hpp`), §3.6 (service names), §3.8 to
  §3.10 (`SessionSetup::context()`, `owned`, `ProductSession`,
  `TopicMirror::stateOf`, the `Listener`), §2 file table (test rig).
- LIB-9 D-2: the reset-to-last-used rule is the library's
  (`setFixedOpenFolder("")` keeps the fixed folder as the start; the
  products already did this).
- LIB-11 §3.4: `HarnessHost` holds a `Listener` instead of being a sink.
- LIB-15 §2: `WorkerHost` row (`WebSession` gone).
- C59-5 §2, §3, D-11, the H tests: no `HostedSession`; the thread owns the
  library session.
- LIB-5 (C2 draft): two mentions of the rigs updated.

## 8. Questions and findings for the user

1. `products/calc-u-1600/docs/background/Decisions.md` said the core's
   machine mutexes stay; they no longer exist, and the line now says the
   core holds no lock. It is a recorded decision: please confirm.
2. `CoreTests` crashes (exit 139) when the ROMs are missing, early in the
   app-layer tests (`test_describe`); it did before this work. Fix with a
   SKIP in Part C2?
3. No golden replay or reference run differed. The 57 TI reference runs
   recorded with P-5b's branch and with `main` before it are the same; the
   M-34 finding (34 of 57 differ from the M-9 references, printer timing at
   full speed) is unchanged and stays with M-36.
4. No spec change is wanted for any of the above.

## 9. Line counts

Counted by `find … | xargs cat | wc -l` over `.cpp .hpp .h .swift .js .mm
.py CMakeLists.txt .cmake`, without `node_modules` and `dist`.

| | Before (`46344b0`) | After |
|---|---|---|
| `library/` | 21 380 | 21 706 |
| `library/` without tests | 11 222 | 11 761 |
| Calc-U-59 `app/` | 10 689 | 10 470 |
| Calc-U-1600 `app/` (without `app/tests`) | 6 510 | 6 327 |
| Calc-U-1600 `app/tests` | 1 729 | 1 740 |
| Calc-U-59 `tests/` | 7 326 | 7 226 |
| Calc-U-1600 `tests/` | 24 692 | 24 223 |

The library grew by what moved into it from the products (the listener,
the argument and value helpers, the test toolkit); the products' app
layers and tests shrank.
