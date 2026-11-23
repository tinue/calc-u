"""The PC-1500, PC-1500A and PC-1600 as a pytest test machine, on `calcu serve`
(library/harness/python/calcu_machine.py). The user guide is docs/testing/README.md.

    machine = Machine(tmp_path, slot_2="superRAM r4")
    machine.type('PRINT "=";6*7').wait(1)
    assert "= 42" in machine.lcd
    image = machine.card(2)

Steps collect until a result is read (`lcd`, `listing()`, `card()`, `run()`, `peek()`, ...); then
they run on one live machine. Steps after a read continue from there. Each Machine needs its own
directory. A failed `expect()` raises MachineError. In a pytest suite the fixture `make_machine`
(calcu_pytest.py) makes them.
"""

from __future__ import annotations

import os
import sys
from pathlib import Path
from typing import Any, Optional, Union

from calcu_machine import CHECKOUT, MachineError, Session
from cardimage import read_card

__all__ = ["Machine", "MachineError"]

PRODUCT = CHECKOUT / "products" / "calc-u-1600"
APP = "Calc-U-1600"

Slot = Union[str, os.PathLike, None]


def app_save_folder() -> Path:
    """The Calc-U-1600 app's save folder: [storage] save-folder of its settings file, else
    ~/Calc-U-1600 (library/settings/Paths.hpp). MachineError if the app has no settings file."""
    home = Path.home()
    if sys.platform == "darwin":
        base = home / "Library" / "Application Support"
    elif sys.platform == "win32":
        base = Path(os.environ.get("APPDATA", home))
    else:
        base = Path(os.environ.get("XDG_CONFIG_HOME", home / ".config"))
    ini = base / APP / f"{APP}.ini"
    if not ini.is_file():
        raise MachineError(f"$saved: the {APP} app has no settings file at {ini}; start the app once")
    section, folder = None, ""
    for line in ini.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if line.startswith("["):
            section = line.strip("[]").strip()
        elif section == "storage" and line.split("=", 1)[0].strip() == "save-folder" and "=" in line:
            folder = line.split("=", 1)[1].strip()
    if not folder:
        return home / APP
    return Path(home / folder[2:] if folder.startswith("~/") else folder)


class Machine(Session):
    def __init__(self, workdir, slot_1: Slot = None, slot_2: Slot = None, host_drive=None,
                 model: str = "PC-1600", printer: Optional[str] = None,
                 bus_rom: Optional[list[dict[str, Any]]] = None, project=None, paths=None):
        """model: PC-1500, PC-1500A or PC-1600.
        slot_1, slot_2: a bundled module's name (CE-1600M, superRAM r4, ...) or a module file: a
        Path, or a str ending in .module.yaml or holding a /. printer: CE-150 or CE-1600P[:old]. bus_rom: ROMs on the 60-pin bus, in plug order,
        each {"file", "address", "pv"}. project, paths: see Session.path()."""
        super().__init__(workdir, "c1600", project, paths)
        self.add_path_name("c1600", PRODUCT)
        self.add_path_name("saved", app_save_folder)
        self.model = model
        self.printer = printer
        self.slots = {1: slot_1, 2: slot_2}
        self._files = {slot: self._module_file(name) for slot, name in self.slots.items() if name}
        self.host_drive = host_drive
        self.bus_rom = [dict(rom, file=self._existing(rom["file"])) for rom in bus_rom or []]

    def _existing(self, path) -> Path:
        file = self.path(path)
        if not file.exists():
            raise MachineError(f"{path}: no file {file}")
        return file

    def _module_file(self, slot: Slot) -> Optional[Path]:
        """The module file of a slot argument, or None for a bundled module's name."""
        if isinstance(slot, os.PathLike) or "/" in slot or slot.endswith(".module.yaml"):
            return self._existing(slot)
        return None

    # ── steps ──────────────────────────────────────────────────────────

    def load(self, path) -> "Machine":
        """Loads machine code straight into memory (a file with a header brings its address)."""
        return self._step(f"c1600 load-code {self.path(path)}")

    def load_basic(self, path) -> "Machine":
        """Loads a BASIC program (a listing or tokenized) straight into the program area."""
        return self._step(f"c1600 load-basic {self.path(path)}")._step("wait")

    def type_basic(self, path) -> "Machine":
        """Types a BASIC listing in, line by line; the machine is in PRO mode for it."""
        return self._step(f"c1600 type-basic {self.path(path)}")._step("wait")

    def floppy(self, path, side: int = 0) -> "Machine":
        """Inserts a floppy disk file (.floppy.yaml) into the CE-1600F; needs printer CE-1600P."""
        return self._step(f"floppy {{ file: {self.path(path)}, side: {side} }}")

    def cassette(self, path) -> "Machine":
        """Inserts a cassette file (.cassette.yaml) into the printer's cassette recorder."""
        return self._step(f"cassette {{ file: {self.path(path)} }}")

    def eject(self, place: str) -> "Machine":
        """Ejects the floppy disk (`floppy`) or the cassette (`cassette`)."""
        return self._step(f"eject {place}")

    # ── reads ──────────────────────────────────────────────────────────

    def listing(self) -> list[str]:
        """The BASIC program as listing lines, as the detokenizer writes them (MODE 0 only)."""
        return list(self.send("c1600 list-basic"))

    def card(self, slot: int) -> bytes:
        """The byte image of the module in a slot, now; also saved as $work/slot-<n>.module.yaml."""
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
            lines.append(f"host-drive: {{ folder: {self.path(self.host_drive)} }}")
        if self.bus_rom:
            lines.append("bus-rom:")
            for rom in self.bus_rom:
                lines.append(f"  - file: {rom['file']}")
                lines += [f"    {key}: {value}" for key, value in rom.items() if key != "file"]
        preset = self.workdir / "setup.pc1600"
        preset.write_text("\n".join(lines) + "\n")
        self._send(f"preset {preset}")
        self._send("wait")
