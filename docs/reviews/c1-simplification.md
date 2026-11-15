# Part C1: structural simplification before the boundary

Status: survey and plan written before any code edit (2026-11-15).
The results (changes made, line counts after, departures, questions)
are added at the end as the commits land.

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

## 4. Status (interim, 2026-11-15)

All of P-1 to P-7 merged into `main` (`923cef0`), Frequent 28/28 after
each merge, golden replays unchanged, Swift macOS build green. Also merged:
`DisplayOutput` indicator names owned (a use-after-free found under ASan;
LIB-1 design §3.6 amended).

Line counts, before → after: `library/` 21 380 → 21 706 (without tests
11 222 → 11 761: the Listener, Args, ValuePayload, test toolkit moved in);
Calc-U-59 `app/` 10 689 → 10 470; Calc-U-1600 `app/` 6 510 → 6 327;
product tests 59 7 326 → 7 226, 1600 24 692 → 24 223.

Still to do: the Longer group locally, CI (P-7 changed build files), and
the full report (§5 what came of the three changes, rejected candidates,
departures, questions). Departures and findings so far are in the merge
commits' branch reports: LIB-3 §3.1/§3.6/§3.8–3.10, C59-5 §2/§3, LIB-11
§3.4, LIB-15 §2, LIB-9 D-2, LIB-1 §3.6; findings: CoreTests crashes
without ROMs (exit 139, before this work too); 1600
`docs/background/Decisions.md` line on machine mutexes changed (confirm).
