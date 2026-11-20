# Idle on the PC-1500, PC-1500A and PC-1600

When the machine takes a key, waits, has settled, or has finished: the
signals, and how each question is answered from them. Addresses are those
of the annotated disassemblies (research skill `PC1500`): the PC-1500 A04
(the same in A01 and A03 for every routine named here), the PC-1600 NEW
ROM (OLD where it differs). "Measured" means with the harness.

| Question | Meaning | Asked by |
|---|---|---|
| A, takes keys | the ROM or a program reads the keyboard as input now; watching for BREAK alone is not A | a scripted press before it goes down |
| W, waits | nothing happens until the user acts: A, and nothing is busy | pacing (full speed runs at real time while W holds) |
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

- **A** = not off, and (the key wait, or a key read for INKEY$ or TRACE in
  this frame).
- **W** = A, and none of: the buzzer line changed in this frame, the
  CE-150's motor phases not 0.
- **B** = the key wait, BUSY off, BREAKPARAM bits 5 and 6 clear, settled.

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
| CLOAD waiting for the tape | no | no | no | no | LOAD_NIBBLE BE25, BUSY on: a medium is awaited, but no medium place reports it |
| switched off | no | no | no | no | powered off |

### Keys that do not wait for A

| Key | How the ROM sees it | Hold |
|---|---|---|
| ON/BREAK | latched: F00B bit 1, tested once per statement (C42A), in each key-wait pass (E269), in TIME_DELAY per 15.625 ms tick (E8A3, E8B0), in BEEP's tone loop each half period (E683), in the CE-150's BRK_150 A746 (after each character, GRAPH step and feed step) and SAVE_NIBBLE BDD6 | any: a 1 ms press broke `10 GOTO 10` (measured). During CLOAD the loader reads the PB7 level instead (BE2F, BE4C): held while it polls; how long is not established |

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

- **A** = the system on, CK0 on, and (the key wait, or a key read in this
  frame).
- **W** = A, and none of: the CE-1600F's motor on, the CE-1600P's motor
  power on.
- **B** = the key wait, BUSY off, CURLINE 0, INTFLG bits 5 and 6 clear,
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
| a direct LOAD, SAVE, COPY (S1:, S2:, S3:, COM1:, X:) | no | no | no | no | no key wait (the serial waits P2-B6 A014, A503 test only the BREAK latch) |
| plotting, printing on the CE-1600P | no | no | no | no | no key wait; at the prompt with the motor power still on: A, not W |
| the CE-1600F's motor on at the prompt | yes | no | no | no | motor |
| switched off, auto power-off | no | no | no | no | system off |

### Keys that do not wait for A

| Key | How the ROM sees it | Hold |
|---|---|---|
| ON/BREAK | latched: the IF latch, port 1BH bit 1 (rising edge of PB7), tested before every statement (STMTCHK 3AC0 → BREAKHIT 3C8D; OLD 3B65), in the editor loop (6862), in BEEP, the plotter (PBREAKCHK, CE-1600P bank 4 5788), the floppy (FDFILEEXIT, bank 5 42C9), the cassette and the serial loops; the level, port 1FH bit 7, by KEYMATRIX and WAITONUP (0775) | any for the latch; held while a routine waits for the release (WAITONUP) |

OFF is not one of them: a matrix key handled only by the editor (EDOFF
P1-B0 6B3F; ignored during a program's INPUT wait and a PRINT stop with
F0BBH bit 0).

## Today

- **PC-1500:** `PC1500Machine::keyReady()` is the key wait alone: an INKEY$
  program is never A (measured: `wait` runs the hour). The idle rule is
  settled on it. `key break` waits for A and is refused after an hour
  (measured with `10 GOTO 10`); `key-down break` works.
- **PC-1600:** a frame is ready when CK0 is on and the SC7852 is in the
  command loop (9280–9300) or the key matrix was read: the interrupt's
  scan makes that true while any program runs, so `10 GOTO 10` counts as
  idle. The idle rule then excludes the CE-1600F's motor and a direct file
  command (CURLINE 0, not in KEYGET, bank 3b or DTR); with A from the key
  wait and the key reads, that exclusion is no longer needed. The comment
  in `app/SharpPresetRules.cpp` says BASIC reads the matrix for BREAK; it is
  the interrupt.
- Both: no B; no W beyond the floppy motor.

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
