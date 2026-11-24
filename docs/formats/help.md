# Help sources and the help asset

The help of a product (its user guide) as an interface: the Markdown
sources and their two manifests, the help asset `<Product>-help.zip`
that a release carries, and the help tool that checks the sources and
builds the asset.

Terms follow `docs/GLOSSARY.md` (Help, Shared part, Product part, Help
asset, Help tool).

| Thing | Defined in |
|---|---|
| The command line, exit status | `tools/help` |
| Manifests, the version | `tools/helptool/manifest.py` |
| The manifests' YAML subset | `tools/helptool/yamlsubset.py` |
| Headings, links, pictures, code | `tools/helptool/markdown.py` |
| The anchor rule | `tools/helptool/anchors.py`, cases in `library/help/anchor_cases.txt` |
| The checks, link resolution | `tools/helptool/checks.py` |
| The asset, `toc.json` | `tools/helptool/asset.py` |

---

## 1. The sources

| Folder | Holds |
|---|---|
| `library/help/` | the shared parts, one Markdown file per chapter that has one; `help.yaml` (the shared manifest); `anchor_cases.txt` |
| `products/<product>/help/` | the product parts; `help.yaml` (the product manifest); `images/` (pictures); `downloads/` (files the text offers); `settings-keys.txt` |

Every help file is Markdown (`.md`). A shared part names nothing of one
product and has no picture; a Mermaid block is not a picture.

### 1.1 The shared manifest: `library/help/help.yaml`

| Key | Meaning |
|---|---|
| `format-version` | `1` (not checked by the tool) |
| `chapters` | The one list of chapters, in order. Each item a mapping: |
| `number` | The chapter's number: 1 to n, in order, none missing |
| `title` | The chapter's title |
| `parts` | `[shared]`, `[product]` or `[shared, product]`: which parts the chapter has |
| `shared` | The shared files of the chapter, relative to `library/help/`, in reading order. Present exactly when `parts` holds `shared` |

The chapters today:

| # | Title | Parts |
|---|---|---|
| 1 | Introduction | product |
| 2 | Getting started | shared, product |
| 3 | Models and machine control | shared |
| 4 | Media | shared, product |
| 5 | Peripherals and paper | shared, product |
| 6 | Presets | shared, product |
| 7 | Debugger | shared, product |
| 8 | Settings | shared, product |
| 9 | Product topics | product |
| 10 | Reference | shared, product |

### 1.2 The product manifest: `products/<product>/help/help.yaml`

| Key | Meaning |
|---|---|
| `format-version` | `1` (not checked by the tool) |
| `product` | The product's name: `Calc-U-59`, `Calc-U-1600`. Names the asset |
| `short` | The short form (`"59"`, `"1600"`), quoted so that it stays text |
| `site` | The product's site folder (`https://calc-u.org/59/`); not used by the tool |
| `version-from` | The release file whose `version:` gives the asset's version (§4.1), relative to the help folder |
| `names` | The words no shared part may use: the product, its models, every peripheral and module of its catalogue, the makers' and processors' names |
| `chapters` | A mapping from chapter number to the list of the product's files of that chapter, relative to the help folder, in reading order. Only chapters whose `parts` hold `product` |
| `downloads` | Files the text links to and the asset carries, relative to the help folder (`downloads/tutorial.ti59`) |

The tool finds a product by `product`, by `short` or by the name of the
folder under `products/`, in any case.

### 1.3 A chapter

A chapter's files are its shared files, then its product files, each in
manifest order. A chapter whose `parts` hold both may lack the product
part; a chapter with `parts: [product]` needs one.

### 1.4 Links

| Target | Leads to |
|---|---|
| a relative path, with an optional `#anchor` | that file of the help, relative to the file that links (`../../../library/help/04-media.md#name--save`). `#anchor` alone: a heading of the same file |
| `chapter:<n>`, with an optional `#anchor` | the first product file of chapter n of the product being checked or built. A shared part's only way to point at a product part |
| a target with a scheme (`https:`, `mailto:`) | outside the help; not followed |

A link is `[text](target)`, a picture `![alt](target)` or
`<img src="target">`. Links inside code spans and fenced blocks are not
links. Pictures (`.png` `.jpg` `.jpeg` `.gif` `.svg` `.webp`) lie in a
help folder; in practice in the product's `images/`.

### 1.5 Anchors

The anchor of a heading is GitHub's:

1. The heading's text as a reader sees it: link and picture marks reduced
   to their text, HTML tags, `` ` `` and `*` removed, `_`/`__` emphasis
   reduced to its text.
2. Lower case.
3. Every character that is not a letter, a digit, a space, `-` or `_`
   removed.
4. Each space becomes `-`.
5. Within one file, a second heading with the same anchor gets `-1`, a
   third `-2`, and so on.

`library/help/anchor_cases.txt`: `#` lines are comments; each other line
is a heading's source text, a tab, its anchor; a blank line starts a new
file, so repeats count within a group. The site's renderer tests against
the same list.

| Heading | Anchor |
|---|---|
| `Name & Save` | `name--save` |
| ``The `[preset]` section`` | `the-preset-section` |
| `File ▸ Open…` | `file--open` |
| `Größe und Übersicht` | `größe-und-übersicht` |

### 1.6 `settings-keys.txt`

The settings keys a product declares, written by a test of the product
from its settings declaration (`library/testing/SettingsKeysFile.hpp`). Blank lines and `#` lines are ignored.
Each other line:

```
<side> [<section>] <key>
```

| `<side>` | The key is described in |
|---|---|
| `family` | the shared part of chapter 8 |
| `product` | the product part of chapter 8 |

A family key whose name the product gives is written once in its
general form (`family [preset] <model>`, `family [folders] <kind>`), and
the product's names as product lines (`product [preset] TI-59`).

## 2. The help tool

```
tools/help check <product>                           the checks, no asset
tools/help build <product> [--version V] [--out D]   the checks, then the asset
                           [--quiet-warnings]
```

`<product>` is the product's name or short form, any case (`Calc-U-1600`,
`1600`). `build` writes the asset only when no check fails. `--out`
defaults to `build/help/` of the repository; `--version` to the
product's version (§4.1). `--quiet-warnings` prints the number of
warnings instead of each.

| Exit status | When |
|---|---|
| 0 | no failure (warnings do not count) |
| 1 | a check failed, a manifest cannot be read, or the version cannot be read |
| 2 | a usage error, or no product of that name has a help |

A failure names the file and, where it has one, the line
(`<path>:<line>: <message>`, on standard error). Warnings go to standard
output as `warning: …`.

## 3. What `check` checks

All fail, except the last row.

| Subject | Rule |
|---|---|
| Chapters | numbered 1 to n in order, none missing, each with a title and `parts` of `shared`, `product` or both; each has the parts its `parts` name, and no others; the product manifest names no chapter the shared manifest lacks |
| Files | every file a manifest lists is a `.md` file that exists; every download exists |
| Links | no empty link; every relative link leads to a help file or a download of this product, and its `#anchor` to a heading there; every `chapter:<n>` resolves (the product has a part of chapter n) |
| Pictures | every picture link leads to a picture file inside the help folders; every picture file in the help folders is used by some help file |
| Listing | every file under the two help folders is listed in a manifest (aside: the manifests, `anchor_cases.txt`, `settings-keys.txt`, hidden files, pictures) |
| Shared parts | no word of any product's `names` (whole words, case as written); no picture; no link to a product file other than by `chapter:<n>` |
| No source | no prose or code span names a path into the source code (`frontends/`, `core/`, `app/`, `library/`, or a product's `frontends/`, `core/`, `app/`, standing on its own); no link into those folders of the source repository on GitHub; no milestone `M-<n>` |
| Settings | `settings-keys.txt` exists, and every key in it appears as `` `[section] key` `` in the part of chapter 8 its side names |
| Warnings | an `http:` link; a word that `docs/GLOSSARY.md` lists after `_Avoid_:` (the words before a parenthesis, less those that are terms themselves), matched in prose in any case |

## 4. The help asset: `<Product>-help.zip`

`build` writes `<out>/<product>-help.zip` (`Calc-U-59-help.zip`,
`Calc-U-1600-help.zip`).

```
toc.json
shared/<the shared files>
product/<the product files>
product/images/<the pictures used>
product/downloads/<the downloads>
```

A source file keeps its path below its help folder: below `shared/` for
`library/help/`, below `product/` for `products/<product>/help/`. Only
pictures that a help file uses are in the asset.

Entries in order: `toc.json`, the help files in table-of-contents order,
then the pictures and downloads sorted by path. Every entry is deflated,
dated 1980-01-01 00:00:00, mode 0644; two builds of one tree give the
same bytes.

Every relative and `chapter:` link is rewritten to the target's place in
the asset, relative to the linking file there; a link to a heading of
the same file becomes `#anchor`. Links with a scheme stay.

### 4.1 The version

`--version`, else read as ReleaseIt reads it: the `version:` mapping of
the file `version-from` names (`preset`, `file`, `regex`), and the first
group of the preset's pattern in that file:

| `preset` | File when `file` is not given | Pattern |
|---|---|---|
| `xcode` | (needs `file`) | `MARKETING_VERSION = ([^;\s]+);`, the first match |
| `cmake` | `CMakeLists.txt` | `^set\(APP_VERSION "([^"]+)"\)` |
| `cargo` | `Cargo.toml` | `^version\s*=\s*"([^"]+)"` |
| `regex` | (needs `file`) | `regex` |

`file` is relative to the release file.

### 4.2 `toc.json`

JSON, UTF-8 (non-ASCII written as is), indented by 2, ending in a
newline. Keys in this order:

```json
{
  "product": "Calc-U-1600",
  "version": "1.0.0",
  "format-version": 1,
  "chapters": [
    {
      "number": 1,
      "title": "Introduction",
      "files": [
        { "path": "product/01-introduction.md", "shared": false }
      ]
    },
    {
      "number": 2,
      "title": "Getting started",
      "files": [
        { "path": "shared/02-getting-started.md", "shared": true },
        { "path": "product/02-getting-started.md", "shared": false }
      ]
    }
  ]
}
```

| Key | Meaning |
|---|---|
| `product` | the product manifest's `product` |
| `version` | §4.1 |
| `format-version` | `1`: the version of this layout |
| `chapters` | every chapter of the shared manifest, in order, also one without files for this product |
| `number`, `title` | as in the shared manifest |
| `files` | the chapter's files in reading order (§1.3): `path` in the asset, `shared` true for a shared part |

## 5. Consumers

calc-u.org pins one release's help asset per product by its address,
its version and its sha256, and renders its Markdown and Mermaid itself,
each chapter at an address of its own (see `docs/help-site.md`).
