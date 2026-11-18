"""The manifests and the manifest subset of YAML."""

import unittest

from helptool import manifest, yamlsubset
from helptool.tests.support import REPO, HelpTestCase


class ManifestTests(HelpTestCase):
    def test_ten_chapters_in_order_with_their_parts(self):
        help_ = manifest.load(self.root, "demo")
        self.assertEqual([c.number for c in help_.chapters], list(range(1, 11)))
        self.assertEqual([c.title for c in help_.chapters],
                         ["Introduction", "Getting started", "Models and machine control", "Media",
                          "Peripherals and paper", "Presets", "Debugger", "Settings", "Product topics", "Reference"])
        parts = {c.number: c.parts for c in help_.chapters}
        self.assertEqual(parts[1], ("product",))
        self.assertEqual(parts[3], ("shared",))
        self.assertEqual(parts[9], ("product",))
        for n in (2, 4, 5, 6, 7, 8, 10):
            self.assertEqual(parts[n], ("shared", "product"))
        self.assertPasses(self.tool("check", "demo"))

    def test_the_repository_manifest_has_the_ten_chapters(self):
        # library/help/help.yaml is the tool's one list of the chapters; here
        # it is held against the table of docs/formats/help.md.
        help_ = manifest.load(REPO, "calc-u-1600")
        s, p, sp = ("shared",), ("product",), ("shared", "product")
        self.assertEqual([(c.number, c.title, c.parts) for c in help_.chapters], [
            (1, "Introduction", p),
            (2, "Getting started", sp),
            (3, "Models and machine control", s),
            (4, "Media", sp),
            (5, "Peripherals and paper", sp),
            (6, "Presets", sp),
            (7, "Debugger", sp),
            (8, "Settings", sp),
            (9, "Product topics", p),
            (10, "Reference", sp),
        ])

    def test_a_chapter_out_of_order_or_with_other_parts_fails(self):
        self.replace(self.shared / "help.yaml", "number: 4, title: Media, parts: [shared, product]",
                     "number: 4, title: Media, parts: [shared, extra]")
        self.assertFailsWith(self.tool("check", "demo"), "chapter 4 has the parts ['shared', 'extra']")
        self.replace(self.shared / "help.yaml", "number: 4,", "number: 12,")
        self.assertFailsWith(self.tool("check", "demo"), "the chapters are not in order")

    def test_a_product_part_of_a_shared_only_chapter_fails(self):
        self.write(self.product / "03-models.md", "# Models\n")
        self.replace(self.product / "help.yaml", "  8: [08-settings.md]", "  3: [03-models.md]\n  8: [08-settings.md]")
        self.assertFailsWith(self.tool("check", "demo"), "chapter 3 (Models and machine control) has no product part by library/help/help.yaml")

    def test_yaml_subset_refuses_what_it_does_not_read(self):
        with self.assertRaises(yamlsubset.YamlError) as e:
            yamlsubset.loads("a: 1\nb: |\n  text\n")
        self.assertEqual(e.exception.line, 2)

    def test_yaml_subset_reads_the_manifest_forms(self):
        data = yamlsubset.loads('x: 1\nl:\n  - { a: 1, b: [p, q] }\n  - plain\nm:\n  3: [f.md]  # c\ns: "1600"\n')
        self.assertEqual(data, {"x": 1, "l": [{"a": 1, "b": ["p", "q"]}, "plain"], "m": {3: ["f.md"]}, "s": "1600"})


if __name__ == "__main__":
    unittest.main()
