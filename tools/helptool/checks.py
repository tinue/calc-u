"""The checks of a product's help (LIB-16 design §3.5, C1 to C9)."""

import os
import re
from pathlib import Path
from urllib.parse import unquote

from . import markdown
from .manifest import ASIDE, PICTURE_SUFFIXES

_SCHEME = re.compile(r"^[A-Za-z][A-Za-z0-9+.-]*:")
_FOLLOWS = re.compile(r"^<!--\s*follows:\s*(.*?)\s*-->$")
_AVOID = re.compile(r"^_Avoid_:\s*(.*)$")
_TERM = re.compile(r"^\*\*([^*]+)\*\*:?\s*$")


class Report:
    def __init__(self, root):
        self.root = Path(root)
        self.failures = []
        self.warnings = []

    def _where(self, path, line):
        try:
            shown = Path(path).resolve().relative_to(self.root.resolve())
        except ValueError:
            shown = path
        return f"{shown}:{line}" if line else f"{shown}"

    def fail(self, path, line, message):
        self.failures.append(f"{self._where(path, line)}: {message}")

    def warn(self, path, line, message):
        self.warnings.append(f"{self._where(path, line)}: {message}")


class Resolved:
    """Where a link of a help file leads."""

    def __init__(self, kind, path=None, anchor=None):
        self.kind = kind        # "external", "file", "missing", "chapter-missing"
        self.path, self.anchor = path, anchor


def resolve(help_, source, target):
    """Resolves a link target of `source` (AC-15, EC-2, EC-3; design §3.2)."""
    if target.startswith("chapter:"):
        rest = target[len("chapter:"):]
        number, _, anchor = rest.partition("#")
        try:
            chapter = help_.chapters[int(number) - 1]
        except (ValueError, IndexError):
            return Resolved("chapter-missing")
        if not chapter.product:
            return Resolved("chapter-missing")
        return Resolved("file", chapter.product[0], anchor or None)
    if _SCHEME.match(target):
        return Resolved("external")
    path_part, _, anchor = target.partition("#")
    path = Path(source) if not path_part else (Path(source).parent / unquote(path_part))
    path = Path(os.path.normpath(path))
    return Resolved("file", path, anchor or None)


def _docs(help_):
    docs = {}
    for _, path, _ in help_.files():
        if path.suffix == ".md" and path.is_file():
            docs[path] = markdown.Document(path.read_text(encoding="utf-8"))
    return docs


def run(help_, report):
    """Runs every check; returns the parsed documents for the build."""
    known = {path for _, path, _ in help_.files()}
    _c1_chapters(help_, report)
    docs = _docs(help_)
    used_pictures = set()
    for path, doc in docs.items():
        shared = help_.shared_dir in path.parents
        _c2_links(help_, report, path, doc, known, docs, used_pictures, shared)
        if shared:
            _c5_shared_words(help_, report, path, doc)
    _c6_follows(help_, report)
    _c3_c4_files(help_, report, known, used_pictures)
    _c7_settings(help_, report, docs)
    _c8_glossary(help_, report, docs)
    return docs, used_pictures


def _c1_chapters(help_, report):
    shared_path = help_.shared_dir / "help.yaml"
    # AC-1's chapters are the shared manifest's: numbered 1 to n in order.
    order = [c.number for c in help_.chapters]
    if not all(isinstance(n, int) for n in order) or order != sorted(order) or len(order) != len(set(order)):
        report.fail(shared_path, 0, f"the chapters are not in order: {order}")
    else:
        for n in sorted(set(range(1, (order[-1] if order else 0) + 1)) - set(order)):
            report.fail(shared_path, 0, f"chapter {n} is missing")
    for chapter in help_.chapters:
        if not chapter.title:
            report.fail(shared_path, 0, f"chapter {chapter.number} has no title")
        if not chapter.parts or set(chapter.parts) - {"shared", "product"}:
            report.fail(shared_path, 0, f"chapter {chapter.number} has the parts {list(chapter.parts)},"
                                        " not shared, product or both")
    product_path = help_.product_dir / "help.yaml"
    for n in help_.extra_product_chapters:
        report.fail(product_path, 0, f"chapter {n} is not one of library/help/help.yaml's (AC-1)")
    for chapter in help_.chapters:
        if "shared" in chapter.parts and not chapter.shared:
            report.fail(shared_path, 0, f"chapter {chapter.number} ({chapter.title}) has no shared part")
        if "shared" not in chapter.parts and chapter.shared:
            report.fail(shared_path, 0, f"chapter {chapter.number} ({chapter.title}) has no shared part by AC-1")
        if "product" in chapter.parts and "shared" not in chapter.parts and not chapter.product:
            report.fail(product_path, 0, f"chapter {chapter.number} ({chapter.title}) has no product part")
        if "product" not in chapter.parts and chapter.product:
            report.fail(product_path, 0, f"chapter {chapter.number} ({chapter.title}) has no product part by AC-1"
                                         " (what differs by model goes to chapter 9, EC-3)")
        for f in chapter.shared + chapter.product:
            where = shared_path if f in chapter.shared else product_path
            if f.suffix != ".md":
                report.fail(where, 0, f"{f.name}: a help file is Markdown (.md) (AC-4)")
            elif not f.is_file():
                report.fail(where, 0, f"{f.name}: not found (chapter {chapter.number})")
    for d in help_.downloads:
        if not d.is_file():
            report.fail(product_path, 0, f"{d.name}: download not found")


def _c2_links(help_, report, path, doc, known, docs, used_pictures, shared):
    for link in doc.links:
        target = link.target
        if not target:
            report.fail(path, link.line, "an empty link")
            continue
        if target.startswith("http:"):
            report.warn(path, link.line, f"{target}: not https")
            continue
        r = resolve(help_, path, target)
        if r.kind == "external":
            continue
        if r.kind == "chapter-missing":
            report.fail(path, link.line, f"{target}: the product has no part of that chapter (EC-3)")
            continue
        if link.picture:
            if shared:
                report.fail(path, link.line, f"{target}: a shared part has no picture, only Mermaid diagrams (AC-2)")
            if r.path.suffix.lower() not in PICTURE_SUFFIXES or not r.path.is_file():
                report.fail(path, link.line, f"{target}: the picture leads nowhere")
                continue
            if not _inside(r.path, help_):
                report.fail(path, link.line, f"{target}: the picture is outside the help")
                continue
            used_pictures.add(r.path)
            continue
        if shared and not target.startswith("chapter:") and help_.product_dir in r.path.parents:
            report.fail(path, link.line, f"{target}: a shared part names no product file; it links with chapter:<n> (AC-2, EC-3)")
            continue
        if r.path.suffix.lower() in PICTURE_SUFFIXES:
            if r.path.is_file() and _inside(r.path, help_):
                used_pictures.add(r.path)
                continue
            report.fail(path, link.line, f"{target}: the picture leads nowhere")
            continue
        if r.path not in known and r.path not in help_.downloads:
            report.fail(path, link.line, f"{target}: leads to no file of this help")
            continue
        if r.anchor:
            target_doc = docs.get(r.path)
            if target_doc is None or r.anchor not in target_doc.anchors():
                report.fail(path, link.line, f"{target}: no heading with the anchor #{r.anchor}")


def _inside(path, help_):
    return help_.shared_dir in path.parents or help_.product_dir in path.parents


def _c3_c4_files(help_, report, known, used_pictures):
    listed = set(known) | set(help_.downloads)
    for folder in (help_.shared_dir, help_.product_dir):
        for f in sorted(folder.rglob("*")):
            if not f.is_file() or f.name.startswith("."):
                continue
            if f.parent == folder and f.name in ASIDE:
                continue
            if f.suffix.lower() in PICTURE_SUFFIXES:
                if f not in used_pictures:
                    report.fail(f, 0, "a picture that no help file uses (AC-7)")
                continue
            if f not in listed:
                report.fail(f, 0, "not in the table of contents: list it in a manifest (AC-7)")


def _word_pattern(word):
    return re.compile(r"(?<![A-Za-z0-9-])" + re.escape(word) + r"(?![A-Za-z0-9])")


def _c5_shared_words(help_, report, path, doc):
    patterns = [(w, _word_pattern(w)) for w in help_.all_names]
    for number, text in enumerate(doc.lines, 1):
        for word, pattern in patterns:
            if pattern.search(text):
                report.fail(path, number, f"a shared part names {word!r}, a word of one product (AC-2)")


def _c6_follows(help_, report):
    for chapter in help_.chapters:
        for f in chapter.shared:
            if not f.is_file():
                continue
            first = next((l.strip() for l in f.read_text(encoding="utf-8").splitlines() if l.strip()), "")
            m = _FOLLOWS.match(first)
            want = ", ".join(chapter.follows)
            if not m:
                report.fail(f, 1, f"a shared part starts with <!-- follows: {want} --> (AC-3)")
                continue
            given = [s.strip() for s in m.group(1).split(",") if s.strip()]
            if set(given) != set(chapter.follows):
                report.fail(f, 1, f"follows {', '.join(given)}; chapter {chapter.number} follows {want} (AC-3)")


def settings_keys(help_):
    """The lines of settings-keys.txt: (side, "[section] key", line)."""
    path = help_.product_dir / "settings-keys.txt"
    if not path.is_file():
        return None
    keys = []
    for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        side, _, key = line.partition(" ")
        keys.append((side, key.strip(), number))
    return keys


def _c7_settings(help_, report, docs):
    keys = settings_keys(help_)
    path = help_.product_dir / "settings-keys.txt"
    if keys is None:
        report.fail(path, 0, "not found: the product's settings keys (AC-14; a product test writes it)")
        return
    chapter = help_.chapters[7]
    texts = {
        "family": "\n".join(docs[f].text for f in chapter.shared if f in docs),
        "product": "\n".join(docs[f].text for f in chapter.product if f in docs),
    }
    for side, key, number in keys:
        if side not in texts:
            report.fail(path, number, f"{side}: not family or product")
            continue
        if f"`{key}`" not in texts[side]:
            part = "shared" if side == "family" else "product"
            report.fail(path, number, f"`{key}` is not described in the {part} part of chapter 8 (AC-14)")


def avoid_words(glossary_path):
    """The words the glossary lists after `_Avoid_:` (the words before a
    parenthesis), less those that are themselves a term of the glossary."""
    if not glossary_path.is_file():
        return []
    words, terms = set(), set()
    for line in glossary_path.read_text(encoding="utf-8").splitlines():
        t = _TERM.match(line.strip())
        if t:
            terms.add(t.group(1).strip().lower())
            continue
        m = _AVOID.match(line.strip())
        if not m:
            continue
        for part in m.group(1).split(","):
            word = part.split("(")[0].strip().strip("\"'`")
            if word:
                words.add(word)
    return sorted(w for w in words if w.lower() not in terms)


def _c8_glossary(help_, report, docs):
    words = avoid_words(help_.root / "docs" / "GLOSSARY.md")
    patterns = [(w, re.compile(r"(?<![\w-])" + re.escape(w) + r"(?![\w-])", re.IGNORECASE)) for w in words]
    for path, doc in docs.items():
        for number, text in doc.prose:
            for word, pattern in patterns:
                if pattern.search(text):
                    report.warn(path, number, f"{word!r}: the glossary says to avoid it (AC-5)")
