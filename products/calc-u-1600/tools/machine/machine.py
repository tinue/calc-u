"""The PC-1600 as a pytest test machine, on `calcu serve` (library/harness/python/calcu_machine.py).

    machine = Machine(tmp_path, slot_2="ram-1m")
    machine.type('PRINT "=";6*7').wait(1)
    assert "= 42" in machine.lcd
    image = machine.card(2)

    own = Machine(tmp_path, slot_2=Path("my.module.yaml"), printer="CE-1600P").floppy("my.floppy.yaml")

Steps collect until a result is read (`lcd`, `card()`, `run()`, `peek()`, ...); then they run on one
live machine. Steps after a read continue from there. Each Machine needs its own directory.
A failed `expect()` raises MachineError. See README.md.
"""

from __future__ import annotations

import os
from pathlib import Path
from typing import Optional, Union

from calcu_machine import MachineError, Session
from cardimage import read_card

__all__ = ["Machine", "MachineError"]

CARDS = Path(__file__).resolve().parent / "cards"

Slot = Union[str, os.PathLike, None]


def _module_file(slot: Slot, workdir: Path) -> Optional[Path]:
    """The module file a slot argument names, or None for a name (cards/, else bundled)."""
    if isinstance(slot, os.PathLike) or (isinstance(slot, str) and ("/" in slot or slot.endswith(".module.yaml"))):
        path = (workdir / slot).resolve()
        if not path.is_file():
            raise MachineError(f"no module file {path}")
        return path
    file = CARDS / f"{slot}.module.yaml"
    return file if file.exists() else None


class Machine(Session):
    def __init__(self, workdir, slot_1: Slot = None, slot_2: Slot = None,
                 host_drive=None, model: str = "PC-1600", printer: Optional[str] = None):
        """slot_1, slot_2: a module of cards/ (ram-256k, ...), a bundled one (CE-1600M, ...), or a
        module file of your own (a Path, or a str ending in .module.yaml or holding a /).
        printer: CE-150 or CE-1600P[:old]; a floppy or cassette needs one.
        A relative path, here and in the steps, is relative to workdir, not to the current directory."""
        super().__init__(workdir, "c1600")
        self.model = model
        self.printer = printer
        self.slots = {1: slot_1, 2: slot_2}
        self._files = {slot: _module_file(name, self.workdir) for slot, name in self.slots.items() if name}
        self.host_drive = host_drive

    def _path(self, path) -> Path:
        return (self.workdir / path).resolve()

    def load(self, path) -> "Machine":
        """Loads a program file straight into memory (a file with a header brings its address)."""
        return self._step(f"c1600 load-code {self._path(path)}")

    def floppy(self, path, side: int = 0) -> "Machine":
        """Inserts a floppy disk file (.floppy.yaml) into the CE-1600F; needs printer CE-1600P."""
        return self._step(f"floppy {{ file: {self._path(path)}, side: {side} }}")

    def cassette(self, path) -> "Machine":
        """Inserts a cassette file (.cassette.yaml) into the printer's cassette recorder."""
        return self._step(f"cassette {{ file: {self._path(path)} }}")

    def eject(self, place: str) -> "Machine":
        """Ejects the floppy disk (`floppy`) or the cassette (`cassette`)."""
        return self._step(f"eject {place}")

    def card(self, slot: int) -> bytes:
        """The byte image of the module in a slot, now."""
        path = self.workdir / f"slot-{slot}.module.yaml"
        path.unlink(missing_ok=True)
        self.send(f"saveas {{ place: slot-{slot}, as: live, file: {path} }}")
        return read_card(path)

    def setup(self) -> None:
        lines = ["format-version: 2", f"model: {self.model}"]
        if self.printer:
            lines.append(f"printer: {self.printer}")
        for slot, name in self.slots.items():
            if name:
                file = self._files[slot]
                lines.append(f"slot-{slot}: {{ file: {file} }}" if file else f"slot-{slot}: {{ name: {name} }}")
        if self.host_drive:
            lines.append(f"host-drive: {{ folder: {self._path(self.host_drive)} }}")
        preset = self.workdir / "setup.pc1600"
        preset.write_text("\n".join(lines) + "\n")
        self._send(f"preset {preset}")
        self._send("wait")
