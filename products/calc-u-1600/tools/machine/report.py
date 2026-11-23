"""A Markdown report of a pytest run: which state was tested on what, and whether the run was complete.

calcu_pytest switches it on with `calcu_report = <folder>` in pytest.ini (relative to the test
project root), with `calcu_report_title = <title>` (default: the root's folder name). The folder
gets one report per run and `latest.md`, the last KEEP_REPORTS runs are kept. A project adds facts
of its own with the hook `pytest_calcu_report_facts(config)`, returning a dict, in its conftest.py.

Contributed by spellbound and fitted to the driver.
"""

from __future__ import annotations

import hashlib
import shutil
import subprocess
import time
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path

import pytest

from calcu_machine import CHECKOUT, calcu_program

KEEP_REPORTS = 20
MESSAGE_LENGTH = 160
LATEST = "latest.md"
OUTCOMES = ("passed", "failed", "error", "skipped")
STOPPED_EXIT_CODES = {2, 3, 4}   # interrupted, internal error, usage error
ROMS = CHECKOUT / "products" / "calc-u-1600" / "roms"


def sha256(path) -> str:
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def rom_images_hash(folder: Path = ROMS) -> str:
    """One hash over the ROM images calcu runs on: their names and bytes."""
    digest = hashlib.sha256()
    for path in sorted(folder.iterdir()):
        if path.suffix.lower() in (".bin", ".rom"):
            digest.update(path.name.encode())
            digest.update(path.read_bytes())
    return digest.hexdigest()


def git(directory, *args) -> str:
    try:
        result = subprocess.run(["git", "-C", str(directory), *args], capture_output=True, text=True)
    except OSError:
        return ""
    return result.stdout.strip() if result.returncode == 0 else ""


def project_commit(root) -> str:
    """The commit of the repository the test project is in, marked if the project differs from it."""
    head = git(root, "rev-parse", "--short", "HEAD")
    if not head:
        return "not in a git repository"
    return head + (" + uncommitted changes" if git(root, "status", "--porcelain", "--", ".") else "")


def calcu_facts() -> dict[str, str]:
    """What the suite ran on: the Calc-U checkout, the calcu program, the ROM images."""
    return {
        "Calc-U": git(CHECKOUT, "describe", "--always", "--dirty") or "unknown",
        "calcu": calcu_program(),
        "ROM images": rom_images_hash(),
    }


def incomplete_reasons(option, args, rootpath, start=None) -> list[str]:
    """Why a run does not cover the whole suite; empty for a full run. args are relative to start."""
    reasons = []
    if getattr(option, "markexpr", ""):
        reasons.append(f"-m {option.markexpr}")
    if getattr(option, "keyword", ""):
        reasons.append(f"-k {option.keyword}")
    if getattr(option, "lf", False):
        reasons.append("--lf")
    if getattr(option, "deselect", None):
        reasons.append("--deselect")
    if getattr(option, "maxfail", 0):
        reasons.append(f"stops after {option.maxfail} failures")
    base = Path(start or Path.cwd())
    selected = [arg for arg in args if (base / arg).resolve() != Path(rootpath).resolve()]
    if selected:
        reasons.append("only " + " ".join(selected))
    return reasons


def short_message(report) -> str:
    """One line for a failure: the first line of its exception."""
    crash = getattr(report.longrepr, "reprcrash", None)
    text = crash.message if crash else str(report.longrepr)
    lines = [line.strip() for line in text.splitlines() if line.strip()] or [""]
    return lines[0][:MESSAGE_LENGTH]


def render(title, started, seconds, facts, reasons, counts, failures, exit_code) -> str:
    total = defaultdict(int)
    for file_counts in counts.values():
        for outcome, count in file_counts.items():
            total[outcome] += count
    if exit_code in STOPPED_EXIT_CODES:
        reasons = [*reasons, f"aborted, exit code {exit_code}"]
    ran = sum(total.values())
    verdict = "FAILED" if total["failed"] + total["error"] or not ran else "PASSED"
    scope = "complete" if not reasons else "PARTIAL: " + ", ".join(reasons)

    lines = [f"# {title}", "", f"**{verdict}**, {scope}", "", "| | |", "|---|---|"]
    lines += [f"| Time | {started:%Y-%m-%d %H:%M:%S} UTC, {seconds:.0f} s |"]
    lines += [f"| {name} | `{value}` |" for name, value in facts.items()]
    lines += ["", "| File | " + " | ".join(OUTCOMES) + " |", "|---|" + "---|" * len(OUTCOMES)]
    for name in sorted(counts):
        lines.append(f"| `{name}` | " + " | ".join(str(counts[name][o]) for o in OUTCOMES) + " |")
    lines.append("| Total | " + " | ".join(str(total[o]) for o in OUTCOMES) + " |")
    if failures:
        lines += ["", "## Failures", ""]
        lines += [f"- `{nodeid}`: {message}" for nodeid, message in sorted(failures)]
    return "\n".join(lines) + "\n"


def store(directory: Path, stamp: str, text: str) -> Path:
    """Writes the report and latest.md, keeps the last KEEP_REPORTS runs."""
    directory.mkdir(parents=True, exist_ok=True)
    path = directory / f"{stamp}.md"
    path.write_text(text)
    shutil.copyfile(path, directory / LATEST)
    runs = sorted(item for item in directory.glob("*.md") if item.name != LATEST)
    for old in runs[:-KEEP_REPORTS]:
        old.unlink()
    return path


class Hooks:
    @pytest.hookspec(firstresult=False)
    def pytest_calcu_report_facts(self, config):
        """Facts of the test project for the report, as a dict (name: value)."""


class Reporter:
    """The pytest plugin; on the controlling process only, xdist sends it every result."""

    def __init__(self, title: str, directory: Path):
        self.title = title
        self.directory = Path(directory)
        self.counts = defaultdict(lambda: defaultdict(int))
        self.failures = []
        self.started = datetime.now(timezone.utc)
        self.clock = time.monotonic()
        self.path = None
        self.verdict_line = None

    def pytest_runtest_logreport(self, report):
        outcome = self._outcome(report)
        if outcome:
            self.counts[report.nodeid.split("::")[0].rsplit("/", 1)[-1]][outcome] += 1
        if outcome in ("failed", "error"):
            self.failures.append((report.nodeid, short_message(report)))

    @staticmethod
    def _outcome(report):
        # The test itself in its call; a failed setup or teardown is an error of its own.
        if report.when == "call":
            return report.outcome
        if report.outcome == "failed":
            return "error"
        return "skipped" if report.outcome == "skipped" else None

    def pytest_sessionfinish(self, session, exitstatus):
        config = session.config
        if config.option.collectonly:
            return
        reasons = incomplete_reasons(config.option, config.args, config.rootpath, config.invocation_params.dir)
        facts = {"Test project": project_commit(config.rootpath), **calcu_facts()}
        for extra in config.hook.pytest_calcu_report_facts(config=config):
            facts.update(extra or {})
        seconds = time.monotonic() - self.clock
        text = render(self.title, self.started, seconds, facts, reasons, self.counts, self.failures, int(exitstatus))
        self.path = store(self.directory, f"{self.started:%Y%m%d-%H%M%S}", text)
        self.verdict_line = text.splitlines()[2]

    def pytest_terminal_summary(self, terminalreporter):
        if self.path:
            terminalreporter.write_line(f"report: {self.path}, {self.verdict_line}")
