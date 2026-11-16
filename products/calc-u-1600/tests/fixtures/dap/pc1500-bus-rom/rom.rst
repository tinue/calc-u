                              1 ; rom.asm -- RENUM for the PC-1500: a ROM extension (LH5801,
                              2 ; sdaslh5801) that adds a BASIC command, plugged in at &8800 by
                              3 ; debug.pc1500a's bus-rom:.
                              4 ;
                              5 ; Debugging it: F5 assembles the ROM, starts the machine with it, enters
                              6 ; debug.pc1500a's sample program and types its `command:` (RENUM 100,,10).
                              7 ; Set a breakpoint first, on RENUM, RENSCAN, RENFIX or RENNUMBER, or on
                              8 ; any line below. Build & Load (Cmd+Alt+L / Ctrl+Alt+L) rebuilds it and
                              9 ; sets the machine up again. `boot: debug` in debug.pc1500a stops at INIT
                             10 ; instead, the code the firmware calls at power-on.
                             11 ;
                             12 ;   RENUM [new][,[old][,step]]
                             13 ;
                             14 ; renumbers the program in memory, as the PC-1600's built-in RENUM does:
                             15 ; the lines from line `old` on (default: the first line) get the numbers
                             16 ; new, new+step, ... (defaults 10 and 10), and every GOTO, GOSUB, THEN,
                             17 ; ON...GOTO/GOSUB and RESTORE that names one of them follows. RENUM works
                             18 ; in RUN and PRO mode; inside a program it is ERROR 1. On any error the
                             19 ; program is left as it was:
                             20 ;   ERROR 19  new or step is 0 or above 65279, or the new numbers would
                             21 ;             reach the line before `old`
                             22 ;   ERROR 11  line `old` doesn't exist
                             23 ;   ERROR 30  the last new number would be above 65279
                             24 ;   ERROR 13  the longer line numbers don't fit in memory
                             25 ;   ERROR 11 IN n  line n jumps to a line that doesn't exist
                             26 ;   ERROR 1 IN n   line n jumps to a computed line (GOTO A, GOTO 100+X)
                             27 ; RENUM refuses a merged program (MERGE) with ERROR 1.
                             28 ;
                             29 ; How the firmware finds the command
                             30 ; ----------------------------------
                             31 ; At power-on and for every word it doesn't know, the PC-1500 ROM looks at
                             32 ; the 2 KB pages &8000-&B800, with PV low and high, for a keyword module
                             33 ; (ROM A04: TOK_TABL_SRCH E4A8, the name lookup at F9C7, TOK_PROCESS FA89):
                             34 ;   +00   55H, the module's sentinel
                             35 ;   +0A   code the ROM calls at power-on (RESET_19 E10B), ending in RTN
                             36 ;   +20   an index: one word per letter A-Z, the address of the 2nd letter
                             37 ;         of the first keyword with that letter, or 0
                             38 ;   +54   the keyword table, entries of
                             39 ;         marker, name, code (2 bytes), address (2 bytes)
                             40 ;         - the marker's low nibble is the name's length, 0 ends the table;
                             41 ;         - its high nibble describes the keyword BEFORE it (as in the
                             42 ;           ROM's own table at C054, CNIB macro): bit 4 = last keyword
                             43 ;           with that letter, bits 6-5 = where it may run: 00 typed only
                             44 ;           (the routine's first byte is then a mask of the allowed
                             45 ;           modes, ANDed with 764FH: 40H RUN, 20H PRO, 10H RESERVE, and
                             46 ;           execution starts after it), 01 in a program only, 1x both;
                             47 ;         - the code's high byte E0H-EFH names the page: 80H+(code&7)*8,
                             48 ;           PV high for E8H-EFH; a low byte of 80H or more is a statement.
                             49 ; When the command runs, Y points just past its token at the rest of the
                             50 ; statement, and the routine ends with VEJ (E2), the ROM's "next statement".
                             51 ;
                             52 ; How RENUM works
                             53 ; ---------------
                             54 ; The algorithm follows the PC-1600's RENUM (ROM P1-B3B, K_RENUM 45BDH):
                             55 ; check the arguments, collect the references, renumber, fix the
                             56 ; references. The PC-1600 keeps a line reference in binary and patches it
                             57 ; in place; the PC-1500 keeps it as decimal digits, so a reference can get
                             58 ; longer or shorter and the rest of the program moves. Hence two passes
                             59 ; over the references:
                             60 ;   1. RENSCAN (dry run): find every reference and its target, report the
                             61 ;      first bad one, add up how much the program grows;
                             62 ;   2. RENFIX: rewrite the references, moving the program as needed;
                             63 ; then RENNUMBER writes the new line numbers. The references are fixed
                             64 ; while the lines still have their old numbers, so finding a target is a
                             65 ; plain search; no table is needed.
                             66 ;
                             67 ; A program line is: line number (2 bytes, high first), length (the bytes
                             68 ; that follow, up to and including the 0DH), the tokenized text, 0DH. The
                             69 ; program ends with FFH, at BASPRG_END.
                             70 
                             71 ; ── ROM addresses (the same in A01, A03 and A04) ──────────────────────
                             72 
                     7864    73 RAM_END_H   .equ    0x7864      ; first page past the RAM
                     7865    74 BASPRG_ST   .equ    0x7865      ; program start
                     7867    75 BASPRG_END  .equ    0x7867      ; program end (its FFH)
                     7869    76 BASPRG_EDT  .equ    0x7869      ; start of the last MERGEd program
                     789C    77 CURR_LINE   .equ    0x789C      ; line number being executed (0: none)
                     789E    78 CURR_TOP    .equ    0x789E      ; start of the program it is in
                     FFFFCFD0    79 INIT_SYS    .equ    0xCFD0      ; reset DATA, GOSUB/FOR, ON ERROR, CONT
                     FFFFD09C    80 DEL_DIM_VAR .equ    0xD09C      ; drop arrays the program has grown into
                             81 
                     0092    82 TOK_GOTO    .equ    0x92        ; F1xxH tokens that take line numbers
                     0094    83 TOK_GOSUB   .equ    0x94
                     00A7    84 TOK_RESTORE .equ    0xA7
                     00AB    85 TOK_REM     .equ    0xAB
                     00AE    86 TOK_THEN    .equ    0xAE
                             87 
                             88 ; ── Work variables: the string buffer (7B10H-7B5FH), free while RENUM
                             89 ;    runs; 16-bit values high byte first, like the ROM's ───────────────
                             90 
                     7B10    91 W           .equ    0x7B10
                     7B10    92 R_NEW       .equ    W+0         ; first new line number
                     7B12    93 R_OLD       .equ    W+2         ; first line to renumber (0: the first)
                     7B14    94 R_STEP      .equ    W+4         ; increment
                     7B16    95 R_OADR      .equ    W+6         ; address of line `old`
                     7B18    96 R_PREV      .equ    W+8         ; number of the line before it (0: none)
                     7B1A    97 R_VAL       .equ    W+10        ; running new number
                     7B1C    98 R_LINE      .equ    W+12        ; line being scanned
                     7B1E    99 R_PTR       .equ    W+14        ; start of the reference's digits
                     7B20   100 R_END       .equ    W+16        ; just past them
                     7B22   101 R_GROW      .equ    W+18        ; how much the program grows (signed)
                     7B24   102 R_T         .equ    W+20        ; scratch word
                     7B26   103 R_Y         .equ    W+22        ; the statement pointer to return with
                     7B28   104 R_PASS      .equ    W+24        ; 0 dry run, 1 fix
                     7B29   105 R_INR       .equ    W+25        ; NEWNUM: in the renumbered range
                     7B2A   106 R_OVF       .equ    W+26        ; reference too big to be a line number
                     7B2B   107 R_OLEN      .equ    W+27        ; digits in the reference now
                     7B2C   108 R_NLEN      .equ    W+28        ; digits it will have
                     7B2D   109 R_DLT       .equ    W+29        ; R_NLEN - R_OLEN
                     7B2E   110 R_DGT       .equ    W+30        ; TODEC: current digit
                     7B2F   111 R_LEAD      .equ    W+31        ; TODEC: a digit has been written
                     7B30   112 R_CNT       .equ    W+32        ; TODEC: powers left
                     7B31   113 R_DIGIT     .equ    W+33        ; parse: digit being added
                     7B32   114 R_DIG       .equ    W+34        ; the new digits (5)
                            115 
                            116             .area   CODE (ABS)
                            117 
                            118 ; ── The module header ─────────────────────────────────────────────────
                            119 
   FFFF8800                     120             .org    0x8800
   FFFF8800 55                  121             .db     0x55        ; sentinel: a keyword module lives here
                            122 
   FFFF880A                     123             .org    0x880A
   FFFF880A 9A                  124 INIT:       rtn                 ; nothing to set up at power-on
                            125 
   FFFF8820                     126             .org    0x8820      ; the index, A-Z
   FFFF8820 00 00 00 00 00 00   127             .dw     0, 0, 0, 0, 0, 0, 0, 0, 0       ; A-I
        00 00 00 00 00 00
        00 00 00 00 00 00
   FFFF8832 00 00 00 00 00 00   128             .dw     0, 0, 0, 0, 0, 0, 0, 0          ; J-Q
        00 00 00 00 00 00
        00 00 00 00
   FFFF8842r88r56               129             .dw     KW_RENUM+2                      ; R: the E of RENUM
   FFFF8844 00 00 00 00 00 00   130             .dw     0, 0, 0, 0, 0, 0, 0, 0          ; S-Z
        00 00 00 00 00 00
        00 00 00 00
                            131 
   FFFF8854                     132             .org    0x8854      ; the keyword table
   FFFF8854 C5                  133 KW_RENUM:   .db     0xC5        ; length 5 (high nibble: no keyword before)
   FFFF8855 52 45 4E 55 4D      134             .ascii  "RENUM"
   FFFF885A E1 80               135             .dw     0xE180      ; code: page 8800H (E1), PV low; a statement
   FFFF885Cr88r5F               136             .dw     RENUM_MODES ; address
   FFFF885E 90                  137             .db     0x90        ; end of table; for RENUM: last R keyword,
                            138                                 ; typed only (bits 6-5 = 00)
                            139 
                            140 ; ── RENUM ─────────────────────────────────────────────────────────────
                            141 
   FFFF885F 60                  142 RENUM_MODES: .db    0x60        ; allowed in RUN and PRO mode; RENUM follows
                            143 
                            144 ; Arguments: [new][,[old][,step]], each an expression. The values go on
                            145 ; the stack; the work variables are only safe once no expression is
                            146 ; being evaluated.
   FFFF8860 68 00               147 RENUM:      ldi     uh,0
   FFFF8862 6A 0A               148             ldi     ul,10
   FFFF8864 BEr89rAE            149             sjp     GETNUM      ; new
   FFFF8867 FD A8               150             psh     u
   FFFF8869 68 00               151             ldi     uh,0
   FFFF886B 6A 00               152             ldi     ul,0
   FFFF886D 15                  153             lda     (y)
   FFFF886E B7 2C               154             cpi     a,0x2C      ; ','
   FFFF8870 89 04               155             bzr     ARG_OLD
   FFFF8872 54                  156             inc     y
   FFFF8873 BEr89rAE            157             sjp     GETNUM      ; old
   FFFF8876 FD A8               158 ARG_OLD:    psh     u
   FFFF8878 68 00               159             ldi     uh,0
   FFFF887A 6A 0A               160             ldi     ul,10
   FFFF887C 15                  161             lda     (y)
   FFFF887D B7 2C               162             cpi     a,0x2C
   FFFF887F 89 04               163             bzr     ARG_STEP
   FFFF8881 54                  164             inc     y
   FFFF8882 BEr89rAE            165             sjp     GETNUM      ; step
   FFFF8885 FD A8               166 ARG_STEP:   psh     u
   FFFF8887 15                  167             lda     (y)
   FFFF8888 B7 0D               168             cpi     a,0x0D      ; nothing may follow
   FFFF888A 8B 01               169             bzs     ARG_DONE
   FFFF888C E4                  170             vej     0xE4        ; ERROR 1
   FFFF888D FD 2A               171 ARG_DONE:   pop     u
   FFFF888F A4                  172             lda     uh
   FFFF8890 AE 7B 14            173             sta     (R_STEP)
   FFFF8893 24                  174             lda     ul
   FFFF8894 AE 7B 15            175             sta     (R_STEP+1)
   FFFF8897 FD 2A               176             pop     u
   FFFF8899 A4                  177             lda     uh
   FFFF889A AE 7B 12            178             sta     (R_OLD)
   FFFF889D 24                  179             lda     ul
   FFFF889E AE 7B 13            180             sta     (R_OLD+1)
   FFFF88A1 FD 2A               181             pop     u
   FFFF88A3 A4                  182             lda     uh
   FFFF88A4 AE 7B 10            183             sta     (R_NEW)
   FFFF88A7 24                  184             lda     ul
   FFFF88A8 AE 7B 11            185             sta     (R_NEW+1)
   FFFF88AB 94                  186             lda     yh
   FFFF88AC AE 7B 26            187             sta     (R_Y)
   FFFF88AF 14                  188             lda     yl
   FFFF88B0 AE 7B 27            189             sta     (R_Y+1)
                            190 
                            191 ; new and step: 1..65279
   FFFF88B3 68 13               192             ldi     uh,19
   FFFF88B5 A5 7B 10            193             lda     (R_NEW)
   FFFF88B8 B7 FF               194             cpi     a,0xFF
   FFFF88BA 8B 2E               195             bzs     ERROR
   FFFF88BC AB 7B 11            196             ora     (R_NEW+1)
   FFFF88BF 8B 29               197             bzs     ERROR
   FFFF88C1 A5 7B 14            198             lda     (R_STEP)
   FFFF88C4 B7 FF               199             cpi     a,0xFF
   FFFF88C6 8B 22               200             bzs     ERROR
   FFFF88C8 AB 7B 15            201             ora     (R_STEP+1)
   FFFF88CB 8B 1D               202             bzs     ERROR
                            203 
                            204 ; One program only: after a MERGE, BASPRG_EDT points at the last one.
   FFFF88CD 68 01               205             ldi     uh,1
   FFFF88CF A5 78 69            206             lda     (BASPRG_EDT)
   FFFF88D2 A7 78 65            207             cpa     (BASPRG_ST)
   FFFF88D5 89 13               208             bzr     ERROR
   FFFF88D7 A5 78 6A            209             lda     (BASPRG_EDT+1)
   FFFF88DA A7 78 66            210             cpa     (BASPRG_ST+1)
   FFFF88DD 89 0B               211             bzr     ERROR
                            212 
                            213 ; An empty program: nothing to do.
   FFFF88DF BEr89rBE            214             sjp     PRGSTART
   FFFF88E2 05                  215             lda     (x)
   FFFF88E3 B7 FF               216             cpi     a,0xFF
   FFFF88E5 89 04               217             bzr     FINDOLD
   FFFF88E7 BAr89rA5            218             jmp     FINISH
                            219 
   FFFF88EA E0                  220 ERROR:      vej     0xE0        ; ERROR UH
                            221 
                            222 ; Find line `old` (R_OADR) and the number of the line before it (R_PREV).
   FFFF88EB B5 00               223 FINDOLD:    ldi     a,0
   FFFF88ED AE 7B 18            224             sta     (R_PREV)
   FFFF88F0 AE 7B 19            225             sta     (R_PREV+1)
   FFFF88F3 A5 7B 12            226             lda     (R_OLD)
   FFFF88F6 AB 7B 13            227             ora     (R_OLD+1)
   FFFF88F9 8B 25               228             bzs     FO_FOUND    ; old = 0: the first line
   FFFF88FB 05                  229 FO_LOOP:    lda     (x)
   FFFF88FC B7 FF               230             cpi     a,0xFF
   FFFF88FE 8B 1C               231             bzs     FO_MISS
   FFFF8900 A7 7B 12            232             cpa     (R_OLD)
   FFFF8903 89 08               233             bzr     FO_NEXT
   FFFF8905 44                  234             inc     x
   FFFF8906 05                  235             lda     (x)
   FFFF8907 46                  236             dec     x
   FFFF8908 A7 7B 13            237             cpa     (R_OLD+1)
   FFFF890B 8B 13               238             bzs     FO_FOUND
   FFFF890D 05                  239 FO_NEXT:    lda     (x)
   FFFF890E AE 7B 18            240             sta     (R_PREV)
   FFFF8911 44                  241             inc     x
   FFFF8912 05                  242             lda     (x)
   FFFF8913 AE 7B 19            243             sta     (R_PREV+1)
   FFFF8916 46                  244             dec     x
   FFFF8917 BEr89rC7            245             sjp     NEXTLINE
   FFFF891A 9E 21               246             bch     FO_LOOP
   FFFF891C 68 0B               247 FO_MISS:    ldi     uh,11
   FFFF891E 9E 36               248             bch     ERROR
   FFFF8920 84                  249 FO_FOUND:   lda     xh
   FFFF8921 AE 7B 16            250             sta     (R_OADR)
   FFFF8924 04                  251             lda     xl
   FFFF8925 AE 7B 17            252             sta     (R_OADR+1)
                            253 
                            254 ; The new numbers must stay above the line before `old` ...
   FFFF8928 68 13               255             ldi     uh,19
   FFFF892A FB                  256             sec
   FFFF892B A5 7B 19            257             lda     (R_PREV+1)
   FFFF892E A1 7B 11            258             sbc     (R_NEW+1)
   FFFF8931 A5 7B 18            259             lda     (R_PREV)
   FFFF8934 A1 7B 10            260             sbc     (R_NEW)
   FFFF8937 93 4F               261             bcs     ERROR       ; prev >= new
                            262 
                            263 ; ... and below 65280.
   FFFF8939 68 1E               264             ldi     uh,30
   FFFF893B BEr89rCD            265             sjp     STARTVAL
   FFFF893E BEr89rC7            266 CR_LOOP:    sjp     NEXTLINE
   FFFF8941 05                  267             lda     (x)
   FFFF8942 B7 FF               268             cpi     a,0xFF
   FFFF8944 8B 0E               269             bzs     CR_DONE
   FFFF8946 BEr89rE2            270             sjp     STEPVAL
   FFFF8949 93 61               271             bcs     ERROR       ; carry out of 16 bits
   FFFF894B A5 7B 1A            272             lda     (R_VAL)
   FFFF894E B7 FF               273             cpi     a,0xFF
   FFFF8950 9B 68               274             bzs     ERROR
   FFFF8952 9E 16               275             bch     CR_LOOP
   FFFF8954                     276 CR_DONE:
                            277 
                            278 ; Pass 1, dry run: check every reference, add up the growth.
   FFFF8954 B5 00               279 RENSCAN:    ldi     a,0
   FFFF8956 AE 7B 28            280             sta     (R_PASS)
   FFFF8959 AE 7B 22            281             sta     (R_GROW)
   FFFF895C AE 7B 23            282             sta     (R_GROW+1)
   FFFF895F BEr89rF6            283             sjp     SCANREFS
                            284 
                            285 ; Room for the growth? Like the ROM's own line insert (PRGLINE_TDI CF27):
                            286 ; the new end must stay below RAM_END_H.
   FFFF8962 A5 7B 22            287             lda     (R_GROW)
   FFFF8965 BF 80               288             bii     a,0x80
   FFFF8967 89 16               289             bzr     MEM_OK      ; the program shrinks
   FFFF8969 F9                  290             rec
   FFFF896A A5 78 68            291             lda     (BASPRG_END+1)
   FFFF896D A3 7B 23            292             adc     (R_GROW+1)
   FFFF8970 A5 78 67            293             lda     (BASPRG_END)
   FFFF8973 A3 7B 22            294             adc     (R_GROW)
   FFFF8976 68 0D               295             ldi     uh,13
   FFFF8978 93 90               296             bcs     ERROR
   FFFF897A A7 78 64            297             cpa     (RAM_END_H)
   FFFF897D 93 95               298             bcs     ERROR
   FFFF897F                     299 MEM_OK:
                            300 
                            301 ; Pass 2: rewrite the references.
   FFFF897F B5 01               302 RENFIX:     ldi     a,1
   FFFF8981 AE 7B 28            303             sta     (R_PASS)
   FFFF8984 BEr89rF6            304             sjp     SCANREFS
                            305 
                            306 ; The new line numbers, from line `old` on.
   FFFF8987 BEr89rCD            307 RENNUMBER:  sjp     STARTVAL
   FFFF898A 05                  308 RN_LOOP:    lda     (x)
   FFFF898B B7 FF               309             cpi     a,0xFF
   FFFF898D 8B 10               310             bzs     RN_DONE
   FFFF898F A5 7B 1A            311             lda     (R_VAL)
   FFFF8992 41                  312             sin     x
   FFFF8993 A5 7B 1B            313             lda     (R_VAL+1)
   FFFF8996 41                  314             sin     x
   FFFF8997 BEr89rC9            315             sjp     NEXTLEN
   FFFF899A BEr89rE2            316             sjp     STEPVAL
   FFFF899D 9E 15               317             bch     RN_LOOP
   FFFF899F                     318 RN_DONE:
   FFFF899F BE D0 9C            319             sjp     DEL_DIM_VAR ; as after a line edit
   FFFF89A2 BE CF D0            320             sjp     INIT_SYS    ; nothing may point into the old program
                            321 
   FFFF89A5 A5 7B 26            322 FINISH:     lda     (R_Y)
   FFFF89A8 18                  323             sta     yh
   FFFF89A9 A5 7B 27            324             lda     (R_Y+1)
   FFFF89AC 1A                  325             sta     yl
   FFFF89AD E2                  326             vej     0xE2        ; end of statement
                            327 
                            328 ; ── Argument parsing ──────────────────────────────────────────────────
                            329 
                            330 ; GETNUM: U = the number at Y, or U unchanged (the default) if the
                            331 ; argument is left out (Y at ',' or the end).
   FFFF89AE 15                  332 GETNUM:     lda     (y)
   FFFF89AF B7 2C               333             cpi     a,0x2C
   FFFF89B1 8B 09               334             bzs     GN_RET
   FFFF89B3 B7 0D               335             cpi     a,0x0D
   FFFF89B5 8B 05               336             bzs     GN_RET
   FFFF89B7 DE                  337             vej     0xDE        ; evaluate the expression at Y
   FFFF89B8 04                  338             .db     GN_ERR-.-1
   FFFF89B9 D0                  339             vej     0xD0        ; to a 16-bit integer in U
   FFFF89BA 00                  340             .db     0x00
   FFFF89BB 01                  341             .db     GN_ERR-.-1
   FFFF89BC 9A                  342 GN_RET:     rtn
   FFFF89BD E0                  343 GN_ERR:     vej     0xE0
                            344 
                            345 ; ── Walking the program ───────────────────────────────────────────────
                            346 
                            347 ; PRGSTART: X = the first line.
   FFFF89BE A5 78 65            348 PRGSTART:   lda     (BASPRG_ST)
   FFFF89C1 08                  349             sta     xh
   FFFF89C2 A5 78 66            350             lda     (BASPRG_ST+1)
   FFFF89C5 0A                  351             sta     xl
   FFFF89C6 9A                  352             rtn
                            353 
                            354 ; NEXTLINE: X from a line's start to the next line's.
   FFFF89C7 44                  355 NEXTLINE:   inc     x
   FFFF89C8 44                  356             inc     x
                            357 ; NEXTLEN: the same from the line's length byte.
   FFFF89C9 45                  358 NEXTLEN:    lin     x
   FFFF89CA FD CA               359             adr     x
   FFFF89CC 9A                  360             rtn
                            361 
                            362 ; STARTVAL: X = line `old`, R_VAL = new.
   FFFF89CD A5 7B 16            363 STARTVAL:   lda     (R_OADR)
   FFFF89D0 08                  364             sta     xh
   FFFF89D1 A5 7B 17            365             lda     (R_OADR+1)
   FFFF89D4 0A                  366             sta     xl
   FFFF89D5 A5 7B 10            367             lda     (R_NEW)
   FFFF89D8 AE 7B 1A            368             sta     (R_VAL)
   FFFF89DB A5 7B 11            369             lda     (R_NEW+1)
   FFFF89DE AE 7B 1B            370             sta     (R_VAL+1)
   FFFF89E1 9A                  371             rtn
                            372 
                            373 ; STEPVAL: R_VAL += step; C = carry out.
   FFFF89E2 F9                  374 STEPVAL:    rec
   FFFF89E3 A5 7B 1B            375             lda     (R_VAL+1)
   FFFF89E6 A3 7B 15            376             adc     (R_STEP+1)
   FFFF89E9 AE 7B 1B            377             sta     (R_VAL+1)
   FFFF89EC A5 7B 1A            378             lda     (R_VAL)
   FFFF89EF A3 7B 14            379             adc     (R_STEP)
   FFFF89F2 AE 7B 1A            380             sta     (R_VAL)
   FFFF89F5 9A                  381             rtn
                            382 
                            383 ; ── Every line reference in the program (RENSCAN, RENFIX) ─────────────
                            384 
   FFFF89F6 BEr89rBE            385 SCANREFS:   sjp     PRGSTART
   FFFF89F9 05                  386 SC_LINE:    lda     (x)
   FFFF89FA B7 FF               387             cpi     a,0xFF
   FFFF89FC 89 01               388             bzr     SC_BODY
   FFFF89FE 9A                  389             rtn
   FFFF89FF 84                  390 SC_BODY:    lda     xh
   FFFF8A00 AE 7B 1C            391             sta     (R_LINE)
   FFFF8A03 04                  392             lda     xl
   FFFF8A04 AE 7B 1D            393             sta     (R_LINE+1)
   FFFF8A07 44                  394             inc     x
   FFFF8A08 44                  395             inc     x
   FFFF8A09 44                  396             inc     x
   FFFF8A0A 05                  397 SC_ELEM:    lda     (x)
   FFFF8A0B B7 0D               398             cpi     a,0x0D
   FFFF8A0D 8B 40               399             bzs     SC_NEXT
   FFFF8A0F B7 22               400             cpi     a,0x22      ; '"': skip the string
   FFFF8A11 89 05               401             bzr     SC_NOSTR
   FFFF8A13 BEr8Ar52            402             sjp     SKIPSTR
   FFFF8A16 9E 0E               403             bch     SC_ELEM
   FFFF8A18 B7 E0               404 SC_NOSTR:   cpi     a,0xE0
   FFFF8A1A 83 03               405             bcs     SC_TOKEN
   FFFF8A1C 44                  406             inc     x           ; a character
   FFFF8A1D 9E 15               407             bch     SC_ELEM
   FFFF8A1F 44                  408 SC_TOKEN:   inc     x
   FFFF8A20 B7 F1               409             cpi     a,0xF1      ; BASIC's own statements are F1xxH
   FFFF8A22 8B 03               410             bzs     SC_F1
   FFFF8A24 44                  411             inc     x           ; another token: skip its low byte
   FFFF8A25 9E 1D               412             bch     SC_ELEM
   FFFF8A27 45                  413 SC_F1:      lin     x           ; A = the token's low byte
   FFFF8A28 B7 AB               414             cpi     a,TOK_REM   ; REM: the rest of the line is text
   FFFF8A2A 8B 1B               415             bzs     SC_REM
   FFFF8A2C B7 92               416             cpi     a,TOK_GOTO
   FFFF8A2E 8B 12               417             bzs     SC_REFS
   FFFF8A30 B7 94               418             cpi     a,TOK_GOSUB
   FFFF8A32 8B 0E               419             bzs     SC_REFS
   FFFF8A34 B7 A7               420             cpi     a,TOK_RESTORE
   FFFF8A36 8B 0A               421             bzs     SC_REFS
   FFFF8A38 B7 AE               422             cpi     a,TOK_THEN  ; THEN 100, but not THEN PRINT
   FFFF8A3A 99 32               423             bzr     SC_ELEM
   FFFF8A3C 05                  424             lda     (x)
   FFFF8A3D BEr8Ar5E            425             sjp     ISDIGIT
   FFFF8A40 91 38               426             bcr     SC_ELEM
   FFFF8A42 BEr8Ar67            427 SC_REFS:    sjp     REFLIST
   FFFF8A45 9E 3D               428             bch     SC_ELEM
   FFFF8A47 05                  429 SC_REM:     lda     (x)
   FFFF8A48 B7 0D               430             cpi     a,0x0D
   FFFF8A4A 8B 03               431             bzs     SC_NEXT
   FFFF8A4C 44                  432             inc     x
   FFFF8A4D 9E 08               433             bch     SC_REM
   FFFF8A4F 44                  434 SC_NEXT:    inc     x           ; past the 0DH: the next line
   FFFF8A50 9E 59               435             bch     SC_LINE
                            436 
                            437 ; SKIPSTR: X from a '"' to just past the closing one (or to the 0DH).
   FFFF8A52 44                  438 SKIPSTR:    inc     x
   FFFF8A53 05                  439 SS_LOOP:    lda     (x)
   FFFF8A54 B7 0D               440             cpi     a,0x0D
   FFFF8A56 8B 05               441             bzs     SS_RET
   FFFF8A58 44                  442             inc     x
   FFFF8A59 B7 22               443             cpi     a,0x22
   FFFF8A5B 99 0A               444             bzr     SS_LOOP
   FFFF8A5D 9A                  445 SS_RET:     rtn
                            446 
                            447 ; ISDIGIT: C = 1 if A is '0'-'9'. A is kept.
   FFFF8A5E B7 3A               448 ISDIGIT:    cpi     a,0x3A
   FFFF8A60 83 03               449             bcs     ISD_NO
   FFFF8A62 B7 30               450             cpi     a,0x30
   FFFF8A64 9A                  451             rtn
   FFFF8A65 F9                  452 ISD_NO:     rec
   FFFF8A66 9A                  453             rtn
                            454 
                            455 ; REFLIST: the references after GOTO/GOSUB/RESTORE/THEN: a number or a
                            456 ; label ("A"), several for ON ... GOTO, or none (RESTORE).
   FFFF8A67 05                  457 REFLIST:    lda     (x)
   FFFF8A68 B7 22               458             cpi     a,0x22
   FFFF8A6A 89 05               459             bzr     RL_NUM
   FFFF8A6C BEr8Ar52            460             sjp     SKIPSTR     ; a label: leave it
   FFFF8A6F 8E 08               461             bch     RL_SEP
   FFFF8A71 BEr8Ar5E            462 RL_NUM:     sjp     ISDIGIT
   FFFF8A74 81 0B               463             bcr     RL_END      ; no number here
   FFFF8A76 BEr8Ar8F            464             sjp     DOREF
   FFFF8A79 05                  465 RL_SEP:     lda     (x)
   FFFF8A7A B7 2C               466             cpi     a,0x2C
   FFFF8A7C 89 03               467             bzr     RL_END
   FFFF8A7E 44                  468             inc     x
   FFFF8A7F 9E 1A               469             bch     REFLIST
   FFFF8A81 B7 0D               470 RL_END:     cpi     a,0x0D      ; the statement must end here
   FFFF8A83 8B 09               471             bzs     RL_RET
   FFFF8A85 B7 3A               472             cpi     a,0x3A      ; ':'
   FFFF8A87 8B 05               473             bzs     RL_RET
   FFFF8A89 68 01               474             ldi     uh,1        ; anything else is a computed line
   FFFF8A8B BAr8Cr97            475             jmp     ERR_IN
   FFFF8A8E 9A                  476 RL_RET:     rtn
                            477 
                            478 ; DOREF: the reference at X. Dry run: look it up, add the growth.
                            479 ; Fix: rewrite it. Returns X past it.
   FFFF8A8F 84                  480 DOREF:      lda     xh
   FFFF8A90 AE 7B 1E            481             sta     (R_PTR)
   FFFF8A93 04                  482             lda     xl
   FFFF8A94 AE 7B 1F            483             sta     (R_PTR+1)
   FFFF8A97 B5 00               484             ldi     a,0
   FFFF8A99 AE 7B 2A            485             sta     (R_OVF)
   FFFF8A9C AE 7B 2B            486             sta     (R_OLEN)
   FFFF8A9F 68 00               487             ldi     uh,0
   FFFF8AA1 6A 00               488             ldi     ul,0
   FFFF8AA3 05                  489 DR_DIGIT:   lda     (x)
   FFFF8AA4 BEr8Ar5E            490             sjp     ISDIGIT
   FFFF8AA7 81 12               491             bcr     DR_GOT
   FFFF8AA9 B9 0F               492             ani     a,0x0F
   FFFF8AAB AE 7B 31            493             sta     (R_DIGIT)
   FFFF8AAE BEr8Br16            494             sjp     MUL10ADD
   FFFF8AB1 44                  495             inc     x
   FFFF8AB2 A5 7B 2B            496             lda     (R_OLEN)
   FFFF8AB5 DD                  497             inc     a
   FFFF8AB6 AE 7B 2B            498             sta     (R_OLEN)
   FFFF8AB9 9E 18               499             bch     DR_DIGIT
   FFFF8ABB 84                  500 DR_GOT:     lda     xh
   FFFF8ABC AE 7B 20            501             sta     (R_END)
   FFFF8ABF 04                  502             lda     xl
   FFFF8AC0 AE 7B 21            503             sta     (R_END+1)
   FFFF8AC3 A5 7B 2A            504             lda     (R_OVF)
   FFFF8AC6 89 2A               505             bzr     DR_UNDEF
   FFFF8AC8 BEr8Br4F            506             sjp     NEWNUM
   FFFF8ACB 81 25               507             bcr     DR_UNDEF
   FFFF8ACD BEr8BrA0            508             sjp     TODEC
   FFFF8AD0 FB                  509             sec
   FFFF8AD1 A5 7B 2C            510             lda     (R_NLEN)
   FFFF8AD4 A1 7B 2B            511             sbc     (R_OLEN)
   FFFF8AD7 AE 7B 2D            512             sta     (R_DLT)
   FFFF8ADA A5 7B 28            513             lda     (R_PASS)
   FFFF8ADD 89 18               514             bzr     DR_FIX
   FFFF8ADF 48 7B               515             ldi     xh,>R_GROW  ; dry run: R_GROW += delta
   FFFF8AE1 4A 22               516             ldi     xl,<R_GROW
   FFFF8AE3 A5 7B 2D            517             lda     (R_DLT)
   FFFF8AE6 BEr8Cr84            518             sjp     ADDS16
   FFFF8AE9 A5 7B 20            519             lda     (R_END)
   FFFF8AEC 08                  520             sta     xh
   FFFF8AED A5 7B 21            521             lda     (R_END+1)
   FFFF8AF0 0A                  522             sta     xl
   FFFF8AF1 9A                  523             rtn
   FFFF8AF2 68 0B               524 DR_UNDEF:   ldi     uh,11
   FFFF8AF4 BAr8Cr97            525             jmp     ERR_IN
   FFFF8AF7 A5 7B 2D            526 DR_FIX:     lda     (R_DLT)
   FFFF8AFA 8B 03               527             bzs     DR_WRITE
   FFFF8AFC BEr8Cr05            528             sjp     MOVE
   FFFF8AFF A5 7B 1E            529 DR_WRITE:   lda     (R_PTR)     ; the new digits over the old
   FFFF8B02 18                  530             sta     yh
   FFFF8B03 A5 7B 1F            531             lda     (R_PTR+1)
   FFFF8B06 1A                  532             sta     yl
   FFFF8B07 48 7B               533             ldi     xh,>R_DIG
   FFFF8B09 4A 32               534             ldi     xl,<R_DIG
   FFFF8B0B A5 7B 2C            535             lda     (R_NLEN)
   FFFF8B0E DF                  536             dec     a
   FFFF8B0F 2A                  537             sta     ul
   FFFF8B10 F5                  538 DR_COPY:    tin
   FFFF8B11 88 03               539             lop     ul,DR_COPY
   FFFF8B13 FD 18               540             ldx     y           ; X past them
   FFFF8B15 9A                  541             rtn
                            542 
                            543 ; MUL10ADD: U = U*10 + R_DIGIT; R_OVF = 1 past 16 bits.
   FFFF8B16 BEr8Br41            544 MUL10ADD:   sjp     SHLU        ; U*2
   FFFF8B19 A4                  545             lda     uh
   FFFF8B1A AE 7B 24            546             sta     (R_T)
   FFFF8B1D 24                  547             lda     ul
   FFFF8B1E AE 7B 25            548             sta     (R_T+1)
   FFFF8B21 BEr8Br41            549             sjp     SHLU
   FFFF8B24 BEr8Br41            550             sjp     SHLU        ; U*8
   FFFF8B27 F9                  551             rec
   FFFF8B28 24                  552             lda     ul
   FFFF8B29 A3 7B 25            553             adc     (R_T+1)
   FFFF8B2C 2A                  554             sta     ul
   FFFF8B2D A4                  555             lda     uh
   FFFF8B2E A3 7B 24            556             adc     (R_T)       ; U*10
   FFFF8B31 28                  557             sta     uh
   FFFF8B32 BEr8Br47            558             sjp     OVFC
   FFFF8B35 F9                  559             rec
   FFFF8B36 24                  560             lda     ul
   FFFF8B37 A3 7B 31            561             adc     (R_DIGIT)
   FFFF8B3A 2A                  562             sta     ul
   FFFF8B3B A4                  563             lda     uh
   FFFF8B3C B3 00               564             adi     a,0         ; + carry
   FFFF8B3E 28                  565             sta     uh
   FFFF8B3F 8E 06               566             bch     OVFC
   FFFF8B41 24                  567 SHLU:       lda     ul
   FFFF8B42 D9                  568             shl
   FFFF8B43 2A                  569             sta     ul
   FFFF8B44 A4                  570             lda     uh
   FFFF8B45 DB                  571             rol
   FFFF8B46 28                  572             sta     uh
   FFFF8B47 81 05               573 OVFC:       bcr     OV_RET
   FFFF8B49 B5 01               574             ldi     a,1
   FFFF8B4B AE 7B 2A            575             sta     (R_OVF)
   FFFF8B4E 9A                  576 OV_RET:     rtn
                            577 
                            578 ; NEWNUM: U = a line number. If that line exists: C = 1 and U = its new
                            579 ; number (unchanged before line `old`). Otherwise C = 0.
   FFFF8B4F BEr89rBE            580 NEWNUM:     sjp     PRGSTART
   FFFF8B52 B5 00               581             ldi     a,0
   FFFF8B54 AE 7B 29            582             sta     (R_INR)
   FFFF8B57 84                  583 NN_LINE:    lda     xh          ; at line `old`, the new numbers start
   FFFF8B58 A7 7B 16            584             cpa     (R_OADR)
   FFFF8B5B 89 17               585             bzr     NN_CHECK
   FFFF8B5D 04                  586             lda     xl
   FFFF8B5E A7 7B 17            587             cpa     (R_OADR+1)
   FFFF8B61 89 11               588             bzr     NN_CHECK
   FFFF8B63 A5 7B 10            589             lda     (R_NEW)
   FFFF8B66 AE 7B 1A            590             sta     (R_VAL)
   FFFF8B69 A5 7B 11            591             lda     (R_NEW+1)
   FFFF8B6C AE 7B 1B            592             sta     (R_VAL+1)
   FFFF8B6F B5 01               593             ldi     a,1
   FFFF8B71 AE 7B 29            594             sta     (R_INR)
   FFFF8B74 05                  595 NN_CHECK:   lda     (x)
   FFFF8B75 B7 FF               596             cpi     a,0xFF
   FFFF8B77 8B 25               597             bzs     NN_MISS
   FFFF8B79 A6                  598             cpa     uh
   FFFF8B7A 89 15               599             bzr     NN_NEXT
   FFFF8B7C 44                  600             inc     x
   FFFF8B7D 05                  601             lda     (x)
   FFFF8B7E 46                  602             dec     x
   FFFF8B7F 26                  603             cpa     ul
   FFFF8B80 89 0F               604             bzr     NN_NEXT
   FFFF8B82 A5 7B 29            605             lda     (R_INR)     ; found
   FFFF8B85 8B 08               606             bzs     NN_HIT
   FFFF8B87 A5 7B 1A            607             lda     (R_VAL)
   FFFF8B8A 28                  608             sta     uh
   FFFF8B8B A5 7B 1B            609             lda     (R_VAL+1)
   FFFF8B8E 2A                  610             sta     ul
   FFFF8B8F FB                  611 NN_HIT:     sec
   FFFF8B90 9A                  612             rtn
   FFFF8B91 A5 7B 29            613 NN_NEXT:    lda     (R_INR)
   FFFF8B94 8B 03               614             bzs     NN_ADV
   FFFF8B96 BEr89rE2            615             sjp     STEPVAL
   FFFF8B99 BEr89rC7            616 NN_ADV:     sjp     NEXTLINE
   FFFF8B9C 9E 47               617             bch     NN_LINE
   FFFF8B9E F9                  618 NN_MISS:    rec
   FFFF8B9F 9A                  619             rtn
                            620 
                            621 ; TODEC: U (1..65279) to decimal digits at R_DIG, R_NLEN of them.
   FFFF8BA0 58 7B               622 TODEC:      ldi     yh,>R_DIG
   FFFF8BA2 5A 32               623             ldi     yl,<R_DIG
   FFFF8BA4 48s8B               624             ldi     xh,>POWERS
   FFFF8BA6 4ArFD               625             ldi     xl,<POWERS
   FFFF8BA8 B5 00               626             ldi     a,0
   FFFF8BAA AE 7B 2F            627             sta     (R_LEAD)
   FFFF8BAD B5 04               628             ldi     a,4
   FFFF8BAF AE 7B 30            629             sta     (R_CNT)
   FFFF8BB2 B5 00               630 TD_POW:     ldi     a,0
   FFFF8BB4 AE 7B 2E            631             sta     (R_DGT)
   FFFF8BB7 FB                  632 TD_SUB:     sec                 ; U -= power while it fits
   FFFF8BB8 24                  633             lda     ul
   FFFF8BB9 01                  634             sbc     (x)
   FFFF8BBA AE 7B 24            635             sta     (R_T)
   FFFF8BBD 44                  636             inc     x
   FFFF8BBE A4                  637             lda     uh
   FFFF8BBF 01                  638             sbc     (x)
   FFFF8BC0 46                  639             dec     x
   FFFF8BC1 81 0E               640             bcr     TD_STOP
   FFFF8BC3 28                  641             sta     uh
   FFFF8BC4 A5 7B 24            642             lda     (R_T)
   FFFF8BC7 2A                  643             sta     ul
   FFFF8BC8 A5 7B 2E            644             lda     (R_DGT)
   FFFF8BCB DD                  645             inc     a
   FFFF8BCC AE 7B 2E            646             sta     (R_DGT)
   FFFF8BCF 9E 1A               647             bch     TD_SUB
   FFFF8BD1 A5 7B 2E            648 TD_STOP:    lda     (R_DGT)
   FFFF8BD4 89 08               649             bzr     TD_PUT
   FFFF8BD6 A5 7B 2F            650             lda     (R_LEAD)    ; no leading zeros
   FFFF8BD9 8B 0B               651             bzs     TD_NEXT
   FFFF8BDB A5 7B 2E            652             lda     (R_DGT)
   FFFF8BDE BB 30               653 TD_PUT:     ori     a,0x30
   FFFF8BE0 51                  654             sin     y
   FFFF8BE1 B5 01               655             ldi     a,1
   FFFF8BE3 AE 7B 2F            656             sta     (R_LEAD)
   FFFF8BE6 44                  657 TD_NEXT:    inc     x
   FFFF8BE7 44                  658             inc     x
   FFFF8BE8 A5 7B 30            659             lda     (R_CNT)
   FFFF8BEB DF                  660             dec     a
   FFFF8BEC AE 7B 30            661             sta     (R_CNT)
   FFFF8BEF 99 3F               662             bzr     TD_POW
   FFFF8BF1 24                  663             lda     ul          ; the units
   FFFF8BF2 BB 30               664             ori     a,0x30
   FFFF8BF4 51                  665             sin     y
   FFFF8BF5 14                  666             lda     yl
   FFFF8BF6 FB                  667             sec
   FFFF8BF7 B1 32               668             sbi     a,<R_DIG
   FFFF8BF9 AE 7B 2C            669             sta     (R_NLEN)
   FFFF8BFC 9A                  670             rtn
                            671 
   FFFF8BFD 10 27               672 POWERS:     .db     <10000, >10000
   FFFF8BFF E8 03               673             .db     <1000, >1000
   FFFF8C01 64 00               674             .db     <100, >100
   FFFF8C03 0A 00               675             .db     <10, >10
                            676 
                            677 ; MOVE: make room for (R_DLT > 0) or close the gap of (R_DLT < 0) the
                            678 ; new digits: everything from R_END to the program's end moves by R_DLT.
                            679 ; Then the line's length, BASPRG_END and R_OADR follow.
   FFFF8C05 FB                  680 MOVE:       sec                 ; U = END - R_END: bytes to move - 1
   FFFF8C06 A5 78 68            681             lda     (BASPRG_END+1)
   FFFF8C09 A1 7B 21            682             sbc     (R_END+1)
   FFFF8C0C 2A                  683             sta     ul
   FFFF8C0D A5 78 67            684             lda     (BASPRG_END)
   FFFF8C10 A1 7B 20            685             sbc     (R_END)
   FFFF8C13 28                  686             sta     uh
   FFFF8C14 A5 7B 2D            687             lda     (R_DLT)
   FFFF8C17 BF 80               688             bii     a,0x80
   FFFF8C19 89 19               689             bzr     MV_DOWN
   FFFF8C1B A5 78 67            690             lda     (BASPRG_END) ; up: copy from the end down
   FFFF8C1E 08                  691             sta     xh
   FFFF8C1F A5 78 68            692             lda     (BASPRG_END+1)
   FFFF8C22 0A                  693             sta     xl
   FFFF8C23 FD 5A               694             stx     y
   FFFF8C25 A5 7B 2D            695             lda     (R_DLT)
   FFFF8C28 FD DA               696             adr     y
   FFFF8C2A 47                  697 MV_UP:      lde     x
   FFFF8C2B 53                  698             sde     y
   FFFF8C2C 88 04               699             lop     ul,MV_UP
   FFFF8C2E FD 62               700             dec     uh
   FFFF8C30 93 08               701             bcs     MV_UP
   FFFF8C32 8E 1C               702             bch     MV_FIX
   FFFF8C34 A5 7B 20            703 MV_DOWN:    lda     (R_END)     ; down: copy from the start up
   FFFF8C37 08                  704             sta     xh
   FFFF8C38 A5 7B 21            705             lda     (R_END+1)
   FFFF8C3B 0A                  706             sta     xl
   FFFF8C3C A5 7B 1E            707             lda     (R_PTR)
   FFFF8C3F 18                  708             sta     yh
   FFFF8C40 A5 7B 1F            709             lda     (R_PTR+1)
   FFFF8C43 1A                  710             sta     yl
   FFFF8C44 A5 7B 2C            711             lda     (R_NLEN)
   FFFF8C47 FD DA               712             adr     y
   FFFF8C49 F5                  713 MV_DN:      tin
   FFFF8C4A 88 03               714             lop     ul,MV_DN
   FFFF8C4C FD 62               715             dec     uh
   FFFF8C4E 93 07               716             bcs     MV_DN
   FFFF8C50 A5 7B 1C            717 MV_FIX:     lda     (R_LINE)    ; the line's length byte
   FFFF8C53 08                  718             sta     xh
   FFFF8C54 A5 7B 1D            719             lda     (R_LINE+1)
   FFFF8C57 0A                  720             sta     xl
   FFFF8C58 44                  721             inc     x
   FFFF8C59 44                  722             inc     x
   FFFF8C5A F9                  723             rec
   FFFF8C5B 05                  724             lda     (x)
   FFFF8C5C A3 7B 2D            725             adc     (R_DLT)
   FFFF8C5F 0E                  726             sta     (x)
   FFFF8C60 48 78               727             ldi     xh,>BASPRG_END
   FFFF8C62 4A 67               728             ldi     xl,<BASPRG_END
   FFFF8C64 A5 7B 2D            729             lda     (R_DLT)
   FFFF8C67 BEr8Cr84            730             sjp     ADDS16
   FFFF8C6A FB                  731             sec                 ; line `old` moves if it is further on
   FFFF8C6B A5 7B 1F            732             lda     (R_PTR+1)
   FFFF8C6E A1 7B 17            733             sbc     (R_OADR+1)
   FFFF8C71 A5 7B 1E            734             lda     (R_PTR)
   FFFF8C74 A1 7B 16            735             sbc     (R_OADR)
   FFFF8C77 83 0A               736             bcs     MV_RET
   FFFF8C79 48 7B               737             ldi     xh,>R_OADR
   FFFF8C7B 4A 16               738             ldi     xl,<R_OADR
   FFFF8C7D A5 7B 2D            739             lda     (R_DLT)
   FFFF8C80 BEr8Cr84            740             sjp     ADDS16
   FFFF8C83 9A                  741 MV_RET:     rtn
                            742 
                            743 ; ADDS16: the 16-bit value at X += A (signed).
   FFFF8C84 2A                  744 ADDS16:     sta     ul
   FFFF8C85 68 00               745             ldi     uh,0
   FFFF8C87 BF 80               746             bii     a,0x80
   FFFF8C89 8B 02               747             bzs     AS_POS
   FFFF8C8B 68 FF               748             ldi     uh,0xFF
   FFFF8C8D 44                  749 AS_POS:     inc     x
   FFFF8C8E F9                  750             rec
   FFFF8C8F 05                  751             lda     (x)
   FFFF8C90 22                  752             adc     ul
   FFFF8C91 0E                  753             sta     (x)
   FFFF8C92 46                  754             dec     x
   FFFF8C93 05                  755             lda     (x)
   FFFF8C94 A2                  756             adc     uh
   FFFF8C95 0E                  757             sta     (x)
   FFFF8C96 9A                  758             rtn
                            759 
                            760 ; ERR_IN: ERROR UH IN the line being scanned. The program is unchanged
                            761 ; (errors come from the dry run); the ROM shows the error as one in that
                            762 ; line, and the up arrow shows the line.
   FFFF8C97 FD A8               763 ERR_IN:     psh     u
   FFFF8C99 FD 88               764             psh     x
   FFFF8C9B BE CF D0            765             sjp     INIT_SYS    ; ON ERROR GOTO must not catch it
   FFFF8C9E FD 0A               766             pop     x
   FFFF8CA0 FD 2A               767             pop     u
   FFFF8CA2 FD 5A               768             stx     y           ; where in the line
   FFFF8CA4 A5 7B 1C            769             lda     (R_LINE)
   FFFF8CA7 08                  770             sta     xh
   FFFF8CA8 A5 7B 1D            771             lda     (R_LINE+1)
   FFFF8CAB 0A                  772             sta     xl
   FFFF8CAC 05                  773             lda     (x)
   FFFF8CAD AE 78 9C            774             sta     (CURR_LINE)
   FFFF8CB0 44                  775             inc     x
   FFFF8CB1 05                  776             lda     (x)
   FFFF8CB2 AE 78 9D            777             sta     (CURR_LINE+1)
   FFFF8CB5 A5 78 65            778             lda     (BASPRG_ST)
   FFFF8CB8 AE 78 9E            779             sta     (CURR_TOP)
   FFFF8CBB A5 78 66            780             lda     (BASPRG_ST+1)
   FFFF8CBE AE 78 9F            781             sta     (CURR_TOP+1)
   FFFF8CC1 E0                  782             vej     0xE0
