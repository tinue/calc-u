from pathlib import Path

PROGRAMS = Path(__file__).parent / "programs"
COM1 = 'SETCOM "COM1:",9600,8,N,1,N,N'
LISTED = (PROGRAMS / "greet.bas").read_text().splitlines()   # listing() gives these lines as they are


def test_console(make_machine, tmp_path):
    name = tmp_path / "name.txt"
    name.write_bytes(b"WORLD\r")                 # INPUT takes the line up to CR
    machine = make_machine(model="PC-1600")
    machine.key("mode").type("NEW0").load_basic("$programs/greet.bas").key("mode")
    machine.type("RUN").wait(1)                  # NAME? is out, INPUT waits
    machine.command(f"c1600 serial rs232c feed {name}").wait(1)
    report = machine.send("c1600 serial rs232c report")
    assert report["sent"].splitlines() == ["NAME?", "HELLO WORLD"]
    assert report["taken"] == report["fed"] == 6


def test_save(make_machine, tmp_path):
    sent = tmp_path / "sent.bas"
    machine = make_machine(model="PC-1600")
    machine.key("mode").type("NEW0").load_basic("$programs/greet.bas").key("mode")
    machine.command(f"c1600 serial rs232c capture {sent}")
    machine.type(COM1).type('SAVE"COM1:",A').run()   # type waits until the SAVE has ended
    machine.close()                              # the capture file is complete when calcu ends
    data = sent.read_bytes()
    assert data.endswith(b"\r\n\x1a")            # CR LF after each line, 1AH at the end
    assert [line.rstrip() for line in data[:-3].decode("ascii").split("\r\n")] == LISTED


def test_load(make_machine, tmp_path):
    file = tmp_path / "greet.txt"
    file.write_bytes(b"".join(line.encode("ascii") + b"\r\n" for line in LISTED) + b"\x1a")
    machine = make_machine(model="PC-1600")
    machine.key("mode").type("NEW0").key("mode").type(COM1)
    machine.command('type LOAD"COM1:"')          # the harness's type: no wait
    machine.command(f"c1600 serial rs232c feed {file} hold=100").wait()
    assert machine.listing() == LISTED


def test_ce158_csave_cload(make_machine, tmp_path):
    sent = tmp_path / "sent.bin"
    one = make_machine(model="PC-1500A").command("attach interface CE-158")
    one.key("cl").type("NEW0").type('10 PRINT "HELLO"').type("20 GOTO 10").key("mode")
    one.command(f"c1600 serial ce158 capture {sent}")
    one.type("SETCOM 1200,8,N,1").type("OUTSTAT 0").type("SETDEV CO").type('CSAVE "P"')
    listing = one.listing()
    one.close()
    two = make_machine(model="PC-1500A").command("attach interface CE-158")
    two.key("cl").type("NEW0").key("mode")
    two.type("SETCOM 1200,8,N,1").type("OUTSTAT 0").type("SETDEV CI")
    two.command('type CLOAD "P"')
    two.command(f"c1600 serial ce158 feed {sent} hold=10").wait()
    assert two.listing() == listing == ['10 PRINT "HELLO"', "20 GOTO 10"]
