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

    def test_d9_version_of_the_xcode_preset(self):
        # As ReleaseIt reads it: the first MARKETING_VERSION is the app's; the
        # test targets after it sit at 1.0.
        self.write(self.root / "products/demo/release.yaml",
                   "name: Demo-Product\nversion:\n  preset: xcode\n  file: App.xcodeproj/project.pbxproj\n"
                   "release:\n  latest: false\n")
        self.write(self.root / "products/demo/App.xcodeproj/project.pbxproj",
                   "\t\t\t\tMARKETING_VERSION = 2.4.0;\n\t\t\t\tMARKETING_VERSION = 2.4.0;\n"
                   "\t\t\t\tMARKETING_VERSION = 1.0;\n")
        with zipfile.ZipFile(self.build()) as z:
            self.assertEqual(json.loads(z.read("toc.json"))["version"], "2.4.0")

    def test_d9_version_of_the_cargo_and_regex_presets_and_an_unknown_one(self):
        release = self.root / "products/demo/release.yaml"
        self.write(release, "version:\n  preset: cargo\n")
        self.write(self.root / "products/demo/Cargo.toml", '[package]\nname = "demo"\nversion = "0.3.1"\n')
        with zipfile.ZipFile(self.build()) as z:
            self.assertEqual(json.loads(z.read("toc.json"))["version"], "0.3.1")
        self.write(release, "version:\n  preset: regex\n  file: VERSION.txt\n  regex: '^v=(\\S+)'\n")
        self.write(self.root / "products/demo/VERSION.txt", "v=4.5.6\n")
        with zipfile.ZipFile(self.build()) as z:
            self.assertEqual(json.loads(z.read("toc.json"))["version"], "4.5.6")
        self.write(release, "version:\n  preset: gradle\n  file: VERSION.txt\n")
        run = self.tool("build", "demo", "--out", str(self.root / "out"))
        self.assertEqual(run.status, 1)
        self.assertIn("the version preset 'gradle' is not one ReleaseIt has", run.err)

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
