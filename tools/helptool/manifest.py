"""The manifests of a product's help (LIB-16 design §3.1) and the contract
they are checked against (AC-1, AC-3)."""

import re
from pathlib import Path

from . import yamlsubset

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
        # AC-1, AC-3: the chapters, their parts and the specs a shared part
        # follows are library/help/help.yaml's, in its order.
        product_chapters = data.get("chapters") or {}
        for c in shared_data.get("chapters") or []:
            if not isinstance(c, dict):
                continue
            chapter = Chapter(c.get("number"), c.get("title"), c.get("parts") or (), c.get("follows") or ())
            chapter.shared = [self.shared_dir / f for f in c.get("shared") or []]
            chapter.product = [product_dir / f for f in product_chapters.get(chapter.number) or []]
            self.chapters.append(chapter)
        self.extra_product_chapters = [n for n in product_chapters if n not in {c.number for c in self.chapters}]

    def files(self):
        """Every chapter file in table-of-contents order: (chapter, path, shared)."""
        for c in self.chapters:
            for f in c.shared:
                yield c, f, True
            for f in c.product:
                yield c, f, False

    def version(self):
        """The product's version as ReleaseIt reads it (design D-9): the
        first match of its preset's pattern in the `version:` file."""
        if not self.version_from:
            raise HelpError(f"{self.product_dir / 'help.yaml'}: no version-from")
        release = (self.product_dir / self.version_from).resolve()
        if not release.is_file():
            raise HelpError(f"{release}: not found (version-from)")
        given = _version_block(release.read_text(encoding="utf-8"))
        preset = given.get("preset")
        if preset not in VERSION_PRESETS:
            raise HelpError(f"{release}: the version preset {preset!r} is not one ReleaseIt has"
                            f" ({', '.join(VERSION_PRESETS)})")
        default_file, pattern = VERSION_PRESETS[preset]
        if preset == "regex":
            pattern = given.get("regex")
            if not pattern:
                raise HelpError(f"{release}: the version preset regex needs version.regex")
        file = given.get("file") or default_file
        if not file:
            raise HelpError(f"{release}: the version preset {preset} needs version.file")
        source = release.parent / file
        if not source.is_file():
            raise HelpError(f"{source}: not found (version.file of {release.name})")
        try:
            m = re.search(pattern, source.read_text(encoding="utf-8"))
        except re.error as e:
            raise HelpError(f"{release}: version.regex: {e}") from None
        version = m.group(1).strip() if m and m.groups() else ""
        if not version:
            raise HelpError(f"{source}: no version (the {preset} preset reads {pattern})")
        return version


# ReleaseIt's version presets (src/version.rs): the file when release.yaml
# names none, and the pattern whose first match's group is the version. In
# an Xcode project several targets carry MARKETING_VERSION (test targets
# often sit at 1.0); the first one is the app's.
VERSION_PRESETS = {
    "xcode": (None, r"MARKETING_VERSION = ([^;\s]+);"),
    "cargo": ("Cargo.toml", r'(?m)^version\s*=\s*"([^"]+)"'),
    "cmake": ("CMakeLists.txt", r'(?m)^set\(APP_VERSION "([^"]+)"\)'),
    "regex": (None, None),
}


def _version_block(text):
    """The scalars of release.yaml's `version:` mapping (preset, file, regex)."""
    given = {}
    in_version = False
    for line in text.splitlines():
        if re.match(r"^version:\s*(#.*)?$", line):
            in_version = True
            continue
        if not in_version or not line.strip() or line.lstrip().startswith("#"):
            continue
        if not line[0].isspace():
            break
        m = re.match(r"^\s+(preset|file|regex):\s*(.*?)\s*$", line)
        if m:
            value = m.group(2)
            if len(value) >= 2 and value[0] == value[-1] and value[0] in "'\"":
                value = value[1:-1].replace("''", "'") if value[0] == "'" else value[1:-1]
            else:
                value = re.sub(r"\s+#.*$", "", value)
            given[m.group(1)] = value
    return given


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
