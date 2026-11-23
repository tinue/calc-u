"""Tests of the run report (report.py) and of switching it on through calcu_pytest."""

from collections import defaultdict
from datetime import datetime
from pathlib import Path
from types import SimpleNamespace

from calcu_machine import calcu_program
from report import KEEP_REPORTS, LATEST, calcu_facts, incomplete_reasons, project_commit, render, store

HERE = Path(__file__).resolve().parent
PYTHONPATH = f"{HERE} {HERE.parents[3] / 'library' / 'harness' / 'python'}"

pytest_plugins = ["pytester"]


def counts(**outcomes):
    result = defaultdict(lambda: defaultdict(int))
    result["test_a.py"].update(outcomes)
    return result


def render_with(reasons=(), outcomes=None, failures=(), exit_code=0):
    started = datetime(2026, 11, 1, 12, 0, 0)
    return render("T", started, 1.0, {"Commit": "abc"}, list(reasons), outcomes, list(failures), exit_code)


def test_report_full_pass():
    text = render_with(outcomes=counts(passed=3))
    assert "**PASSED**, complete" in text
    assert "| Commit | `abc` |" in text


def test_report_failure_is_listed():
    text = render_with(outcomes=counts(passed=1, failed=1), failures=[("a::b", "wrong")])
    assert "**FAILED**" in text
    assert "- `a::b`: wrong" in text


def test_report_marks_partial_run():
    assert "PARTIAL: -m not slow" in render_with(["-m not slow"], counts(passed=1))


def test_report_marks_abort():
    assert "PARTIAL: aborted, exit code 2" in render_with(outcomes=counts(passed=1), exit_code=2)


def test_report_without_tests_fails():
    assert "**FAILED**" in render_with(outcomes=defaultdict(lambda: defaultdict(int)))


def test_incomplete_reasons(tmp_path):
    full = SimpleNamespace(markexpr="", keyword="", lf=False, deselect=[], maxfail=0)
    assert incomplete_reasons(full, [str(tmp_path)], tmp_path) == []
    assert incomplete_reasons(full, ["."], tmp_path, tmp_path) == []
    assert incomplete_reasons(full, [str(tmp_path / "test_a.py")], tmp_path)
    assert incomplete_reasons(SimpleNamespace(markexpr="not slow"), [], tmp_path) == ["-m not slow"]
    assert incomplete_reasons(SimpleNamespace(keyword="x", maxfail=1), [], tmp_path) == [
        "-k x",
        "stops after 1 failures",
    ]


def test_store_keeps_last_runs(tmp_path):
    for number in range(KEEP_REPORTS + 3):
        store(tmp_path, f"{number:03d}", str(number))
    runs = sorted(path.name for path in tmp_path.glob("*.md") if path.name != LATEST)
    assert len(runs) == KEEP_REPORTS
    assert runs[0] == "003.md"
    assert (tmp_path / LATEST).read_text() == str(KEEP_REPORTS + 2)


def test_facts_name_the_checkout_and_program(tmp_path):
    facts = calcu_facts()
    assert facts["Calc-U"] != "unknown"
    assert facts["calcu"] == calcu_program()
    assert len(facts["ROM images"]) == 64
    assert project_commit(tmp_path) == "not in a git repository"


def test_report_switched_on_with_facts_of_the_project(pytester):
    pytester.makeini(f"""
        [pytest]
        pythonpath = {PYTHONPATH}
        calcu_report = reports
        calcu_report_title = My suite
    """)
    pytester.makeconftest("""
        pytest_plugins = ["calcu_pytest"]

        def pytest_calcu_report_facts(config):
            return {"Program": "p.bin 1234"}
    """)
    pytester.makepyfile(test_a="def test_ok():\n    pass\n\ndef test_bad():\n    assert 1 == 2\n")
    result = pytester.runpytest("-p", "no:cacheprovider")
    result.assert_outcomes(passed=1, failed=1)
    result.stdout.fnmatch_lines(["report: *.md, **FAILED**, complete"])
    text = (pytester.path / "reports" / LATEST).read_text()
    assert text.startswith("# My suite\n")
    assert "| Program | `p.bin 1234` |" in text
    assert "| `test_a.py` | 1 | 1 | 0 | 0 |" in text
    assert "- `test_a.py::test_bad`: assert 1 == 2" in text


def test_no_report_without_the_setting(pytester):
    pytester.makeini(f"""
        [pytest]
        pythonpath = {PYTHONPATH}
    """)
    pytester.makeconftest('pytest_plugins = ["calcu_pytest"]')
    pytester.makepyfile("def test_ok():\n    pass\n")
    pytester.runpytest("-p", "no:cacheprovider").assert_outcomes(passed=1)
    assert not (pytester.path / "reports").exists()
