# The bundle file

What a product bundles from its `examples/` folder, and how the bundled
presets are shown. One file per product, `examples/bundle.yaml` in its
part of the source repository. Every distribution that bundles examples
(the Qt6, Swift and Web frontends) is built from it, and bundles exactly
what it lists: nothing else from `examples/` comes along. The examples
folder as a whole is not a release file; the documentation explains how
to get every example from the source repository (user, 2026-11-13, B-27
of `docs/walkthroughs/presets.md`).

The bundle is a selection. Examples that are not listed stay in the
source repository only.

## 1. Keys

| Key | Required | Meaning |
|---|---|---|
| `format` | yes | `example-bundle` |
| `format-version` | yes | `1` |
| `startup` | yes | The startup presets, one entry each (§2), in the order in which they are offered |
| `groups` | yes | The groups of examples, in the order in which they are shown; each has `group` (its name, as shown) and `examples` (its entries, §2, in the order in which they are shown) |

The bundle file has no name tag and no `description` (README §2): it is
never shown.

## 2. Entries

An entry is a mapping:

| Key | Required | Meaning |
|---|---|---|
| `preset` | yes | The preset, as a reference that accepts `file:` only (README §3) |
| `files` | no | Every other file the preset needs, each a reference that accepts `file:` only: a BASIC or program file, a machine-code file, a magnetic card, a floppy disk, a cassette and its WAV, a module definition |

- A path is relative to the folder of the bundle file and stays inside
  it.
- A preset is listed once, as a startup preset or as an example, never
  both.
- The name, the description and the model of an entry come from its
  preset (`preset-name`, else the file stem; `description`; `model`).
- A bundled preset is reached by its path inside the bundle, its
  **bundle path** (`examples:<path>`, glossary).

```yaml
format: example-bundle
format-version: 1
startup:
  - preset: { file: startup/default-pc1600.pc1600 }
groups:
  - group: BASIC
    examples:
      - preset: { file: basic/hanoi.pc1600 }
        files:
          - { file: basic/hanoi.bas }
```

## 3. Checks

The test suite fails unless every listed file exists, and every file a
listed preset names is listed with it (or is a bundled medium or module
of the product).
