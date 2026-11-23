"""Tests of the driver and the card parser."""

import sys
from pathlib import Path

import pytest

from cardimage import CardFormatError, RamDisk, read_card
from machine import Machine, MachineError

CARD = """\
    bank-size: 0x20
    initial-content:
      blocks:
        - bank: 0
          offset: 0x0000
          encoding: addressed-hex
          bytes: |
            $0000: 01 02 03 04 05 06 07 08  09 0A 0B 0C 0D 0E 0F 10
            $0010: AA...
        - bank: 1
          offset: 0x0000
          encoding: addressed-hex
          bytes: |
            $0000: FF...
"""
UNBANKED = """\
    capacity: 0x20
    banking: none
    initial-content:
      blocks:
        - offset: 0x0000
          encoding: addressed-hex
          bytes: |
            $0000: 01 02 03 04 05 06 07 08  09 0A 0B 0C 0D 0E 0F 10
            $0010: FF...
"""
PRINT_42 = 'PRINT "=";6*7'
DWX = Path(__file__).resolve().parents[2] / "examples" / "dwx"
OWN_CARD = DWX / "CE-1601M - Progs.module.yaml"
OWN_FLOPPY = DWX / "Progs.floppy.yaml"
BANK_1 = slice(0x8000, None)   # the RAM disk; the PC-1600 rewrites bank 0's head at power on


def write(tmp_path, text):
    path = tmp_path / "card.yaml"
    path.write_text(text)
    return path


def test_read_card(tmp_path):
    image = read_card(write(tmp_path, CARD))
    assert image == bytes(range(1, 17)) + b"\xAA" * 16 + b"\xFF" * 32


@pytest.mark.parametrize(
    "broken",
    [
        CARD.replace("$0010: AA...", "$0018: AA..."),
        CARD.replace("        - bank: 1", "        - bank: 2"),
        CARD.replace("bank-size: 0x20", "bank-size: 0x08"),
        CARD.replace("bank-size: 0x20", ""),
        UNBANKED.replace("capacity: 0x20", "capacity: 0x08"),
        CARD.replace("addressed-hex", "base64"),
    ],
    ids=["gap", "bank number", "bank too long", "no bank size", "unbanked too long", "encoding"],
)
def test_read_card_rejects(tmp_path, broken):
    with pytest.raises(CardFormatError):
        read_card(write(tmp_path, broken))


def test_read_unbanked_card(tmp_path):
    assert read_card(write(tmp_path, UNBANKED)) == bytes(range(1, 17)) + b"\xFF" * 16


def test_unbanked_module_is_returned(tmp_path):
    assert len(Machine(tmp_path, slot_1="CE-1600M").card(1)) == 32 * 1024


def test_file_chain_is_checked():
    image = bytearray(0x4000)
    image[0x12], image[0x13], image[0x15], image[0x17] = 1, 3, 0x10, 1
    image[0x18], image[0x0D] = 2, 0
    image[0x09:0x0B] = b"\x00\x02"
    image[0x201], image[0x202] = 2, 1
    image[0x400:0x40B] = b"LOOP       "
    image[0x41A] = 1
    disk = RamDisk(bytes(image))
    with pytest.raises(CardFormatError):
        disk.clusters(disk.files()[0])


def test_machine_returns_display(tmp_path):
    machine = Machine(tmp_path).type(PRINT_42).wait(1)
    assert machine.lcd[:2] == [PRINT_42, "= 42"]


def test_steps_after_a_read_continue(tmp_path):
    machine = Machine(tmp_path).type(PRINT_42).wait(1)
    assert "= 42" in machine.lcd
    machine.key("cl").type('PRINT "=";6*8').wait(1)
    assert "= 48" in machine.lcd


def test_failed_expect_raises(tmp_path):
    with pytest.raises(MachineError, match="absent"):
        Machine(tmp_path).type(PRINT_42).wait(1).expect("absent").run()


def test_blank_card_is_returned(tmp_path):
    image = Machine(tmp_path, slot_2="superRAM r4").card(2)
    assert len(image) == 1024 * 1024


def test_step_rejects_line_break(tmp_path):
    with pytest.raises(ValueError):
        Machine(tmp_path).type("A\nkeys:")


# The steps and reads on harness commands of calcu_machine.Session: keys, expectations, memory,
# breaking and waiting for a program, the display's dots, the state, running to an address.


def test_bs_is_a_key(tmp_path):
    machine = Machine(tmp_path).key("cl").key("a b").key("bs")
    assert machine.lcd[0].rstrip() == "A"


def test_not_expect_raises(tmp_path):
    with pytest.raises(MachineError, match="= 42"):
        Machine(tmp_path).type(PRINT_42).not_expect("= 42").run()


def test_peek_reads_memory(tmp_path):
    machine = Machine(tmp_path).type("POKE &C000,1,2,3")
    assert machine.peek("SC7852", "memory", 0xC000, 3) == bytes([1, 2, 3])
    machine.expect_memory("SC7852", "memory", 0xC000, bytes([1, 2, 3]))
    with pytest.raises(MachineError):
        machine.expect_memory("SC7852", "memory", 0xC000, bytes([9]))


def test_peek_reads_either_half_of_bank_3(tmp_path):
    # Port 3DH bit 2 is page B's vertical bank: 0 the ROM of bank 3, 1 the hidden bank 3b,
    # whatever the port selects now.
    roms = Path(__file__).parents[2] / "roms"
    bank3 = (roms / "PC1600-P1-B3-new.bin").read_bytes()[:4]
    bank3b = (roms / "PC1600-P1-B3B-new.bin").read_bytes()[:4]
    assert bank3 != bank3b
    machine = Machine(tmp_path)
    assert machine.peek("SC7852", "memory", 0x4000, 4, bank=3, vbank=0) == bank3
    assert machine.peek("SC7852", "memory", 0x4000, 4, bank=3, vbank=1) == bank3b
    assert machine.peek("SC7852", "memory", 0x4000, 4, bank=3) == bank3


def test_break_goes_in_while_a_program_runs(tmp_path):
    machine = Machine(tmp_path).key("mode").type("10 GOTO 10").key("mode").command("type RUN").wait(1)
    assert "BREAK" in " ".join(machine.key("break").lcd)


def test_wait_finished_lasts_until_the_program_has_ended(tmp_path):
    machine = Machine(tmp_path).key("mode").type("10 FOR I=1 TO 300").type("20 NEXT I").key("mode")
    assert machine.command("type RUN").wait_finished().lcd[:2] == ["RUN", ">"]


def test_key_takes_its_own_hold(tmp_path):
    assert Machine(tmp_path).key("cl").key("a", hold=0.2).lcd[0].startswith("A")


def test_display_gives_the_dots(tmp_path):
    dots = Machine(tmp_path).type(PRINT_42).display()
    assert len(dots["rows"]) == 32 and len(dots["rows"][0]) == 156
    assert any(any(row) for row in dots["rows"])


def test_state_restores(tmp_path):
    machine = Machine(tmp_path).type("A=42")
    machine.save_state(tmp_path / "state.bin")
    machine.type("A=7").type("PRINT A")
    machine.restore_state(tmp_path / "state.bin")
    machine.key("cl").type("PRINT A")
    assert "42" in " ".join(machine.lcd)


def test_run_to_an_address(tmp_path):
    loop = tmp_path / "loop.bin"
    loop.write_bytes(bytes([0x00, 0x00, 0x18, 0xFE]))   # NOP, NOP, then a loop on itself
    machine = Machine(tmp_path).command(f"c1600 load-code {loop} address=0xD000").command("type CALL &D000")
    assert machine.run_to("SC7852", 0xD002, timeout=1) < 1
    with pytest.raises(MachineError, match="did not reach"):
        Machine(tmp_path / "b").run_to("SC7852", 0x0005, timeout=0.1)



def test_run_to_where_it_stands_runs_round_once_more(tmp_path):
    loop = tmp_path / "loop.bin"
    loop.write_bytes(bytes([0x00, 0x00, 0x18, 0xFC]))   # NOP, NOP, then back to the first NOP
    machine = Machine(tmp_path).command(f"c1600 load-code {loop} address=0xD000").command("type CALL &D000")
    machine.run_to("SC7852", 0xD001, timeout=1)
    assert machine.run_to("SC7852", 0xD001, timeout=1) > 0
    machine.run_to("SC7852", 0xD002, timeout=1)
    assert machine.registers("SC7852")["pc"] == 0xD002
    assert machine.step(2)["reason"] == "step"
    assert machine.registers("SC7852")["pc"] == 0xD001
    machine.set_register("SC7852", "hl", 0x1234)
    assert machine.registers("SC7852")["hl"] == 0x1234
    machine.poke("SC7852", "memory", 0xC000, bytes([7, 8]))
    assert machine.peek("SC7852", "memory", 0xC000, 2) == bytes([7, 8])
    assert not machine.resume()["paused"]


# Modules and media of your own: module files by path, floppy disks, eject.


def test_own_card_is_loaded(tmp_path):
    image = Machine(tmp_path, slot_2=OWN_CARD).card(2)
    assert image[BANK_1] == read_card(OWN_CARD)[BANK_1]


def test_relative_paths_are_in_the_project(tmp_path):
    (tmp_path / "mine").mkdir()
    (tmp_path / "mine" / "card.module.yaml").write_text(OWN_CARD.read_text())
    image = Machine(tmp_path / "work", slot_2="mine/card.module.yaml", project=tmp_path).card(2)
    assert image[BANK_1] == read_card(OWN_CARD)[BANK_1]


def test_saved_card_comes_back(tmp_path):
    first = Machine(tmp_path / "a", slot_2=OWN_CARD).command("poke SC7852 memory 0x8000 0x5a bank=2 vbank=1")
    image = first.card(2)
    assert image[0x8000] == 0x5A
    assert Machine(tmp_path / "b", slot_2=tmp_path / "a" / "slot-2.module.yaml").card(2) == image


def test_template_card_file_stays_as_it_is(tmp_path):
    # A fixture with `template: true` is never written: the machine's writes stay in memory, so tests
    # that run side by side never see each other's writes. card() reads what the machine wrote.
    Machine(tmp_path / "a", slot_2="superRAM r4").card(2)
    text = (tmp_path / "a" / "slot-2.module.yaml").read_text()
    mine = tmp_path / "mine.module.yaml"
    mine.write_text(text.replace("\nmodule-name:", "\ntemplate: true\nmodule-name:", 1))
    before = mine.read_text()
    machine = Machine(tmp_path / "b", slot_2=mine)
    machine.key("mode").type("NEW0").key("mode").wait(1).type('INIT"S2:","P"').wait(3).run()
    assert machine.card(2) != read_card(mine)
    machine.close()
    assert mine.read_text() == before


def test_own_floppy_goes_in_and_out(tmp_path):
    def sides(path):
        text = path.read_text()
        return text[text.index("\nsides:"):]

    saved = tmp_path / "live.floppy.yaml"
    machine = Machine(tmp_path, printer="CE-1600P").floppy(OWN_FLOPPY)
    machine.command(f"saveas {{ place: floppy, as: live, file: {saved} }}").run()
    assert sides(saved) == sides(OWN_FLOPPY)
    machine.eject("floppy").run()


def test_missing_own_card_raises(tmp_path):
    with pytest.raises(MachineError, match="no file"):
        Machine(tmp_path, slot_2=tmp_path / "absent.module.yaml")
    with pytest.raises(MachineError, match="no file"):
        Machine(tmp_path, bus_rom=[{"file": "absent.bin", "address": 0x8800, "pv": 0}], project=tmp_path)


def test_floppy_needs_the_printer(tmp_path):
    with pytest.raises(MachineError, match="CE-1600P"):
        Machine(tmp_path).floppy(OWN_FLOPPY).run()


def test_subclass_may_redefine_command(tmp_path):
    class Suite(Machine):
        def command(self, text, seconds=3):   # a BASIC direct command, as a suite may define it
            return self.key("cl").type(text).wait(seconds)

    assert "= 42" in Suite(tmp_path).command(PRINT_42, 1).lcd


# Path names, listing(), load_basic(), bus ROMs.


def test_path_names(tmp_path, monkeypatch):
    machine = Machine(tmp_path / "work", project=tmp_path, paths={"cards": "my-cards", "stick": "/Volumes/X"})
    assert machine.path("a.bas") == tmp_path / "a.bas"
    assert machine.path("/abs/a.bas") == Path("/abs/a.bas")
    assert machine.path("$project/a.bas") == tmp_path / "a.bas"
    assert machine.path("$cards/a.module.yaml") == tmp_path / "my-cards" / "a.module.yaml"
    assert machine.path("$stick/a") == Path("/Volumes/X/a")
    assert machine.path("$work/slot-2.module.yaml") == machine.workdir / "slot-2.module.yaml"
    assert machine.path("$c1600/examples") == DWX.parent
    assert machine.path("$calcu/library") == DWX.parents[3] / "library"
    monkeypatch.setenv("CALCU_PATH_CARDS", str(tmp_path / "elsewhere"))
    assert machine.path("$cards/a") == tmp_path / "elsewhere" / "a"
    with pytest.raises(MachineError, match=r"no path name \$nope"):
        machine.path("$nope/a")


def test_saved_is_the_apps_save_folder(tmp_path, monkeypatch):
    monkeypatch.setattr(Path, "home", lambda: tmp_path)
    monkeypatch.setenv("XDG_CONFIG_HOME", str(tmp_path / ".config"))
    machine = Machine(tmp_path / "work", project=tmp_path)
    with pytest.raises(MachineError, match="no settings file"):
        machine.path("$saved/a.floppy.yaml")
    base = tmp_path / "Library" / "Application Support" if sys.platform == "darwin" else tmp_path / ".config"
    (base / "Calc-U-1600").mkdir(parents=True)
    ini = base / "Calc-U-1600" / "Calc-U-1600.ini"
    ini.write_text("[storage]\n# The save folder\nsave-folder =\n")
    assert machine.path("$saved/a") == tmp_path / "Calc-U-1600" / "a"
    ini.write_text("[storage]\nsave-folder = ~/Media\n")
    assert machine.path("$saved/a") == tmp_path / "Media" / "a"


def test_listing_after_load_basic(tmp_path):
    program = tmp_path / "p.bas"
    program.write_text("10 PRINT 1\n20 GOTO 10\n")
    machine = Machine(tmp_path / "work", model="PC-1500A", project=tmp_path)
    assert machine.key("cl").type("NEW0").listing() == []
    assert machine.load_basic("p.bas").listing() == ["10 PRINT 1", "20 GOTO 10"]


def test_bus_rom_adds_renum(tmp_path):
    renum = {"file": "$c1600/tests/fixtures/dap/pc1500-bus-rom/rom.bin", "address": 0x8800, "pv": 0}
    machine = Machine(tmp_path, model="PC-1500A", bus_rom=[renum]).key("cl").type("NEW0")
    machine.type("1 GOTO 2").type("2 END").key("mode").type("RENUM")
    assert machine.listing() == ["10 GOTO 20", "20 END"]


def test_wait_for_text(tmp_path):
    machine = Machine(tmp_path, model="PC-1500A").key("cl").type("NEW0")
    machine.type("10 FOR I=1 TO 300:NEXT I").type('20 PRINT "DO"+"NE"').key("mode").type("RUN")
    assert "DONE" in " ".join(machine.wait_for("DONE", timeout=30).lcd)
    with pytest.raises(MachineError, match="not on the display within 0.5 s"):
        machine.wait_for("ABSENT", timeout=0.5)
