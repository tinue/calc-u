"""The manifests of a product's help (LIB-16 design §3.1) and the contract
they are checked against (AC-1, AC-3)."""

import re
from pathlib import Path

from . import yamlsubset

# AC-1: the ten chapters, their parts; AC-3: the specs a shared part follows.
CHAPTERS = [
    (1, "Introduction", ("product",), ()),
    (2, "Getting started", ("shared", "product"), ("LIB-14", "LIB-5")),
    (3, "Models and machine control", ("shared",), ("LIB-18", "LIB-12")),
    (4, "Media", ("shared", "product"), ("LIB-7", "LIB-13")),
    (5, "Peripherals and paper", ("shared", "product"), ("LIB-2", "LIB-8")),
    (6, "Presets", ("shared", "product"), ("LIB-6",)),
    (7, "Debugger", ("shared", "product"), ("LIB-10",)),
    (8, "Settings", ("shared", "product"), ("LIB-9",)),
    (9, "Product topics", ("product",), ()),
    (10, "Reference", ("shared", "product"), ("LIB-6", "LIB-9")),
]

SHARED_DIR = Path("library/help")
PICTURE_SUFFIXES = {".png", ".jpg", ".jpeg", ".gif", ".svg", ".webp"}
# Files of the help folders that are no part of the asset (C3).
ASIDE = {"help.yaml", "anchor_cases.txt", "settings-keys.txt"}


class HelpError(Exception):
    """A failure that stops the tool before the checks (a manifest it cannot read)."""


class Chapter:
    def __init__(self, number, title, parts, follows):
        self.number, self.title, self.parts, self.follows = number, title, tuple(parts), tuple(follows)
        self.shared = []    # Paths (absolute) of the shared files, in manifest order
        self.product = []   # Paths (absolute) of the product files, in manifest order


class Help:
    """One product's help: the shared side and the product's."""

    def __init__(self, root, product_dir, data, shared_data, all_names):
        self.root = Path(root)
        self.shared_dir = self.root / SHARED_DIR
        self.product_dir = product_dir
        self.product = data.get("product")
        self.short = str(data.get("short", ""))
        self.site = data.get("site")
        self.version_from = data.get("version-from")
        self.names = [str(n) for n in data.get("names") or []]
        self.all_names = all_names
        self.downloads = [product_dir / d for d in data.get("downloads") or []]
        self.shared_manifest = shared_data
        self.product_manifest = data
        self.chapters = []
        shared_chapters = {c.get("number"): c for c in shared_data.get("chapters") or [] if isinstance(c, dict)}
        product_chapters = data.get("chapters") or {}
        for number, title, parts, follows in CHAPTERS:
            chapter = Chapter(number, title, parts, follows)
            s = shared_chapters.get(number) or {}
            chapter.shared = [self.shared_dir / f for f in s.get("shared") or []]
            chapter.product = [product_dir / f for f in product_chapters.get(number) or []]
            self.chapters.append(chapter)
        self.extra_product_chapters = [n for n in product_chapters if n not in {c[0] for c in CHAPTERS}]

    def files(self):
        """Every chapter file in table-of-contents order: (chapter, path, shared)."""
        for c in self.chapters:
            for f in c.shared:
                yield c, f, True
            for f in c.product:
                yield c, f, False

    def version(self):
        """The product's version as ReleaseIt reads it (design D-9)."""
        if not self.version_from:
            raise HelpError(f"{self.product_dir / 'help.yaml'}: no version-from")
        release = (self.product_dir / self.version_from).resolve()
        if not release.is_file():
            raise HelpError(f"{release}: not found (version-from)")
        preset = file = None
        in_version = False
        for line in release.read_text(encoding="utf-8").splitlines():
            if re.match(r"^version:\s*$", line):
                in_version = True
                continue
            if in_version:
                if line and not line[0].isspace():
                    break
                m = re.match(r"^\s+(preset|file):\s*(\S+)", line)
                if m:
                    if m.group(1) == "preset":
                        preset = m.group(2)
                    else:
                        file = m.group(2)
        if preset != "cmake" or not file:
            raise HelpError(f"{release}: the version preset {preset!r} is not one the help tool reads (cmake)")
        source = release.parent / file
        m = re.search(r'set\(APP_VERSION "([^"]+)"\)', source.read_text(encoding="utf-8"))
        if not m:
            raise HelpError(f"{source}: no set(APP_VERSION \"…\")")
        return m.group(1)


def _load(path):
    try:
        return yamlsubset.loads(path.read_text(encoding="utf-8"))
    except yamlsubset.YamlError as e:
        raise HelpError(f"{path}:{e.line}: {e}") from None


def product_manifests(root):
    """(directory, data) of every product that has a help."""
    found = []
    for path in sorted(Path(root).glob("products/*/help/help.yaml")):
        found.append((path.parent, _load(path)))
    return found


def load(root, product):
    """The help of `product`: its name or short form, any case, or its folder's name."""
    root = Path(root)
    shared_path = root / SHARED_DIR / "help.yaml"
    if not shared_path.is_file():
        raise HelpError(f"{shared_path}: not found")
    shared = _load(shared_path)
    manifests = product_manifests(root)
    all_names = set()
    for _, data in manifests:
        all_names.update(str(n) for n in data.get("names") or [])
    wanted = product.lower()
    for directory, data in manifests:
        keys = {str(data.get("product", "")).lower(), str(data.get("short", "")).lower(), directory.parent.name.lower()}
        if wanted in keys:
            return Help(root, directory, data, shared, sorted(all_names))
    known = ", ".join(str(d.get("product")) for _, d in manifests) or "none"
    raise HelpError(f"no help for the product {product!r} (products with a help: {known})")
