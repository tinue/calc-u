"""Tests of the pytest plugin calcu_pytest, on a small suite of their own (pytester), the run report
(library/harness/python/report.py) switched on through it among them."""

from pathlib import Path

from report import LATEST

HERE = Path(__file__).resolve().parent
PYTHONPATH = f"{HERE} {HERE.parents[3] / 'library' / 'harness' / 'python'}"

pytest_plugins = ["pytester"]


def test_suite_with_a_machine_class_and_path_names(pytester):
    pytester.makeini(f"""
        [pytest]
        pythonpath = {PYTHONPATH}
        calcu_paths =
            programs = progs
    """)
    pytester.makeconftest("""
        import pytest
        from machine import Machine

        pytest_plugins = ["calcu_pytest"]

        class MyMachine(Machine):
            def new0(self):
                return self.key("cl").type("NEW0")

        @pytest.fixture
        def machine_class():
            return MyMachine
    """)
    (pytester.path / "progs").mkdir()
    (pytester.path / "progs" / "p.bas").write_text("10 PRINT 1\n")
    pytester.makepyfile("""
        from calcu_pytest import path_names

        def test_own_class_and_names(make_machine, request):
            machine = make_machine(model="PC-1500A").new0().load_basic("$programs/p.bas")
            assert machine.listing() == ["10 PRINT 1"]
            assert machine.path("x") == request.config.rootpath / "x"
            assert path_names(request.config) == {"programs": "progs"}
    """)
    pytester.runpytest("-p", "no:cacheprovider").assert_outcomes(passed=1)


def test_a_wrong_path_name_line_is_a_usage_error(pytester):
    pytester.makeini(f"""
        [pytest]
        pythonpath = {PYTHONPATH}
        calcu_paths =
            no equals sign
    """)
    pytester.makeconftest('pytest_plugins = ["calcu_pytest"]')
    pytester.makepyfile("def test_x(make_machine):\n    make_machine()\n")
    result = pytester.runpytest("-p", "no:cacheprovider")
    result.stdout.fnmatch_lines(["*calcu_paths*not `name = path`*"])


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
