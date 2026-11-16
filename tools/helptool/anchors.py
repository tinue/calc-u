"""The anchor of a Markdown heading by GitHub's rule (LIB-16 AC-15, design §3.4).

The heading's text without Markdown marks, lower case; every character
that is not a letter, a digit, a space, a hyphen or an underscore
removed; each space a hyphen. A second heading with the same anchor in
one file gets `-1`, a third `-2`. The cases are
library/help/anchor_cases.txt, which LIB-19's renderer tests against too.
"""

import re

_LINK = re.compile(r"!?\[([^\]]*)\]\([^)]*\)")
_HTML = re.compile(r"<[^>]+>")
_EMPHASIS_UNDERSCORE = re.compile(r"(?<![A-Za-z0-9])_{1,2}([^_]+?)_{1,2}(?![A-Za-z0-9])")


def heading_text(raw):
    """The text a reader sees of a heading's Markdown source."""
    text = _LINK.sub(r"\1", raw)
    text = _HTML.sub("", text)
    text = text.replace("`", "").replace("*", "")
    text = _EMPHASIS_UNDERSCORE.sub(r"\1", text)
    return text.strip()


def slug(raw):
    text = heading_text(raw).lower()
    kept = "".join(c for c in text if c.isalnum() or c in " -_")
    return kept.replace(" ", "-")


class Slugger:
    """Gives each heading of one file its anchor, counting repeats (github-slugger's rule)."""

    def __init__(self):
        self.occurrences = {}

    def anchor(self, raw):
        original = slug(raw)
        result = original
        while result in self.occurrences:
            self.occurrences[original] += 1
            result = f"{original}-{self.occurrences[original]}"
        self.occurrences[result] = 0
        return result
