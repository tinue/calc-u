"""The checks other than links and the manifests' chapters."""

import unittest

from helptool.tests.support import HelpTestCase


class CheckTests(HelpTestCase):
    def test_missing_chapter_fails(self):
        self.replace(self.shared / "help.yaml",
                     "  - { number: 6, title: Presets, parts: [shared, product], shared: [06-presets.md] }\n",
                     "")
        self.assertFailsWith(self.tool("check", "demo"), "library/help/help.yaml", "chapter 6 is missing")

    def test_missing_product_only_chapter_fails(self):
        self.replace(self.product / "help.yaml", "  9: [09-product-topics.md, 09-models.md]\n", "")
        run = self.tool("check", "demo")
        self.assertFailsWith(run, "chapter 9 (Product topics) has no product part")

    def test_file_not_in_toc_fails(self):
        self.write(self.product / "09-stray.md", "# Stray\n")
        self.assertFailsWith(self.tool("check", "demo"), "products/demo/help/09-stray.md", "not in the table of contents")
        (self.product / "09-stray.md").unlink()
        self.write(self.shared / "11-extra.md", "# Extra\n")
        self.assertFailsWith(self.tool("check", "demo"), "library/help/11-extra.md", "not in the table of contents")

    def test_unused_picture_fails(self):
        (self.product / "images" / "spare.png").write_bytes((self.product / "images" / "window.png").read_bytes())
        self.assertFailsWith(self.tool("check", "demo"), "images/spare.png", "a picture that no help file uses")

    def test_shared_part_naming_a_product_word_fails(self):
        self.append(self.shared / "05-peripherals.md", "\nThe DM-Printer prints.\n")
        self.assertFailsWith(self.tool("check", "demo"), "library/help/05-peripherals.md:3", "names 'DM-Printer'")

    def test_a_word_containing_a_name_is_not_the_name(self):
        self.append(self.shared / "05-peripherals.md", "\nThe DM-10 and xDM-1 are other words.\n")
        self.assertPasses(self.tool("check", "demo"))

    def test_picture_in_shared_part_fails(self):
        self.append(self.shared / "05-peripherals.md", "\n![w](../../products/demo/help/images/window.png)\n")
        self.assertFailsWith(self.tool("check", "demo"), "a shared part has no picture")

    def test_chapter_with_shared_part_only(self):
        # Chapters 4 to 7 of the fixture have no product part; 8's goes too.
        self.replace(self.product / "help.yaml", "  8: [08-settings.md]\n", "")
        (self.product / "08-settings.md").unlink()
        self.write(self.product / "settings-keys.txt", "family [startup] model\n")
        self.assertPasses(self.tool("check", "demo"))

    def test_undescribed_settings_key_fails(self):
        self.append(self.product / "settings-keys.txt", "product [debug] other\nfamily [trace] folder\n")
        run = self.tool("check", "demo")
        self.assertFailsWith(run, "`[debug] other` is not described in the product part of chapter 8",
                             "`[trace] folder` is not described in the shared part of chapter 8")
        # A product key in the shared part does not count for the product.
        self.append(self.shared / "08-settings.md", "\n`[debug] other`\n")
        self.assertIn("`[debug] other` is not described in the product part", self.tool("check", "demo").err)

    def test_missing_settings_keys_file_fails(self):
        (self.product / "settings-keys.txt").unlink()
        self.assertFailsWith(self.tool("check", "demo"), "settings-keys.txt", "not found")

    def test_avoid_words_listed_as_warnings_exit_zero(self):
        self.append(self.product / "02-getting-started.md", "\nThe app is an emulator.\n\n```\napp in code\n```\n")
        run = self.tool("check", "demo")
        self.assertPasses(run)
        self.assertIn("02-getting-started.md:5: 'app'", run.out)
        self.assertIn("02-getting-started.md:5: 'emulator'", run.out)
        self.assertNotIn(":8: 'app'", run.out)  # code is not prose
        # "product" is under Avoid for "Model" but is a term itself: no warning.
        self.assertNotIn("'product'", run.out)

    def test_source_path_fails(self):
        cases = [
            ("The cards are in frontends/qt6/resources/cards.\n", "'frontends/'"),
            ("See `products/demo/app/Thing.cpp`.\n", "'products/demo/app/'"),
            ("The [templates](https://github.com/tinue/Calc-U/tree/main/products/demo/frontends/qt6/x).\n",
             "links into the source code"),
            ("Read `library/sessions/AppSession.cpp`.\n", "'library/'"),
            ("[core/Machine](https://example.org/x)\n", "'core/'"),
        ]
        for text, fragment in cases:
            with self.subTest(text=text):
                original = (self.product / "09-models.md").read_text(encoding="utf-8")
                self.append(self.product / "09-models.md", "\n" + text)
                self.assertFailsWith(self.tool("check", "demo"), "09-models.md", fragment)
                self.write(self.product / "09-models.md", original)

    def test_milestone_fails(self):
        self.append(self.shared / "06-presets.md", "\nThis came with M-52.\n")
        self.assertFailsWith(self.tool("check", "demo"), "06-presets.md", "the milestone 'M-52'")
        self.replace(self.shared / "06-presets.md", "\nThis came with M-52.\n", "\nIn `M-7`.\n")
        self.assertFailsWith(self.tool("check", "demo"), "the milestone 'M-7'")

    def test_what_a_user_gets_passes(self):
        self.append(self.product / "09-models.md",
                    "\nIn `products/demo/` run `tools/read_trace.py`; open"
                    " [calc-u.org/59/calc/app/](https://calc-u.org/59/calc/app/), the"
                    " [examples](https://github.com/tinue/Calc-U/tree/main/products/demo/examples) and the"
                    " [notes](https://github.com/tinue/Calc-U/blob/main/products/demo/docs/notes.md)."
                    " A CE-1600M-1 and DM-10 are fine.\n"
                    "\n```\ncmake --build build --target frontends/qt6/app\n```\n")
        self.assertPasses(self.tool("check", "demo"))

    def test_non_markdown_source_fails(self):
        self.write(self.product / "09-notes.html", "<h1>Notes</h1>\n")
        self.replace(self.product / "help.yaml", "09-models.md]", "09-models.md, 09-notes.html]")
        self.assertFailsWith(self.tool("check", "demo"), "09-notes.html: a help file is Markdown")

    def test_mermaid_block_allowed_in_shared_part(self):
        text = (self.shared / "04-media.md").read_text(encoding="utf-8")
        self.assertIn("```mermaid", text)
        self.assertPasses(self.tool("check", "demo"))


if __name__ == "__main__":
    unittest.main()
