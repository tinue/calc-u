import pytest

REFUSED, ACCEPTED = True, False
CASES = {
    "none":               ({}, ACCEPTED),
    "CE-161 in 1":        ({"slot_1": "CE-161"}, ACCEPTED),
    "CE-163F in 1":       ({"slot_1": "CE-163F"}, ACCEPTED),
    "CE-163F in 2":       ({"slot_2": "CE-163F"}, ACCEPTED),
    "CE-1600M in 1":      ({"slot_1": "CE-1600M"}, REFUSED),
    "CE-1600M in 2":      ({"slot_2": "CE-1600M"}, REFUSED),
    "superRAM in 2":      ({"slot_2": "superRAM"}, REFUSED),
    "CE-155 and CE-1638": ({"slot_1": "CE-155", "slot_2": "CE-1638"}, REFUSED),
    "a card file in 2":   ({"slot_2": "$c1600/examples/dwx/CE-1601M - Progs.module.yaml"}, REFUSED),
}


@pytest.mark.parametrize("slots, refused", CASES.values(), ids=CASES.keys())
def test_mode1(make_machine, slots, refused):
    machine = make_machine(model="PC-1600", **slots)
    machine.key("mode").type("NEW0").key("mode")     # NEW0 needs PRO mode; back to RUN
    machine.type("MODE1")
    if refused:
        machine.expect("ERROR 110")
    else:
        machine.not_expect("ERROR")
    machine.run()
