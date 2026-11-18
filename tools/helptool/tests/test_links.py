"""Links: resolved, checked and rewritten into the asset."""

import unittest
import zipfile

from helptool.tests.support import HelpTestCase


class LinkTests(HelpTestCase):
    def built(self, name):
        out = self.root / "out"
        self.assertPasses(self.tool("build", "demo", "--out", str(out)))
        with zipfile.ZipFile(out / "Demo-Product-help.zip") as z:
            return z.read(name).decode("utf-8")

    def test_relative_links_rewritten_into_the_asset(self):
        intro = self.built("product/01-introduction.md")
        self.assertIn("(../shared/02-getting-started.md#the-control-bar)", intro)
        self.assertIn("(images/window.png)", intro)
        self.assertIn("(downloads/tutorial.preset)", intro)
        models = self.built("shared/03-models.md")
        self.assertIn("(../product/09-product-topics.md)", models)  # chapter:9
        start = self.built("shared/02-getting-started.md")
        self.assertIn("(#the-control-bar)", start)
        self.assertIn("(04-media.md#name--save)", start)
        self.assertIn("(https://example.org/x)", start)

    def test_link_to_missing_file_heading_picture_fails_with_place(self):
        self.append(self.product / "02-getting-started.md", "\n[gone](09-gone.md)\n")
        run = self.tool("check", "demo")
        self.assertFailsWith(run, "products/demo/help/02-getting-started.md:5", "09-gone.md: leads to no file")

        self.replace(self.product / "02-getting-started.md", "(09-gone.md)", "(09-models.md#no-such-heading)")
        self.assertFailsWith(self.tool("check", "demo"), "no heading with the anchor #no-such-heading")

        self.replace(self.product / "02-getting-started.md", "[gone](09-models.md#no-such-heading)", "![gone](images/gone.png)")
        self.assertFailsWith(self.tool("check", "demo"), "images/gone.png: the picture leads nowhere")

    def test_a_link_into_the_shared_part_follows_the_github_anchor(self):
        self.append(self.product / "02-getting-started.md", "\n[x](../../../library/help/04-media.md#name-save)\n")
        self.assertFailsWith(self.tool("check", "demo"), "#name-save")

    def test_external_link_not_followed(self):
        self.append(self.product / "02-getting-started.md",
                    "\n[site](https://calc-u.org/nowhere/) [mail](mailto:a@b.c) [repo](https://github.com/x/y#z)\n")
        run = self.tool("check", "demo")
        self.assertPasses(run)
        self.append(self.product / "02-getting-started.md", "\n[old](http://example.org/)\n")
        run = self.tool("check", "demo")
        self.assertPasses(run)
        self.assertIn("not https", run.out)

    def test_chapter_link_resolves_to_product_part(self):
        self.replace(self.shared / "03-models.md", "(chapter:9)", "(chapter:9#reset)")
        self.assertFailsWith(self.tool("check", "demo"), "no heading with the anchor #reset")  # 09-product-topics.md has none
        self.replace(self.shared / "03-models.md", "(chapter:9#reset)", "(chapter:10)")
        self.assertPasses(self.tool("check", "demo"))

    def test_chapter_link_without_product_part_fails(self):
        self.replace(self.shared / "03-models.md", "(chapter:9)", "(chapter:5)")
        self.assertFailsWith(self.tool("check", "demo"), "chapter:5: the product has no part of that chapter")

    def test_shared_part_naming_a_product_file_fails(self):
        self.append(self.shared / "03-models.md", "\n[x](../../products/demo/help/09-models.md)\n")
        self.assertFailsWith(self.tool("check", "demo"), "a shared part names no product file")


if __name__ == "__main__":
    unittest.main()
