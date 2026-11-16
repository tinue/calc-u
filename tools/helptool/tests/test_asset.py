"""LIB-16 L1: the asset and its toc.json (AC-6)."""

import json
import unittest
import zipfile

from helptool.tests.support import HelpTestCase


class AssetTests(HelpTestCase):
    def build(self, *extra):
        out = self.root / "out"
        run = self.tool("build", "demo", "--out", str(out), *extra)
        self.assertPasses(run)
        return out / "Demo-Product-help.zip"

    def test_ac6_zip_holds_files_pictures_downloads_toc(self):
        with zipfile.ZipFile(self.build()) as z:
            names = z.namelist()
        self.assertEqual(names[0], "toc.json")
        for n in ("shared/02-getting-started.md", "shared/10-reference.md", "product/01-introduction.md",
                  "product/09-models.md", "product/images/window.png", "product/downloads/tutorial.preset"):
            self.assertIn(n, names)
        self.assertNotIn("product/help.yaml", names)
        self.assertNotIn("product/settings-keys.txt", names)

    def test_ac6_toc_lists_chapters_files_shared_product_version(self):
        with zipfile.ZipFile(self.build()) as z:
            toc = json.loads(z.read("toc.json"))
        self.assertEqual(toc["product"], "Demo-Product")
        self.assertEqual(toc["version"], "1.2.3")  # from release.yaml's cmake file (D-9)
        self.assertEqual(toc["format-version"], 1)
        self.assertEqual([c["number"] for c in toc["chapters"]], list(range(1, 11)))
        two = toc["chapters"][1]
        self.assertEqual(two["title"], "Getting started")
        self.assertEqual(two["files"], [{"path": "shared/02-getting-started.md", "shared": True},
                                        {"path": "product/02-getting-started.md", "shared": False}])
        nine = toc["chapters"][8]["files"]
        self.assertEqual([f["path"] for f in nine], ["product/09-product-topics.md", "product/09-models.md"])

    def test_ac6_version_option_wins(self):
        with zipfile.ZipFile(self.build("--version", "9.9.9")) as z:
            self.assertEqual(json.loads(z.read("toc.json"))["version"], "9.9.9")

    def test_ac6_two_builds_same_bytes(self):
        first = self.build().read_bytes()
        second = self.build().read_bytes()
        self.assertEqual(first, second)

    def test_ac6_product_by_name_or_short_form(self):
        for name in ("Demo-Product", "demo-product", "77", "demo"):
            self.assertPasses(self.tool("check", name))
        self.assertEqual(self.tool("check", "nothing").status, 2)

    def test_ac6_check_writes_no_asset(self):
        self.assertPasses(self.tool("check", "demo"))
        self.assertFalse((self.root / "build").exists())


if __name__ == "__main__":
    unittest.main()
