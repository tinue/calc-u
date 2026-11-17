<!-- follows: LIB-9 -->

# Settings

Settings are your preferences for a product: the model it starts with,
the startup preset of each model, the folders it uses. They never say
which modules, peripherals or media a machine has; a preset does that
([Presets](06-presets.md)).

## The Settings dialog

**File ▸ Settings…** opens the dialog (in a window without a menu bar, the Settings button; on macOS it is in the
application menu). A change applies at once and is kept; the only
button is Close. The dialog can be used while an operation runs. Its
sections, each shown when the frontend has one of its settings:

| Section | Settings |
|---|---|
| General | Startup model: "Last used" or one of the product's models |
| Startup presets | one row per model: "None", the bundled startup presets for that model, or a file chosen with "Choose…" |
| Storage | the save folder, and the product's further folders |
| Folders | the folder in which each file dialog starts |
| Tracing | the trace folder and the largest trace file, in MB |
| the product's sections | the product's own settings ([Settings of this product](chapter:8)) |

A folder's row shows the folder in use, with `~` for your home folder, a
"Change…" button, and "Reset", which puts the setting back to its
default.

## The settings file

All settings are kept in one text file, `<Product>.ini`, named after the
product as it is written. It is written when the product first starts,
with every key at its default and a comment line above each key, and
again whenever a setting changes. It is read when the product starts,
and not again while it runs: to edit it by hand, quit the product
first.

| Platform | Where the settings file lies |
|---|---|
| macOS, iOS | the product's Application Support folder (`~/Library/Application Support/<Product>/`, or inside the app's container when it is sandboxed) |
| Windows | `%APPDATA%\<Product>\` |
| Linux | `$XDG_CONFIG_HOME/<Product>/`, or `~/.config/<Product>/` when that variable is not set |
| Web | the browser's storage for the page |

No other place is searched. The rules of the file are in the
[reference](10-reference.md#the-settings-file).

- A key that is missing has its default.
- A value its key does not accept counts as missing, so the setting has
  its default; the value stays in the file until you change the setting.
- Keys and sections the product does not know, comments, blank lines
  and the order of the file are kept when the product writes it.
- A file that cannot be read is left untouched; the product starts with
  the defaults and says so in its log.
- A file without its `format-version` line is not read; the product
  starts with the defaults and says so.
- When two copies of the product run, the last one to write wins.

## The family's keys

Every product of the family has these keys, with these meanings and
defaults. A key a frontend has no use for is kept in the file and
ignored. Keys marked "written by the product" are kept up to date by the
product itself.

| Key | Default | Meaning |
|---|---|---|
| `[startup] model` | `last` | The startup model: `last` (the model used last) or a model's name. A value that is neither counts as `last` |
| `[startup] last-model` | empty | The model used last; written by the product. A model built by a preset counts too. If it names no model of the product, the product's fallback model starts |
| `[preset] <model>` | empty | One key per model, named after the model: the startup preset of that model, a path or a bundled preset. Empty: none, so the model is built as the product's built-in default machine |
| `[storage] save-folder` | empty | The save folder, where instances, screenshots, traces and serial links go. Empty: a folder named after the product in your home folder (desktop frontends) |
| `[storage] tape-folder` | empty | The folder for cassette tapes. Empty: `Tapes` in the save folder |
| `[trace] folder` | empty | The folder for traces. Empty: the save folder |
| `[trace] max-file-size-mb` | `50` | The largest trace file, in MB (MiB: 1 MB is 1 048 576 bytes); zero or less counts as 50; read when a trace starts ([Debugger](07-debugger.md#traces)) |
| `[trace] location` | `app-support` | Where traces go in the frontends that offer the choice: `app-support`, `icloud`, or `folder` (then `[trace] folder`) |
| `[serial] link-folder` | empty | The folder for the files of serial links. Empty: the save folder |
| `[folders] <kind>` | empty | One key per kind of file the product opens: the folder its file dialog starts in. Written by the product: the folder last used, or `fixed:<folder>` for a folder set in the Settings dialog, which later picks do not change |
| `[window] <key>` | the product's | The size of the main window and the split between its panels, as plain numbers; written by the product |

Which models stand for `<model>`, which kinds for `<kind>` and which
names for `<key>`, and the product's own keys, are in the product's
part: [Settings of this product](chapter:8).

## Folders

- A folder that does not exist yet is created when something is first
  written there.
- A save folder that cannot be created or written is reported, and
  nothing is written to another folder instead.
- A path is absolute; a leading `~` stands for your home folder.
- In a frontend that keeps files in iCloud, the save folder is its
  iCloud Documents folder, or its local Documents folder when iCloud is
  not available; media are not moved between the two.
- In a Web frontend the save folder lies in the browser's storage for
  the page; screenshots and traces are offered as downloads instead.
