"""What the checks need of a Markdown file (LIB-16 design §2.1): headings,
links, pictures and fenced blocks. No renderer."""

import re

from . import anchors

_FENCE = re.compile(r"^\s{0,3}(`{3,}|~{3,})\s*([\w-]*)")
_HEADING = re.compile(r"^\s{0,3}(#{1,6})\s+(.*?)\s*#*\s*$")
# [text](target) and ![alt](target); the target ends at whitespace or ')'
# (an optional "title" after it is passed over).
_LINK = re.compile(r"(!?)\[((?:[^\[\]]|\[[^\]]*\])*)\]\(\s*<?([^)\s>]*)>?(?:\s+\"[^\"]*\")?\s*\)")
_IMG_TAG = re.compile(r"<img\b[^>]*\bsrc=\"([^\"]+)\"", re.IGNORECASE)
_CODE_SPAN = re.compile(r"(`+)(.+?)\1")


class Link:
    def __init__(self, line, target, picture, text):
        self.line, self.target, self.picture, self.text = line, target, picture, text


class Document:
    def __init__(self, text):
        self.text = text
        self.lines = text.splitlines()
        self.headings = []      # (line, raw text, anchor)
        self.links = []         # Link
        self.fences = []        # (line, info string)
        self.prose = []         # (line, text without code): the words a reader reads
        self._parse()

    def anchors(self):
        return {a for _, _, a in self.headings}

    def _parse(self):
        slugger = anchors.Slugger()
        fence = None
        in_comment = False
        for number, line in enumerate(self.lines, 1):
            m = _FENCE.match(line)
            if fence:
                if m and m.group(1)[0] == fence[0] and len(m.group(1)) >= len(fence):
                    fence = None
                continue
            if m:
                fence = m.group(1)
                self.fences.append((number, m.group(2)))
                continue
            # HTML comments are not prose (the `follows:` line among them).
            visible = line
            if in_comment:
                end = visible.find("-->")
                if end < 0:
                    continue
                visible = visible[end + 3:]
                in_comment = False
            while "<!--" in visible:
                start = visible.find("<!--")
                end = visible.find("-->", start + 4)
                if end < 0:
                    visible = visible[:start]
                    in_comment = True
                    break
                visible = visible[:start] + visible[end + 3:]
            h = _HEADING.match(visible)
            if h:
                self.headings.append((number, h.group(2), slugger.anchor(h.group(2))))
            no_code = _CODE_SPAN.sub(" ", visible)
            for lm in _LINK.finditer(no_code):
                self.links.append(Link(number, lm.group(3), lm.group(1) == "!", lm.group(2)))
            for im in _IMG_TAG.finditer(no_code):
                self.links.append(Link(number, im.group(1), True, ""))
            self.prose.append((number, no_code))


def rewrite_links(text, rewrite):
    """Text with every link target outside code passed through rewrite(target)
    (which returns the new target, or the same one)."""
    out = []
    fence = None
    for line in text.split("\n"):
        m = _FENCE.match(line)
        if fence:
            if m and m.group(1)[0] == fence[0] and len(m.group(1)) >= len(fence):
                fence = None
            out.append(line)
            continue
        if m:
            fence = m.group(1)
            out.append(line)
            continue
        # Code spans keep their text: rewrite only between them.
        pieces, last = [], 0
        for cm in _CODE_SPAN.finditer(line):
            pieces.append(_rewrite_piece(line[last:cm.start()], rewrite))
            pieces.append(cm.group(0))
            last = cm.end()
        pieces.append(_rewrite_piece(line[last:], rewrite))
        out.append("".join(pieces))
    return "\n".join(out)


def _rewrite_piece(piece, rewrite):
    def link(m):
        new = rewrite(m.group(3))
        start, end = m.start(3) - m.start(0), m.end(3) - m.start(0)
        whole = m.group(0)
        return whole[:start] + new + whole[end:]

    piece = _LINK.sub(link, piece)

    def tag(m):
        whole = m.group(0)
        start, end = m.start(1) - m.start(0), m.end(1) - m.start(0)
        return whole[:start] + rewrite(m.group(1)) + whole[end:]

    return _IMG_TAG.sub(tag, piece)
