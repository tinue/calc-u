"""A copy of the fixture help tree per test, and the tool run in-process."""

import contextlib
import importlib.machinery
import importlib.util
import io
import shutil
import tempfile
import unittest
from pathlib import Path

TOOLS = Path(__file__).resolve().parents[2]
FIXTURE = Path(__file__).resolve().parent / "fixtures" / "good"
REPO = TOOLS.parent


def _load_entry():
    loader = importlib.machinery.SourceFileLoader("help_entry", str(TOOLS / "help"))
    spec = importlib.util.spec_from_loader("help_entry", loader)
    module = importlib.util.module_from_spec(spec)
    loader.exec_module(module)
    return module


ENTRY = _load_entry()


class Run:
    def __init__(self, status, out, err):
        self.status, self.out, self.err = status, out, err


class HelpTestCase(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.root = Path(self._tmp.name) / "repo"
        shutil.copytree(FIXTURE, self.root)
        self.shared = self.root / "library" / "help"
        self.product = self.root / "products" / "demo" / "help"

    def tearDown(self):
        self._tmp.cleanup()

    def tool(self, *args):
        out, err = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            status = ENTRY.main(list(args) + ["--root", str(self.root)])
        return Run(status, out.getvalue(), err.getvalue())

    def write(self, path, text):
        path = Path(path)
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")

    def append(self, path, text):
        self.write(path, Path(path).read_text(encoding="utf-8") + text)

    def replace(self, path, old, new):
        text = Path(path).read_text(encoding="utf-8")
        self.assertIn(old, text)
        self.write(path, text.replace(old, new))

    def assertPasses(self, run):
        self.assertEqual(run.status, 0, run.err)

    def assertFailsWith(self, run, *fragments):
        self.assertEqual(run.status, 1, f"expected a failure; stdout:\n{run.out}")
        for f in fragments:
            self.assertIn(f, run.err)
