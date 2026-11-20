"""A test machine on `calcu serve`, for pytest suites of any product.

    session = Session(workdir, "c1600")
    session.type('PRINT "=";6*7').wait(1)
    assert "= 42" in session.lcd

Steps (`key`, `type`, `wait`, `expect`, `not_expect`, `command`) are queued
and sent to one live machine at the next read (`run`, `lcd`, `display`,
`peek`, ...). Nothing is replayed: steps after a read continue from where the
machine is. Each `key` and `type` is followed by a `wait` until the machine
is idle, as the items of a preset are (docs/formats/preset.md). A refused
command raises MachineError with the harness's reason.

A path a test gives (`path()`) is absolute, `$name/...` (a named folder:
`$project`, `$calcu`, `$work`, the product's, the suite's own), or relative to
the test project root. An environment variable CALCU_PATH_<NAME> wins over a
name.

A product layer sets up the machine in `setup()` (for example with a preset
that has only a header) and adds its own steps; see
products/calc-u-1600/tools/machine/machine.py. Standard library only.
"""

from __future__ import annotations

import os
import weakref
from pathlib import Path
from typing import Any, Optional

from calcu_serve import TREES, Calcu, Refused, find_calcu

TIMEOUT_SECONDS = 300
CHECKOUT = Path(__file__).resolve().parents[3]   # library/harness/python/ in the checkout


class MachineError(RuntimeError):
    """The harness refused a command, or a check failed. The text says why."""


def calcu_program() -> str:
    """The `calcu` binary: $CALCU, else a root build tree's (calcu_serve.find_calcu)."""
    program = find_calcu()
    if not program or not Path(program).is_file():
        raise MachineError(f"no calcu at {program or 'build/{' + ','.join(TREES) + '}'}; "
                           "build the target calcu or set CALCU")
    return program


class Session:
    def __init__(self, workdir: os.PathLike | str, product: str, project: os.PathLike | str | None = None,
                 paths: Optional[dict[str, os.PathLike | str]] = None):
        """workdir takes the files of one machine: its save folder, outputs and results.
        project: the test project root (default: the current directory); paths: names of the
        suite's own folders, relative to project or absolute."""
        self.workdir = Path(workdir).resolve()
        self.product = product
        self.project = Path(project or Path.cwd()).resolve()
        self._names: dict[str, Any] = {"project": self.project, "calcu": CHECKOUT, "work": self.workdir}
        self._names.update({k: self.project / v for k, v in (paths or {}).items()})
        self._queue: list[str] = []
        self._calcu: Optional[Calcu] = None
        self._replies: list[dict[str, Any]] = []
        self._finalizer: Optional[weakref.finalize] = None

    # ── steps ──────────────────────────────────────────────────────────

    def key(self, names: str) -> "Session":
        """Presses keys by name, several separated by blanks."""
        return self._step(f"key {names}")._step("wait")

    def type(self, text: str) -> "Session":
        """Types one line of text; the line is ended as `type` ends it."""
        return self._step(f"type {text}")._step("wait")

    def wait(self, seconds: Optional[float] = None) -> "Session":
        """Waits in machine time; without seconds until the machine is idle."""
        return self._step("wait" if seconds is None else f"wait {seconds}s")

    def expect(self, text: str) -> "Session":
        """Fails the next read if no display line holds text (or matches /regex/)."""
        return self._step(f"expect {text}")

    def not_expect(self, text: str) -> "Session":
        """Fails the next read if a display line holds text."""
        return self._step(f"not-expect {text}")

    def command(self, line: str) -> "Session":
        """Queues any harness command (docs/formats/harness.md)."""
        return self._step(line)

    def _step(self, line: str) -> "Session":
        # The steps queue here, not through command(): a suite's subclass may give command() a
        # meaning of its own (a BASIC direct command, say) without breaking key() and type().
        if "\n" in line:
            raise ValueError(f"step with a line break: {line!r}")
        self._queue.append(line)
        return self

    # ── reads ──────────────────────────────────────────────────────────

    def run(self) -> list[dict[str, Any]]:
        """Sends the queued steps; returns the replies of all steps sent so far."""
        if self._calcu is None:
            self._start()
        while self._queue:
            self._send(self._queue.pop(0))
        return self._replies

    def send(self, line: str) -> Any:
        """Runs the queue, then one command; returns its result."""
        self.run()
        return self._send(line)["result"]

    @property
    def lcd(self) -> list[str]:
        """The text rows of the display."""
        result = self.send("display-text")
        return list(result["rows"]) if result["on"] else []

    def display(self) -> dict[str, Any]:
        """The display as dots (`rows` of 0/1) or segments, as `display --json` gives it."""
        return self.send("display")

    def peek(self, target: str, space: str, address: int, count: int = 1, **qualifiers: Any) -> bytes:
        """count cells read without side effects (an unreadable cell is 0);
        qualifiers such as bank=2, vbank=17."""
        out = self.workdir / "peek.bin"
        extra = "".join(f" {k}={v}" for k, v in qualifiers.items())
        self.send(f"peek {target} {space} {address:#x} {count}{extra} file={out}")
        return out.read_bytes()

    def expect_memory(self, target: str, space: str, address: int, values: bytes, **qualifiers: Any) -> None:
        """Raises MachineError if the cells do not hold values."""
        extra = "".join(f" {k}={v}" for k, v in qualifiers.items())
        self.send(f"expect-memory {target} {space} {address:#x} " + " ".join(f"{b:#x}" for b in values) + extra)

    def save_state(self, path: os.PathLike | str) -> None:
        self.send(f"state-save {Path(path).resolve()}")

    def restore_state(self, path: os.PathLike | str) -> None:
        self.send(f"state-restore {Path(path).resolve()}")

    def run_to(self, target: str, address: int, timeout: float, **qualifiers: Any) -> float:
        """Runs until target reaches address; MachineError if not within timeout
        seconds of machine time. The machine stays paused there; returns the
        seconds it took."""
        extra = "".join(f" {k}={v}" for k, v in qualifiers.items())
        self.send(f"break {target} {address:#x}{extra}")
        try:
            waited = self._send(f"wait {timeout}s")["result"]
        finally:
            self._send(f"break off {target} {address:#x}")
        if "runState" not in waited:
            raise MachineError(f"{target} did not reach {address:#x} within {timeout} s")
        return waited["seconds"]

    def wait_for(self, text: str, timeout: float = 60.0, step: float = 0.25) -> "Session":
        """Runs the queued steps, then waits in steps of `step` seconds of machine time until a
        display line holds text (or matches /regex/, as expect); MachineError after `timeout`.
        For work the idle wait can't see the end of, such as a PC-1600 LOAD, which reads the
        keyboard for BREAK and so looks idle."""
        self.run()
        waited = 0.0
        while True:
            try:
                self._send(f"expect {text}")
                return self
            except MachineError as e:
                if waited >= timeout:
                    raise MachineError(f"{text!r} not on the display within {timeout} s: {e}") from None
            self._send(f"wait {step}s")
            waited += step

    def resume(self) -> "Session":
        return self._step("resume")

    def close(self) -> None:
        if self._finalizer:
            self._finalizer()

    # ── paths ──────────────────────────────────────────────────────────

    def path(self, path: os.PathLike | str) -> Path:
        """An absolute path as it is; `$name/rest` in the named folder; anything else in the
        test project root."""
        text = os.fspath(path)
        if text.startswith("$"):
            name, _, rest = text[1:].partition("/")
            base = os.environ.get(f"CALCU_PATH_{name.upper()}")
            if base is None:
                if name not in self._names:
                    raise MachineError(f"{text}: no path name ${name}; the names: "
                                       + ", ".join("$" + n for n in sorted(self._names)))
                base = self._names[name]
                if callable(base):
                    base = base()
            return (self.project / base / rest).resolve()
        return (self.project / text).resolve()

    def add_path_name(self, name: str, folder: Any) -> None:
        """For the product layer: a name for a folder, or a function giving it when it is used."""
        self._names.setdefault(name, folder)

    # ── for the product layer ─────────────────────────────────────────

    def setup(self) -> None:
        """Called once, before the first step is sent. The product layer fits
        modules and media here, for example with self._send("preset ...")."""

    def _start(self) -> None:
        self.workdir.mkdir(parents=True, exist_ok=True)
        for sub in ("save", "out"):
            (self.workdir / sub).mkdir(exist_ok=True)
        self._calcu = Calcu(calcu_program(), self.product,
                            ["--save-folder", str(self.workdir / "save"),
                             "--output-folder", str(self.workdir / "out")],
                            timeout=TIMEOUT_SECONDS)
        self._finalizer = weakref.finalize(self, self._calcu.close)
        self.setup()

    def _send(self, line: str) -> dict[str, Any]:
        assert self._calcu is not None
        try:
            reply = self._calcu.send(line)
        except Refused as e:
            raise MachineError(f"{line}: {e}") from None
        except (TimeoutError, RuntimeError) as e:
            raise MachineError(f"{line}: {e}") from None
        self._replies.append(reply)
        return reply
