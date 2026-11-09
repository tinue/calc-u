# Single-threaded sessions, hosted by the frontend

Status: accepted (2026-11-09)

App sessions and machine sessions are single-threaded. Emulated time
advances only through explicit calls (tick, advance), and long work is a
coroutine job that runs sliced or to completion (Calc-U-1600's model,
`Decisions.md`, "Shared is single-threaded"). A frontend hosts a whole
session on a thread of its choice (the GUI thread, a background thread,
or a Web Worker) and talks to it only by messages; a session is never
shared between threads. Tests, the harness and every frontend then run the
same code, in virtual time where needed, and Calc-U-59's two modes
(threaded runner, cooperative mode) become one. A browser cannot share
memory with a worker without COOP/COEP headers, which rules out a threaded
design on the web anyway.

## Considered options

- Calc-U-59's model for both products (threaded runner in the apps,
  cooperative mode for the web and tests): rejected; two code paths that
  must behave the same.

## Consequences

- No locks inside machines, cores or sessions. Calc-U-59's runner thread,
  key-sequencer lanes and core mutexes (`m_keyMutex`, `m_displayMutex`,
  `m_prnMutex`, `m_traceMutex`) go, as do Calc-U-1600's uncontended machine
  mutexes, `TraceRing` mutex and atomic trace flags. Calc-U-1600's
  decision "Core's machine mutexes stay (uncontended)" is superseded; its
  text is updated with the code in phase 6.
- Host adapters that block or are called back by the host, such as the PTY
  behind a serial port (today `PtySerialLink` with its own reader and
  writer threads) or an audio device callback, may use threads of their
  own. They hand data to the session through a thread-safe queue at that
  boundary, the only place that synchronizes.
- Whether the Swift frontend hosts its session on the main thread or on a
  background thread is decided with the Swift bridge; phase 5 may spike
  the PC-1600 on iOS.
