# ASM overlay

An ASM overlay is a small program in the TMC0501 processor's own
instructions, which Calc-U-59 puts over the calculator's ROM at the
addresses 1800 to 1FFF and runs there. It is loaded in the box
**ASM OVERLAY** of the debug panel's CPU tab, or by dropping or opening a
file whose name ends in `.asm`; how to run it is in
[Debugger](07-debugger.md). Examples are on the site's
[software page](https://calc-u.org/59/software/).

The file is a UTF-8 text file in the ASM overlay format, version 2.0:

```
Calc-U-59 ASM 2.0
PROGRAM:
; human-readable assembly listing with comments
1800:   01D8    MOV     A.ALL,#0
        ...

HEX:
01D8 01DB ...   ; a comment after a semicolon
```

- Lines end with LF or CRLF. The first line is exactly
  `Calc-U-59 ASM 2.0`.
- Everything up to the line `HEX:` (any case, alone on its line) is for
  the reader and is not read: typically the annotated listing.
- After `HEX:`, a `;` starts a comment that runs to the end of its line.
- What remains is runs of hexadecimal digits, in either case, each
  optionally after `0x`, parted by spaces and line ends. A run of four
  digits is one instruction; a run of eight, twelve and so on is that
  many, four digits each.
- An instruction is a value from `0000` to `1FFF`. The first goes to the
  address `1800`, each further one to the next address; at most 2048
  fit.

A file is refused, with a notice that says why, when: its first line is
missing or names another version (a file written for Calc-U-59 2.3.0 has
no such line: add it, and check the text after `HEX:` against the rules
above); it has no `HEX:` line; nothing follows that line; a run of digits
is not a multiple of four long; an instruction is above `1FFF`; anything
after `HEX:` is neither hexadecimal digits nor a comment; or it has more
than 2048 instructions. An overlay loaded before stays.

## While an overlay is loaded

- **Run** brings the calculator to the end of its current program step
  and goes on at `1800`, with the processor's COND flag set. With the
  CPU's FREEZE ON START armed, it pauses there.
- While the overlay runs, the CALCULATOR tab cannot be chosen; FREEZE,
  STEP and RESUME of the CPU tab work on its instructions as on the
  ROM's.
- A Reset ends its run and keeps it loaded, so Run enters it again.
  **Clear** takes it away (and resets the calculator when it was
  running); a Cold boot, a preset or a model switch removes it too. It is
  not kept when the product quits.
