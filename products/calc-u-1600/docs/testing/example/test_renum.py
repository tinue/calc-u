from pathlib import Path

import pytest

# listing() gives the lines as the detokenizer writes them: no blank between a
# variable or number and the keyword after it (ON N+1GOTO, IF NTHEN).
LISTED = [
    "10 REM RENUM DEMO", "20 RESTORE 80", "30 READ N",
    "40 ON N+1GOTO 70,50,50,50", "50 GOSUB 90", "60 IF NTHEN 30",
    '70 PRINT "DONE":END', "80 DATA 3,2,1,0", "90 PRINT N:RETURN",
]
PROGRAM = Path(__file__).parent / "programs"
RENUM_ROM = "$c1600/tests/fixtures/dap/pc1500-bus-rom/rom.bin"
RENUMBERED = {
    "RENUM": LISTED,                          # 10, 20, ... again
    "RENUM 100,,10": [
        "100 REM RENUM DEMO", "110 RESTORE 170", "120 READ N",
        "130 ON N+1GOTO 160,140,140,140", "140 GOSUB 180", "150 IF NTHEN 120",
        '160 PRINT "DONE":END', "170 DATA 3,2,1,0", "180 PRINT N:RETURN",
    ],
    "RENUM 1000,50,100": [                    # from line 50 on only
        "10 REM RENUM DEMO", "20 RESTORE 1300", "30 READ N",
        "40 ON N+1GOTO 1200,1000,1000,1000", "1000 GOSUB 1400", "1100 IF NTHEN 30",
        '1200 PRINT "DONE":END', "1300 DATA 3,2,1,0", "1400 PRINT N:RETURN",
    ],
}
REFUSED = {"RENUM 100,,0": "ERROR 19", "RENUM 60000,,1000": "ERROR 30", "RENUM 100,55": "ERROR 11"}


@pytest.fixture(params=["PC-1500", "PC-1500A"])
def machine(request, make_machine):
    renum = {"file": RENUM_ROM, "address": 0x8800, "pv": 0}
    return make_machine(model=request.param, bus_rom=[renum])


@pytest.fixture(params=["typed", "load_basic", "type_basic"])
def with_program(request, machine):
    machine.key("cl").type("NEW0")             # a fresh PC-1500 is in PRO mode
    if request.param == "typed":
        for line in (PROGRAM / "jumps.bas").read_text().splitlines():
            machine.type(line)
    else:
        getattr(machine, request.param)("$programs/jumps.bas")
    return machine.key("mode")                 # to RUN mode


@pytest.mark.parametrize("command", RENUMBERED)
def test_renum(with_program, command):
    assert with_program.type(command).listing() == RENUMBERED[command]


@pytest.mark.parametrize("command", REFUSED)
def test_renum_refused(with_program, command):
    machine = with_program.type(command).expect(REFUSED[command])
    assert machine.listing() == LISTED
