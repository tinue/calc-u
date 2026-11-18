"""A small helper for driving `calcu serve` over a pipe
(docs/formats/harness.md).

    from calcu_serve import Calcu
    with Calcu("/path/to/calcu", "c59") as c:
        c.send("wait 2s")
        print(c.send("display-text")["result"]["rows"])

Each send() writes one command line and reads its one reply. With
json=True (the default) the reply is the JSON object of --json, with the
keys "done", "result" and "reason"; with json=False it is a dict with
"done", "lines" (the result lines) and "reason". A refused command raises
Refused unless check=False. With a timeout (seconds of wall clock, for
the whole reply), a reply that does not come in time ends the program and
raises TimeoutError. find_calcu() finds the program. Standard library only.
"""

from __future__ import annotations

import json
import os
import queue
import subprocess
import threading
from pathlib import Path
from typing import Any, Optional, Sequence

REPO = Path(__file__).resolve().parents[3]
# The root build trees that hold `calcu`, in the order they are looked in:
# build/tests (the Frequent tree, CLAUDE.md), build/all (everything),
# build/tools (Calc-U-59's scripts build it on first use, tools/calcu59.py).
TREES = ("tests", "all", "tools")


def find_calcu() -> Optional[str]:
    """The `calcu` program: $CALCU (as given), else the first root build tree
    of TREES that holds it, else None."""
    if os.environ.get("CALCU"):
        return os.environ["CALCU"]
    for tree in TREES:
        program = REPO / "build" / tree / "library" / "harness" / "cli" / "calcu"
        for candidate in (program, program.with_suffix(".exe")):
            if candidate.is_file():
                return str(candidate)
    return None


class Refused(Exception):
    """A command was refused; the reason is the message."""


class Calcu:
    def __init__(self, program: str, product: str, options: Sequence[str] = (),
                 json: bool = True, cwd: Optional[str] = None, timeout: Optional[float] = None):
        args = [program, "--product", product, *options]
        if json:
            args.append("--json")
        args.append("serve")
        self._json = json
        self._timeout = timeout
        self._lines: "queue.Queue[str]" = queue.Queue()
        self._proc = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                      stderr=subprocess.PIPE, text=True, encoding="utf-8",
                                      bufsize=1, cwd=cwd)
        threading.Thread(target=self._read, daemon=True).start()

    def _read(self) -> None:
        assert self._proc.stdout
        for line in self._proc.stdout:
            self._lines.put(line)
        self._lines.put("")   # the end of the output

    def _line(self, timeout: Optional[float]) -> str:
        try:
            return self._lines.get(timeout=timeout)
        except queue.Empty:
            self._proc.kill()
            raise TimeoutError("no reply from calcu within %g s" % timeout) from None

    def send(self, line: str, check: bool = True, timeout: Optional[float] = None) -> dict[str, Any]:
        """One command and its reply; timeout overrides the one of the constructor."""
        if "\n" in line:
            raise ValueError("a command is one line")
        assert self._proc.stdin and self._proc.stdout
        self._proc.stdin.write(line + "\n")
        self._proc.stdin.flush()
        timeout = self._timeout if timeout is None else timeout
        first = self._line(timeout)
        if not first:
            err = self._proc.stderr.read() if self._proc.stderr else ""
            raise RuntimeError("calcu ended: " + err.strip())
        if self._json:
            reply = json.loads(first)
        else:
            reply = self._plain(first.rstrip("\n"), timeout)
        if check and not reply["done"]:
            raise Refused(reply["reason"])
        return reply

    def _plain(self, first: str, timeout: Optional[float]) -> dict[str, Any]:
        if first.startswith("refused: "):
            return {"done": False, "lines": [], "reason": first[len("refused: "):]}
        lines: list[str] = []
        if first.startswith("done "):
            n = int(first[len("done "):])
            lines = [self._line(timeout).rstrip("\n") for _ in range(n)]
        return {"done": True, "lines": lines, "reason": None}

    def close(self) -> int:
        """Sends quit and waits for the end; returns the exit status."""
        if self._proc.poll() is None:
            try:
                assert self._proc.stdin
                self._proc.stdin.write("quit\n")
                self._proc.stdin.close()
            except (BrokenPipeError, OSError):
                pass
        status = self._proc.wait()
        for f in (self._proc.stdout, self._proc.stderr):
            if f:
                f.close()
        return status

    def __enter__(self) -> "Calcu":
        return self

    def __exit__(self, *exc: object) -> None:
        self.close()
