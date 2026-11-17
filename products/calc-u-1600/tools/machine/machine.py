"""The PC-1600 as a pytest test machine, on `calcu serve` (library/harness/python/calcu_machine.py).

    machine = Machine(tmp_path, slot_2="ram-1m")
    machine.type('PRINT "=";6*7').wait(1)
    assert "= 42" in machine.lcd
    image = machine.card(2)

Steps collect until a result is read (`lcd`, `card()`, `run()`, `peek()`, ...); then they run on one
live machine. Steps after a read continue from there. Each Machine needs its own directory.
A failed `expect()` raises MachineError. See README.md.
"""

from __future__ import annotations

from pathlib import Path
from typing import Optional

from calcu_machine import MachineError, Session
from cardimage import read_card

__all__ = ["Machine", "MachineError"]

CARDS = Path(__file__).resolve().parent / "cards"


class Machine(Session):
    def __init__(self, workdir, slot_1: Optional[str] = None, slot_2: Optional[str] = None,
                 host_drive=None, model: str = "PC-1600"):
        """slot_1, slot_2: a module of cards/ (ram-256k, ...) or a bundled one (CE-1600M, ...)."""
        super().__init__(workdir, "1600")
        self.model = model
        self.slots = {1: slot_1, 2: slot_2}
        self.host_drive = host_drive

    def load(self, path) -> "Machine":
        """Loads a program file straight into memory (a file with a header brings its address)."""
        return self.command(f"pc1600 load-code {Path(path).resolve()}")

    def card(self, slot: int) -> bytes:
        """The byte image of the module in a slot, now."""
        path = self.workdir / f"slot-{slot}.module.yaml"
        path.unlink(missing_ok=True)
        self.send(f"saveas {{ place: slot-{slot}, as: live, file: {path} }}")
        return read_card(path)

    def setup(self) -> None:
        lines = ["format-version: 2", f"model: {self.model}"]
        for slot, name in self.slots.items():
            if name:
                file = CARDS / f"{name}.module.yaml"
                lines.append(f"slot-{slot}: {{ file: {file} }}" if file.exists() else f"slot-{slot}: {{ name: {name} }}")
        if self.host_drive:
            lines.append(f"host-drive: {{ folder: {Path(self.host_drive).resolve()} }}")
        preset = self.workdir / "setup.pc1600"
        preset.write_text("\n".join(lines) + "\n")
        self._send(f"preset {preset}")
        self._send("wait")
