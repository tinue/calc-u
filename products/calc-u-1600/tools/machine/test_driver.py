"""Tests of the driver and the card parser."""

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
PRINT_42 = 'PRINT "=";6*7'


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
        CARD.replace("addressed-hex", "base64"),
    ],
    ids=["gap", "bank number", "bank too long", "no bank size", "encoding"],
)
def test_read_card_rejects(tmp_path, broken):
    with pytest.raises(CardFormatError):
        read_card(write(tmp_path, broken))


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
    image = Machine(tmp_path, slot_2="ram-512k").card(2)
    assert len(image) == 512 * 1024


def test_step_rejects_line_break(tmp_path):
    with pytest.raises(ValueError):
        Machine(tmp_path).type("A\nkeys:")


# The harness commands behind wishes 1-4 and 12 of the requirements (calcu_machine.Session).


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

