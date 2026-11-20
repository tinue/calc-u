# Idle on the PC-1500, PC-1500A and PC-1600

When the machine takes a key, waits, has settled, or has finished: the
signals, and how each question is answered from them. Addresses are those
of the annotated disassemblies (research skill `PC1500`): the PC-1500 A04
(the same in A01 and A03 for every routine named here), the PC-1600 NEW
ROM (OLD where it differs). "Measured" means with the harness.

| Question | Meaning | Asked by |
|---|---|---|
| A, takes keys | the ROM or a program reads the keyboard as input now; watching for BREAK alone is not A | a scripted press before it goes down |
| W, waits | nothing happens until the user acts: A, and nothing is busy | pacing (asked; full speed is not paced, see In the code) |
| settled | W held for 20 frames (1/60 s each), sampled every frame | the steps: preset items, `key`, `type`, harness `wait` |
| B, finished | no program and no CALLed machine code runs, the machine is at the BASIC prompt, and settled | `let-it-finish`, the wait "until the program has ended" |

## PC-1500 and PC-1500A

### Signals

| Signal | Where | Meaning |
|---|---|---|
| key wait | PC in WAIT_4_KB E243–E33D, and 7B0E (CURS_CTRL) bit 0 clear | the ROM waits for a key and takes one (bit 0 is cleared after 8 key-free passes, E2D4); the idle PC is E2AA, the `HLT` at E2A8 |
| key read for INKEY$ | BCMD_INKY D9AA: `sjp KEY_2_ASCII` E42C (the keyboard read `ITA` at E427); one read when the statement runs, no gate | a program reads a key; measured: every 17.3 ms in `10 A$=INKEY$:GOTO 10` |
| key read for TRACE | C419–C428: one KEY_2_ASCII per statement while tracing | the ↓ key; not measured |
| BUSY | 764E bit 0 (LCD RAM); set at every ENTER (CCC3), CONT (C8D5), a DEF key in RUN mode (CACF), ARUN (CA1D; A01 writes the wrong address); cleared only at CA88 in EDITOR_ENTRY CA80 | the ROM has left the editor to carry out a line and has not come back; measured on while a program, a direct command or CALLed code runs; off at the prompt, in INPUT, in a PRINT stop, at NEW0? |
| stop kind | BREAKPARAM 788A: 50H INPUT (C94C), bit 5 a PRINT stop (E6EC), bit 7 BREAK or STOP (C4D9), 0 at the prompt, after END, after an error | why the editor runs: for the user, or for a program that goes on |
| line | CURR_LINE 789C–789D: 0 in the editor (cleared at CA80) and during a direct command; the line number while a program or its CALL runs | — |
| PRINT stop | WAIT_CFG 7871: 0 (no WAIT statement, the default after NEW0, or `WAIT` alone, E86D) stops PRINT until ENTER (E6EC → the editor); `WAIT n` = 2, a delay; `WAIT 0` = 3, no stop (E884) | — |
| BREAK latch | ME1 F00B bit 1, the ON key's rising edge on PB7 (the emulator: `core/PC1500/PC1500Memory.cpp`) | tested by CHK_BRK E451 |
| off | the CPU powered off (OFF CD81, auto-off E359) | — |

While BUSY is on the ROM is in a synchronous operation (a program, BEEP,
the CE-150, the cassette), so at the key wait with BUSY off nothing is
busy: the tone has ended, the CE-150 has switched its motors off (MOTORS_OFF
A773), the cassette its relay (REMOTEOFF BF43). Only machine code started
by CALL can reach the key wait with something still going; the signals for
that: the buzzer line ME1 F008 bit 6 changing (BEEP E66F–E6A0), the CE-150's
motor phases ME1 B008 (not 0 while coils are on; not established that 0
means stopped), the cassette relay through B00E.

### The questions

- **A** = not off, and (the key wait ran in this frame or the last with
  7B0E bit 0 clear, or INKEY$ read a key in this frame or the last). By
  frames, not by the PC of the sample: the timer interrupt runs outside the
  loop (E22E). TRACE's key read is not counted.
- **W** = A, and the buzzer line did not change in this frame or the last
  (busy: "the buzzer"). The CE-150's motor phases are not asked: not
  established that 0 means stopped.
- **B** = W, BUSY off, BREAKPARAM bits 5 and 6 clear, settled.

| Case | A | W | settled | B | Signal |
|---|---|---|---|---|---|
| prompt, RUN and PRO mode | yes | yes | yes | yes | key wait, BUSY off, 788A 0 |
| NEW0? :CHECK after a memory change | yes | yes | yes | yes | the editor's key wait (COLD_START C9E4) |
| after END, an error, BREAK | yes | yes | yes | yes | 788A 0 or bit 7 |
| INPUT | yes | yes | yes | no | key wait, 788A 50H |
| PRINT stop (no WAIT statement) | yes | yes | yes | no | key wait, 788A bit 5 |
| `10 A$=INKEY$:GOTO 10` | yes | yes | yes | no | key read at D9AA each pass, BUSY on |
| CALL to code that waits in WAIT_4_KB (`CALL &E243`) | yes | yes | yes | no | key wait, BUSY on |
| CALL to code with its own keyboard scan | — | — | — | no | no general signal: the test gives seconds |
| `10 GOTO 10`, BEEP, a CE-150 plot or print, CSAVE | no | no | no | no | not in the key wait, BUSY on |
| PRINT after `WAIT n` (n > 0) | no | no | no | no | TIME_DELAY E88C: a timed delay, part of the program's run time (BREAK can end it) |
| CLOAD waiting for the tape (no cassette, at its end) | no | yes | yes | no | a medium awaited, [below](#a-medium-awaited) |
| switched off | no | no | no | no | powered off |

### Keys that do not wait for A

| Key | How the ROM sees it | Hold |
|---|---|---|
| ON/BREAK | latched: F00B bit 1, tested once per statement (C42A), in each key-wait pass (E269), in TIME_DELAY per 15.625 ms tick (E8A3, E8B0), in BEEP's tone loop each half period (E683), in the CE-150's BRK_150 A746 (after each character, GRAPH step and feed step) and SAVE_NIBBLE BDD6 | any: a 1 ms press broke `10 GOTO 10` (measured). During CLOAD the loader reads the PB7 level instead (BE2F, BE4C): held while it polls; how long is not established |

ON/BREAK from a command (the harness's or the driver's `key`) waits for A
at most 0.2 s, then goes down; a preset's key waits for A as any key does.
OFF is not one of them: it works only from the editor (EDCTRL_TBL C3A9 →
BTN_OFF CD71), as on the real machine.

## PC-1600

MODE 1 changes no row: the Z-80 runs BASIC in both modes (BMODE F1BCH
bit 6 changes the keywords); the LH5803 runs one PC-1500 statement or
function per CALLH, and machine code started by XCALL.

### Signals

| Signal | Where | Meaning |
|---|---|---|
| key wait | KEYWK2 F07AH bit 2: set by KEYGET (0166H; bank 6 9296, the `HALT` at 92B2, OLD 92B9) while it waits, cleared at 92B9 | the ROM waits for a key: the editor (prompt, INPUT, PRINT stop) and any caller of KEYGET |
| key read | KEYGETR (0169H, the next key from the buffer) or KEYDIRECT (0175H → bank 6 9398, the matrix now; it empties the buffer) called; INKEY$ uses KEYDIRECT, INKEY$(1) KEYGETR (P1-B0 65E2, 65DD; OLD 6751, 674C) | a program reads a key |
| key scan | the 1/64 s interrupt KEYSCAN (bank 6 90D1 → bank 3 KEYMATRIX 4768) reads the matrix in nearly every frame, whatever runs, and puts keys into the 64-byte KEYBUF (KBPUT 912E) | not A: it runs while the machine is busy too; keys typed meanwhile wait in the buffer for the next KEYGET |
| BUSY | SYMB0 F64EH bit 0: set by MODCLEAR (P1-B0 7BE3: ENTER 6D14, a RUN-mode key 69F9, a single step 6E6E), cleared by CHKMSGRST (70DC) | measured on while a program runs, during CALLed code waiting in KEYGET (from a program and direct); off at the prompt, in INPUT, in a PRINT stop, after END and BREAK; not every place that clears it is known |
| stop kind | INTFLG F88AH: bit 7 stopped (BREAK, STOP; SCONT_I 234C), bit 6 a PRINT stop (PRTSTOP 6461), bit 5 INPUT (5ED3), 0 after END (ENDPRG 3BE2) | why the editor runs |
| line | CURLINE F89CH–F89DH: 0 in the editor (EDLOOP 6844) and for every direct command (DIRECT 3920); the line while a program runs, and during a CALL from it | — |
| PRINT stop | F871H, the PRINT wait mode: RUN sets 3 in MODE 0 (no stop) and 0 in MODE 1 (RUNSET3 1C7C); `WAIT` alone sets 0: PRINT stops (PRTSTOP 644D → the editor); `WAIT n` sets 2: a timed delay | the same modes as the PC-1500; only the default differs: in MODE 0 PRINT stops only after a plain `WAIT` |
| LCD clock | CK0 on | off: the display and the keyboard are dark |
| CE-1600F motor | the drive's motor (emulator: `ce1600fMotorOn()`) | a disk access |
| CE-1600P pen | stepper phases written to ports 82H (pen, colour) and 83H (X, Y); ALLOFF (IOCS 18H) cuts the motor power; the timing is a busy loop with interrupts on (MOTORDLY, CE-1600P bank 4 53B7) | plotting or printing; no busy flag in hardware: the emulator has to note the motor power or recent phase writes |
| tone | SOUT (bank 3 5EB9) masks the 1/64 s and sub-CPU interrupts while it sounds | BEEP |
| off | the system off (OFF and auto power-off end in the LH5803's OFF routine E527) | — |

### The questions

- **A** = the system on, and the last 15 frames each had CK0 on and the
  key wait at the frame's end or a key read in it.
- **W** = A, and the CE-1600F's motor off (busy: "the CE-1600F's motor").
  The CE-1600P's motor power is not asked: while it plots the ROM is not
  in the key wait anyway.
- **B** = W, the key wait, BUSY off, CURLINE 0, INTFLG bits 5 and 6 clear,
  settled.

| Case | A | W | settled | B | Signal |
|---|---|---|---|---|---|
| prompt, RUN and PRO mode, MODE 0 and 1 | yes | yes | yes | yes | key wait, BUSY off, INTFLG 0 |
| after END, an error, BREAK | yes | yes | yes | yes | INTFLG 0 or bit 7 |
| INPUT | yes | yes | yes | no | key wait, INTFLG 20H |
| PRINT stop (after `WAIT`, or MODE 1) | yes | yes | yes | no | key wait, INTFLG 40H |
| `10 A$=INKEY$:GOTO 10` | yes | yes | yes | no | KEYDIRECT each pass, BUSY on |
| CALL to code that waits in KEYGET (`CALL &166`) | yes | yes | yes | no | key wait, BUSY on |
| CALL to code that polls KEYGETR | yes | yes | yes | no | key read |
| CALL to code with its own matrix reads | — | — | — | no | no general signal: the test gives seconds |
| `10 GOTO 10`, `FOR … NEXT` | no | no | no | no | no key wait, no key read |
| PRINT after `WAIT n` (n > 0) | no | no | no | no | PRTEND's timed wait (SWAIT): part of the program's run time (BREAK can end it) |
| BEEP | no | no | no | no | SOUT; measured: `wait` lasted the 74 s of `BEEP 20,100,2000` |
| a direct LOAD, SAVE, COPY (S1:, S2:, S3:, X:; COM1: while bytes come) | no | no | no | no | no key wait |
| a LOAD from COM1: or the cassette with nothing to come | no | yes | yes | no | a medium awaited, [below](#a-medium-awaited) |
| plotting, printing on the CE-1600P | no | no | no | no | no key wait (the motor power is not asked) |
| the CE-1600F's motor on at the prompt | yes | no | no | no | motor |
| switched off, auto power-off | no | no | no | no | system off |

### Keys that do not wait for A

| Key | How the ROM sees it | Hold |
|---|---|---|
| ON/BREAK | latched: the IF latch, port 1BH bit 1 (rising edge of PB7), tested before every statement (STMTCHK 3AC0 → BREAKHIT 3C8D; OLD 3B65), in the editor loop (6862), in BEEP, the plotter (PBREAKCHK, CE-1600P bank 4 5788), the floppy (FDFILEEXIT, bank 5 42C9), the cassette and the serial loops; the level, port 1FH bit 7, by KEYMATRIX and WAITONUP (0775) | any for the latch; held while a routine waits for the release (WAITONUP) |

ON/BREAK from a command waits for A at most 0.2 s, then goes down; a
preset's key waits for A as any key does.
OFF is not one of them: a matrix key handled only by the editor (EDOFF
P1-B0 6B3F; ignored during a program's INPUT wait and a PRINT stop with
F0BBH bit 0).

## A medium awaited

A program or command that waits for the tape or for the serial port waits
for a medium, as the TI-59 waits for a magnetic card: W without A. Only
while nothing arrives: in the loops below the ROM also waits between two
bits of a playing tape and between two bytes of a feed, which is work.

| Model | Wait | ROM signal | Nothing arrives (the emulator) | Ends |
|---|---|---|---|---|
| PC-1500, PC-1500A with CE-150 | the tape loader (CLOAD, MERGE, CHAIN, INPUT# from tape) | an edge wait of LOAD_NIBBLE (CE-150 BE02–BEF7): BE11–BE1C, BE21–BE2C, no timeout | the cassette does not play: none in, the motor off, or at its end | an edge; BREAK (PB7, BE19, BE29) |
| PC-1500, PC-1500A with CE-158 | receiving (INPUT with `SETDEV KI`, CLOAD, MERGE, INPUT# with `CI`) | the receive loop 828E–82A0 with RXCOM 81E6–822F (the shared region 8168–82EB, both banks), entered through the vector 7BAAH | no byte in the UART (DA) and none still to come from the feed (held bytes too) or the host link; or a line the peer must drive is off (the loop waits for CTS, CD, DSR too) | a byte; ON (82B3); error 58; error 50 (low battery) |
| PC-1600 with CE-1600P | the tape loader (CLOAD, MERGE, CHAIN; MODE 0 and MODE 1) | MODE 0: the first edge wait of CMBITIN, L6149–6152 (CE-1600P bank 5; no timeout while F1A9H bit 7 is set, the default); MODE 1: the edge waits of CM1500BIT, 62CD–62D8 and 62F7–6302 | the cassette does not play | an edge; BREAK (port 1BH bit 1) |
| PC-1600 | receiving on COM1: or COM2: (INPUT with `SETDEV "COM1:",KI`, LOAD"COM1:", INPUT#, COPY) | CRCVA (IOCS 04H, bank 6 9FEC): the loop 9FF4–A018 and what it calls (A02B–A084, 9FB1–9FBB, 9E93–9EA1, 9FD4–9FDB); OLD A034, the loop A03C–A060; the ring empty (read pointer F144H = write pointer F146H) | no byte in the ring or the CPC and none still to come from the feed (held bytes too) or the host link | a byte; BREAK (F150H bit 7); the RCVSTAT timeout (F14CH COM1, F14DH COM2; 0 = none, the default); an error |

| Case | A | W | settled | B |
|---|---|---|---|---|
| CLOAD with no cassette in, or the tape at its end | no | yes | yes | no |
| a tape that plays: the leader, the header, the data, and the silence a PC-1600 MODE 0 file has between header and data | no | no | no | no |
| INPUT from the port with nothing fed | no | yes | yes | no |
| LOAD"COM1:" while a feed still has bytes, held ones too | no | no | no | no |
| a direct LOAD"COM1:" with nothing fed | no | yes | yes | no |

Not counted as waiting for a medium:

- **Sending.** The PC-1600 waits before each byte for XOFF to end (F151H
  bit 3) and for the lines SNDSTAT asks for (COMSENDBYTE, bank 6 A503, the
  loop A509–A566); the CE-158 for CTS, CD and DSR (TXCOM 823A, the loop
  823E–8256). The same loops also run while a byte is still going out. The
  harness's other side and a host link keep their lines on, so only a fed
  XOFF can make the PC-1600 wait: work, as any send.
- **Reads that do not wait:** RXD$ (CRCV1, the PC-1600), RINKEY$ (RXCOM, the
  CE-158) return at once without data.
- **The CE-150 on the PC-1600** (MODE 1): its recorder runs and the tape
  moves, but no CLOAD or CSAVE through it works in the emulator yet (the
  CE-150's ROM receives the signal and decodes bits, the header is never
  found; a CSAVE writes no readable signal).

## In the code

- A: `PC1500Machine::keyReady()` and `PC1600Machine::keyReady()` (the frame
  notes and the 15 frames), `Machine::isKeyReady()` above the core.
- W's busy list and B: the kind `c1600.program-state`
  (`core/Machine/SharpProductKinds.hpp`: `PC1500Machine::toneSounding()`,
  `programEnded()`; `PC1600Machine::programEnded()`, the CE-1600F's motor).
- The rule: `app/SharpPresetRules.cpp`; the keys that do not wait:
  `app/SharpTyping.cpp` (ON/BREAK, at most 0.2 s). Pacing asks W, but full
  speed is never paced (`app/PacingProfiles.cpp`): with pacing by the host's
  calls, paced-while-waiting stops the Qt app answering (the DAP smoke test).
- The cases as tests: `tests/harness/idle_tests.cpp`.

## Not established

- PC-1500: whether 764E is changed by a BASIC statement; BREAKPARAM bit 4;
  CURR_LINE high byte FFH; how long ON must be held during CLOAD; whether a
  B008 of 0 means the CE-150's pen has stopped; TRACE not measured.
- PC-1600: every place that clears BUSY; a plotter case, the CE-1600F, S3:,
  COM1: and CAS: against these rules; the MODE 1 PRINT stop; a KEYGETR loop
  in machine code; the OLD ROM beyond the addresses named.
- Seen in the harness, cause not established: on the PC-1600, `type RUN`
  then `key enter` into a program whose first statement is INPUT made INPUT
  take an empty ENTER at once; with `key-down enter`, `wait 0.1s`,
  `key-up enter` it waits.
