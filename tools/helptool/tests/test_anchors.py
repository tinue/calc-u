"""Anchors by GitHub's rule, against library/help/anchor_cases.txt."""

import unittest

from helptool import anchors
from helptool.tests.support import REPO


def cases():
    """Groups of (heading, anchor): a blank line starts a new file."""
    groups, current = [], []
    for line in (REPO / "library/help/anchor_cases.txt").read_text(encoding="utf-8").splitlines():
        if line.startswith("#"):
            continue
        if not line.strip():
            if current:
                groups.append(current)
            current = []
            continue
        heading, _, anchor = line.partition("\t")
        current.append((heading, anchor))
    if current:
        groups.append(current)
    return groups


class AnchorTests(unittest.TestCase):
    def test_every_line_of_anchor_cases(self):
        groups = cases()
        self.assertGreater(sum(len(g) for g in groups), 10)
        for group in groups:
            slugger = anchors.Slugger()
            for heading, anchor in group:
                with self.subTest(heading=heading):
                    self.assertEqual(slugger.anchor(heading), anchor)


if __name__ == "__main__":
    unittest.main()
