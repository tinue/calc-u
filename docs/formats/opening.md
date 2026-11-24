# Opening a file

How a file handed to a product is classified and what it is opened as:
by a drop, by Open With, as the file named when a desktop program
starts, from the file dialog of Load Preset…, or from the harness. The frontend decides nothing about the
file; the app session classifies it and opens it.

Terms follow `docs/GLOSSARY.md` (Open, File kind, Preset, Medium,
Notice).

| Thing | Defined in |
|---|---|
| The command, its request and reply | `library/opening/OpenRequest.hpp` |
| The checks in order, `looksLikePreset`, `wouldTake` | `library/opening/Classify.cpp` |
| What follows each result; several files; a file kept | `library/opening/OpenPart.cpp` |
| How a preset and a medium are opened (the presets and media parts) | `library/opening/OpenPart.cpp` |
| A product's file kind | `library/opening/FileKind.hpp` |
| The start file | `library/opening/StartFile.cpp` |
| Calc-U-59's file kinds | `products/calc-u-59/app/TIFileKinds.cpp` |
| Calc-U-1600's file kinds and size limits | `products/calc-u-1600/app/SharpFileKinds.cpp` |

---

## 1. The command

```
open             {files: [{name, path?, bytes?}], way, upload?, confirmed?}
open-would-take  {name, size?} → {takes}
```

| Key | Meaning |
|---|---|
| `files` | One file; several only from a Web file dialog (§5). Each needs a `name` or a `path` |
| `name` | The file's name without folders. Without it, the last part of `path` |
| `path` | Where the host has the file |
| `bytes` | The file's content. Absent: the host could not read it |
| `way` | `drop`, `open-with`, `start`, `dialog` or `harness`. Absent: `drop` |
| `upload` | The host cannot write the file back (a Web frontend). A preset is then opened from an upload folder (§5), or from its content without one; a medium is imported first |
| `confirmed` | The user said yes to replacing a medium of the same name (§4) |

**Reply.** `{as, file, detail}`:

| `as` | `detail` |
|---|---|
| `preset` | empty |
| `medium` | the place word of the media place it went into, or `imported` |
| `file-kind` | the file kind's name (§3) |
| `kept` | empty: the session was busy or had no machine; the file is opened when it is free (§6) |

A refusal is the reply `done = false` with its sentence, and the same
sentence as a notice; the app log takes it at warn ("A file was not
opened: <sentence>"). Every sentence names the file.

`open-would-take` answers for a drag before the drop: `takes` is true
when the drop would not be refused by size or as hidden, and the name has
a preset extension (when presets are opened), a media suffix, or a file
kind's name rule takes it (`FileKind::takesName`). The size is checked against the `drop` limit,
only when `size` is given.

## 2. The checks, in order

Names are compared in ASCII lower case. The first check that applies
decides.

| # | Check | Result |
|---|---|---|
| 1 | `bytes` absent | refused: "`<name>` could not be read." |
| 2 | more bytes than the way's limit (§2.1) | refused: "`<name>` is larger than `<limit>` and is not opened." |
| 3 | the name starts with `.` | refused: "`<name>` is a hidden file and is not opened." |
| 4 | presets are opened, and the name ends in one of the product's preset extensions | a preset, whatever it holds |
| 5 | the name ends in one of the product's media suffixes | a medium (§4) |
| 6 | the first of the product's file kinds, in its order, that takes the file | that file kind (§3) |
| 7 | presets are opened, and `looksLikePreset(bytes)` | a preset |
| 8 | none | refused: "`<product>` cannot open `<name>`." |

`<product>` is `Calc-U-59` or `Calc-U-1600`. Presets are opened when the
open part is given the presets part. A preset that does not load is
refused with the preset loader's sentence.

### 2.1 Size limits

| Product | `drop` | `open-with` | `start` | `dialog` | `harness` |
|---|---|---|---|---|---|
| Calc-U-59 | none | none | none | none | none |
| Calc-U-1600 | 1 MiB | 1 MiB | 1 MiB | none | none |

`<limit>` is written as `<n> MiB` or `<n> KiB` when it is a whole number
of them, else `<n> bytes` (`sizeText`).

### 2.2 `looksLikePreset`

True when all of these hold, after an optional UTF-8 byte-order mark
(`EF BB BF`):

- some line starts with `model:` in its first column (the first line, or
  a line after a `\n`);
- the content holds no zero byte;
- the content is well-formed UTF-8: no overlong form, no surrogate,
  nothing above U+10FFFF.

### 2.3 Suffixes per product

| Product | Preset extensions | Media suffixes |
|---|---|---|
| Calc-U-59 | `.ti59` `.ti58` `.ti58c` | `.magnetcard.yaml` |
| Calc-U-1600 | `.pc1500` `.pc1500a` `.pc1600` | `.module.yaml` `.floppy.yaml` `.cassette.yaml` |

## 3. File kinds

A product's file kinds are asked in the order of the tables. `takes`
decides on the whole file; `takesName` decides on the name alone and
answers `open-would-take`.

### 3.1 Calc-U-59

Each kind decides by the name alone (`takes` is `takesName`). No two take
the same name.

| Kind name | Takes | What follows |
|---|---|---|
| `asm-overlay` | a name that ends in `.asm` and is longer than it | read into the debug panel as by its Select File |
| `keystroke-program` | a name that ends in `.keystrokes.yaml` and is longer than it | its steps are written into the machine's program memory, as a preset's `program:` writes them, without a cold boot; the partition, the data registers and the steps it does not list stay. Also while paused |
| `rom-module-file` | a name that ends in `.rom.yaml` and is longer than it | refused (§3.3) |
| `constant-memory-file` | the name `ti58c.memory.yaml` | refused (§3.3) |

`.u59` and `.bin` files are taken by nothing and refused by check 8.

### 3.2 Calc-U-1600

Both kinds take every name (`takesName` is always true); the content
decides (`programKindOf`, over `programfile::classify` in
`products/calc-u-1600/core/ProgramFile.hpp`).

| The file is | Kind name |
|---|---|
| a BASIC listing, or tokenized BASIC of the PC-1500 or PC-1600 | `basic-program` |
| machine code behind a CE-158 or PC-1600 header (LH5801 or Z-80) | `machine-code-program` |
| headerless, taken for code by the classifier, and named `….bin` (any case) | `machine-code-program` |
| headerless otherwise (also `.rom`) | neither |
| a cassette WAV: its first readable file | the kind of that file, by the rows above |
| a cassette WAV without a readable file | `basic-program` (its loader says why) |
| empty, or anything else | neither: checks 7 and 8 follow |

What follows:

| Way | Then |
|---|---|
| `harness` | loaded at once: BASIC by the BASIC loader, machine code by the code loader without an address; a WAV's first file. Headerless code is refused (it needs an address) |
| every other way | the stream topic `c1600.program-opened {name, kind, bytes, path}` (`path` empty when the host has none); the frontend runs the flow of File ▸ Load BASIC Program… or Load Machine Code… on it, without their file dialog |

### 3.3 Refusal sentences of the file kinds

| Product | Case | Sentence |
|---|---|---|
| Calc-U-59 | `rom-module-file` | "`<name>` was not opened: this release takes a Solid State ROM module by its ID only." |
| Calc-U-59 | `constant-memory-file` | "`<name>` was not opened: it is Calc-U-59's own constant memory file." |
| Calc-U-59 | `keystroke-program`, no bytes | "`<name>` could not be read." |
| Calc-U-59 | `keystroke-program`, not a valid file or a step beyond the model's last step | "`<name>`: `<the keystroke program reader's reason>`" |
| Calc-U-59 | `asm-overlay` | the ASM overlay reader's sentence |
| Calc-U-1600 | `harness`, the loader refuses | the Sharp loader's refusal for `<name>` |

## 4. Media

A file taken by a media suffix is read by the product's media kind first.

| Case | Result |
|---|---|
| unreadable | refused with the media reader's sentence |
| it fits more than one place now | refused: "`<name>` fits more than one place; choose it in the picker of the place it should go into." |
| it fits no place now, not an upload | refused: "The `<model>` has no place for `<name>` now; a `<kind name>` goes into `<places>`." (`<places>` joined with `, ` and a last ` or `; without any place in the catalogue the sentence ends after "now.") |
| it fits no place now, an upload | imported into the save folder, the same sentence as a notice; reply `detail: imported` |
| one place, which refuses now | refused with that place's sentence; nothing imported |
| one place, not an upload | used from where it lies, written back there unless it is a template |
| one place, an upload | imported into the save folder, then put into the place |

An import whose name is already taken, and not `confirmed`, is a
question, not a refusal: the reply's reason begins with `asks: ` and
holds the question, and no notice is given. The frontend asks; on yes it
sends the same `open` with `confirmed: true`, on no nothing.

## 5. Several files, and an uploaded preset

Several files come only from a Web file dialog, and are one folder only
with an upload folder (`/uploads`). Without one, or without a preset
loader, the first file is opened alone. An uploaded preset alone (one file
with `upload` that check 1 to 3 take as a preset) goes the same way, a
folder of its own.

1. A new folder `<upload folder>/open-<n>` is made (`n` counts up from 1,
   past folders that exist), and every file with bytes is written into
   it under its name without folders.
2. The preset is the first by name (ASCII lower case) among the files with
   a preset extension; when there are none, among the files that
   `looksLikePreset`. Hidden files and files without bytes are passed
   over.
3. With no such file, the first file of the request is opened alone.
   Else the preset passes checks 1 to 3 and is loaded from its path in the
   folder as an upload: a relative `file:` it names is looked up in the
   folder by its last part alone (`cards/a.magnetcard.yaml` is
   `a.magnetcard.yaml`), since the files lie there without their folders;
   `~/` and absolute paths resolve as for any preset. The messages name the
   preset by its file name.

| Case | Sentence |
|---|---|
| the folder cannot be made | "The files could not be put together in `<folder>`: `<reason>`" |
| a file cannot be written | "`<file>` could not be written to `<folder>`: `<reason>`." |
| a relative file the preset names is not in the folder | "The preset `<preset>` names the file `<path>`, which was not chosen with it: choose the preset together with every file it names." |

## 6. A file while the session is busy

When the session runs an operation or has no machine, the request is
kept and replied to with `as: kept`. A later request replaces the kept
one (the app log says so, at info). The kept file is opened when the session is free; a
refusal is then a notice only, and a question becomes the notice
"`<name>` was not opened: `<question>`".

## 7. The ways in

| Way | Qt6 (macOS, Windows, Linux) | Swift (Calc-U-59, iOS and iPadOS) | Web |
|---|---|---|---|
| `drop` | one file dragged onto the window; the window is raised first | none | exactly one file dropped onto the page; a drop of more is ignored. `upload: true` |
| `open-with` | macOS: a file opened from Finder or dropped onto the Dock icon, also at a cold launch | none | none |
| `start` | the last argument after the program's name that does not start with `-` and names an existing file, opened once the window is shown | none | none |
| `dialog` | the file chosen in Load Preset… | the file chosen in the file importer of Load Preset… | the files chosen in Load Preset… (the product's `presetUploads`: the presets and the files a preset can name), `upload: true` |
| `harness` | the harness's `open <path>` | — | — |

A drag shows a pointer that accepts only one file the session would take
(`open-would-take`, with the file's size). Calc-U-59's Qt6 frontend takes
its own options and their values (`--grab`, `--after`, `--pacing-probe`)
out of the arguments before the start file is looked for
(`programArguments`).
