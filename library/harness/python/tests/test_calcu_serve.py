"""The Python helper against `calcu_test` (LIB-11 design T15; no AC).

    python -I test_calcu_serve.py <calcu_test>
"""

import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))

from calcu_serve import Calcu, Refused  # noqa: E402


def main() -> int:
    program = sys.argv[1]
    failures = []

    def check(cond: bool, what: str) -> None:
        if not cond:
            failures.append(what)

    with Calcu(program, "test") as c:
        # The run starts at key readiness: one run piece of 1667 ticks (LIB-18 D-4).
        check(c.send("ticks")["result"] == {"ticks": 1667, "seconds": 0.01667}, "ticks at the start")
        waited = c.send("wait 1ms")["result"]
        check(waited["ticks"] >= 100, "wait 1ms at 100 kHz: %r" % waited)
        lcd = c.send("display-text")["result"]
        check(lcd["on"] is True and len(lcd["rows"]) == 1, "lcd: %r" % lcd)
        try:
            c.send("no-such-command")
            check(False, "an unknown command was not refused")
        except Refused as e:
            check("unknown command" in str(e), "reason: %s" % e)
        check(c.send("state notices", check=False)["done"], "state notices")
    with Calcu(program, "test", json=False) as c:
        reply = c.send("ticks")
        check(reply["lines"] == ["ticks: 1667", "seconds: 0.016670000"], "plain: %r" % reply)
        check(c.send("key-down 1")["lines"] == [], "plain done")
    # A reply that does not come in time ends the program.
    c = Calcu(program, "test", timeout=1e-6)
    try:
        c.send("wait 10s")
        check(False, "no timeout")
    except TimeoutError:
        pass
    c.close()

    for f in failures:
        print("FAIL", f)
    print("ok" if not failures else "%d failures" % len(failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
