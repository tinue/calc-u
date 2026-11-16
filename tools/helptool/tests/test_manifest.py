"""LIB-16 L1: the manifests (AC-1); the manifest subset of YAML."""

import unittest

from helptool import manifest, yamlsubset
from helptool.tests.support import REPO, HelpTestCase


class ManifestTests(HelpTestCase):
    def test_ac1_ten_chapters_in_order_with_their_parts(self):
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

    def test_ac1_the_repository_manifest_is_the_contract(self):
        data = yamlsubset.loads((REPO / "library/help/help.yaml").read_text(encoding="utf-8"))
        given = [(c["number"], c["title"], tuple(c["parts"])) for c in data["chapters"]]
        self.assertEqual(given, [(n, t, p) for n, t, p, _ in manifest.CHAPTERS])

    def test_ac1_a_retitled_chapter_fails(self):
        self.replace(self.shared / "help.yaml", "title: Media,", "title: Cards,")
        self.assertFailsWith(self.tool("check", "demo"), "chapter 4 is titled 'Cards'")

    def test_ac1_a_product_part_of_a_shared_only_chapter_fails(self):
        self.write(self.product / "03-models.md", "# Models\n")
        self.replace(self.product / "help.yaml", "  8: [08-settings.md]", "  3: [03-models.md]\n  8: [08-settings.md]")
        self.assertFailsWith(self.tool("check", "demo"), "chapter 3 (Models and machine control) has no product part by AC-1")

    def test_yaml_subset_refuses_what_it_does_not_read(self):
        with self.assertRaises(yamlsubset.YamlError) as e:
            yamlsubset.loads("a: 1\nb: |\n  text\n")
        self.assertEqual(e.exception.line, 2)

    def test_yaml_subset_reads_the_manifest_forms(self):
        data = yamlsubset.loads('x: 1\nl:\n  - { a: 1, b: [p, q] }\n  - plain\nm:\n  3: [f.md]  # c\ns: "1600"\n')
        self.assertEqual(data, {"x": 1, "l": [{"a": 1, "b": ["p", "q"]}, "plain"], "m": {3: ["f.md"]}, "s": "1600"})


if __name__ == "__main__":
    unittest.main()
