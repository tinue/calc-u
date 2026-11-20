"""The pytest plugin of the driver: the fixtures `make_machine` and `machine_class`, and the path
names of pytest.ini.

    [pytest]
    pythonpath = <checkout>/products/calc-u-1600/tools/machine <checkout>/library/harness/python
    calcu_paths =
        cards = test-cards

The project's conftest.py loads the plugin, `pytest_plugins = ["calcu_pytest"]`; `-p calcu_pytest`
in `addopts` fails, as pytest loads `-p` plugins before it reads `pythonpath`.

The test project root is pytest's rootdir, the folder of pytest.ini. See docs/testing/README.md.
"""

from __future__ import annotations

import itertools

import pytest

from machine import Machine, MachineError


def pytest_addoption(parser):
    parser.addini("calcu_paths", "names for folders: `name = path`, one per line, relative to the root",
                  type="linelist", default=[])


def _path_names(config) -> dict[str, str]:
    names = {}
    for line in config.getini("calcu_paths"):
        name, equals, folder = line.partition("=")
        if not equals or not name.strip() or not folder.strip():
            raise pytest.UsageError(f"calcu_paths: `{line}` is not `name = path`")
        names[name.strip()] = folder.strip()
    return names


@pytest.fixture
def machine_class():
    """The class make_machine makes: Machine. A suite overrides this fixture in its conftest.py with
    a subclass of Machine that has the suite's own steps."""
    return Machine


@pytest.fixture
def make_machine(request, tmp_path, machine_class):
    """make_machine(**options) -> a machine_class in a work folder of its own under tmp_path."""
    root = request.config.rootpath
    names = _path_names(request.config)
    count = itertools.count()
    made: list[Machine] = []

    def make(**options) -> Machine:
        machine = machine_class(tmp_path / f"machine-{next(count)}", project=root, paths=names, **options)
        made.append(machine)
        return machine

    yield make
    for machine in made:
        machine.close()


def path_names(config) -> dict[str, str]:
    """The names of pytest.ini's calcu_paths, for a machine made outside make_machine (for example in
    a session-scoped fixture): Machine(workdir, project=config.rootpath, paths=path_names(config))."""
    return _path_names(config)


__all__ = ["make_machine", "machine_class", "path_names", "MachineError"]
