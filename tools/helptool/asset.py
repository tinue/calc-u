"""The help asset `<Product>-help.zip` and its toc.json (LIB-16 AC-6; design §3.3)."""

import json
import os
import zipfile
from pathlib import Path, PurePosixPath

from . import checks, markdown

FORMAT_VERSION = 1
_STAMP = (1980, 1, 1, 0, 0, 0)


def asset_path(help_, path):
    """Where a source file of the help goes in the asset."""
    path = Path(path)
    if help_.shared_dir in path.parents:
        return PurePosixPath("shared", *path.relative_to(help_.shared_dir).parts)
    return PurePosixPath("product", *path.relative_to(help_.product_dir).parts)


def toc(help_, version):
    chapters = []
    for c in help_.chapters:
        files = [{"path": str(asset_path(help_, f)), "shared": True} for f in c.shared]
        files += [{"path": str(asset_path(help_, f)), "shared": False} for f in c.product]
        chapters.append({"number": c.number, "title": c.title, "files": files})
    return {"product": help_.product, "version": version, "format-version": FORMAT_VERSION,
            "chapters": chapters}


def _relative(from_file, to_file):
    return os.path.relpath(str(to_file), str(PurePosixPath(from_file).parent)).replace(os.sep, "/")


def rewritten(help_, path, text):
    """The text of a help file with its relative links rewritten to the asset's places (AC-15, D-4)."""
    own = asset_path(help_, path)

    def rewrite(target):
        r = checks.resolve(help_, path, target)
        if r.kind != "file":
            return target
        dest = asset_path(help_, r.path)
        if dest == own and r.anchor:
            return f"#{r.anchor}"
        new = own.name if dest == own else _relative(own, dest)
        return new + (f"#{r.anchor}" if r.anchor else "")

    return markdown.rewrite_links(text, rewrite)


def build(help_, version, out_dir, used_pictures):
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    zip_path = out_dir / f"{help_.product}-help.zip"
    entries = [("toc.json", (json.dumps(toc(help_, version), indent=2, ensure_ascii=False) + "\n").encode("utf-8"))]
    for _, path, _ in help_.files():
        text = path.read_text(encoding="utf-8")
        entries.append((str(asset_path(help_, path)), rewritten(help_, path, text).encode("utf-8")))
    for extra in sorted(set(used_pictures) | set(help_.downloads)):
        entries.append((str(asset_path(help_, extra)), Path(extra).read_bytes()))
    tmp = zip_path.with_suffix(".zip.tmp")
    with zipfile.ZipFile(tmp, "w", zipfile.ZIP_DEFLATED) as z:
        for name, data in entries:
            info = zipfile.ZipInfo(name, date_time=_STAMP)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o644 << 16
            info.create_system = 3
            z.writestr(info, data)
    os.replace(tmp, zip_path)
    return zip_path
