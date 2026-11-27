# A folder as a drive (PC-1600)

**File ▸ Host Drive…** makes a folder on your computer a PC-1600
drive, `S3:`. It is also `Y:` when no CE-1600P or CE-1600F is attached.
BASIC uses it like a RAM-disk drive, and the files are ordinary files in the
folder, stored byte for byte. The menu item is there on the PC-1600 only,
and not in the Web frontend.

In short:
- **Commands:** `LOAD`, `SAVE`, `BLOAD`, `BSAVE`, `OPEN`, `FILES`, `KILL`,
  `NAME`, `COPY`, `SET` and `DSKF` work on it.
- **Names:** only 8.3 names the PC-1600 accepts are shown, in capitals.
  Longer names stay hidden and are never touched.
- **Subfolders:** as on a MEP USB drive, `CDIR "GAMES"` changes the current
  folder (`CDIR ".."` goes up, `CDIR "/"` to the top; `\` works like `/`) and `LDIR` lists the
  subfolders. All file commands work in the current folder. Switching the
  calculator off returns to the top.
- **MEP programs:** tools written for the MEP module, such as FILEX, work on
  `S3:`.
- **Dates:** the PC-1600 shows each file's month, day and time. Your
  computer keeps the full date.
- **Refused:** `INIT "S3:"`, so a format can never wipe the folder.
- **In a preset:** `host-drive: { folder: <folder> }` mounts a folder
  ([Preset sections](10-preset-sections.md)).

The host drive is a Calc-U-1600 peripheral with no Sharp original. It is
modelled on the MEP rev3 module, a USB drive for the PC-1600's 60-pin
connector. To software it looks like a MEP rev3 USB drive: the same device
names, subdirectories through `CDIR` / `LDIR`, and the MEP's fixed entries,
so MEP programs such as FILEX work on it. The MEP is credited in
[THIRD-PARTY-NOTICES.md](https://github.com/tinue/Calc-U/blob/main/products/calc-u-1600/THIRD-PARTY-NOTICES.md#acknowledgments).

## What works

| BASIC | Notes |
|---|---|
| `LOAD` / `SAVE` / `BLOAD` / `BSAVE` | Files are stored byte for byte, exactly as the PC-1600 writes them: tokenized BASIC, `SAVE ,A` text, BSAVE headers |
| `OPEN` … `FOR INPUT` / `OUTPUT` / `APPEND`, `INPUT#`, `PRINT#`, `CLOSE` | Set `MAXFILES` first, as for every device |
| `FILES` / `LFILES` | Only 8.3 names, see below |
| `KILL`, `NAME`, `COPY`, `SET "…","P"` / `"I"` / `" "` | Wildcards work as on the RAM disk: every matching file, protected ones skipped (error 159) |
| `DSKF("S3:")` | The host volume's free space, capped at 2 147 450 880 bytes, the most DSKF can compute |
| `INIT "S3:"` | Refused: a format must never wipe a host folder |
| `CDIR "path"` | Changes the current directory and shows the prompt, e.g. `S3:/DEV/ASM>`, `S3:>` at the top. `/` (or `\`) starts at the top folder, `..` goes up, `.` stays. A folder that isn't there, or `..` at the top, is error 152 and the directory stays. A prompt longer than 26 characters shows `...` and the end of the path |
| `LDIR` | Lists the subdirectories of the current directory, in the FILES format |

Nothing is converted on the way. A text file written on the computer needs
CR LF line ends to read line by line with `INPUT#`.

## Which host files are visible

The PC-1600 sees regular files whose name fits 8.3 and uses only the
characters its file-name parser accepts:
letters, digits and ``` `@#$%^&()-_{}' ```.

- Names are shown upper-case: `prog.bas` is `PROG.BAS`.
- Long names, extra dots, spaces, non-ASCII names and dotfiles are not shown
  and are never touched.
- Subdirectories follow the same name rule. FILES lists only the files of the
  current directory; `LDIR` lists its subdirectories. Symbolic links to
  folders are left out, so the drive never leaves the mounted folder.
- On a case-sensitive file system, two files that fold to the same name show
  as one. The exact upper-case spelling wins, otherwise the first in sort
  order.
- New files are created upper-case. Writing to an existing file keeps its
  host spelling.

## Attributes and dates

- **P** (protected) is the host file's owner-write permission. SET "P"
  clears it, and SET " " sets it again.
- **I** (invisible) is the macOS hidden flag (`chflags hidden`). On other
  systems it is never set.
- **Directory dates** are the host modification time in local time, the
  year included, as on the MEP. FILES shows only mm/dd hh:mm.
- **On the host side** the file keeps its real time stamp. The OS writes it
  on every change, from the computer's clock.

## Mounting a folder

- **Host Drive…** (PC-1600 only) plugs the drive in with a power
  cycle, as attaching any peripheral does, because the ROM only finds
  modules at power-on; the memory is kept. The dialog starts in
  **File ▸ Settings… ▸ Folders ▸ Host drive folder** (`[folders] host-drive`):
  the folder last mounted by default, or a fixed folder.
- **Host Drive… again** while a folder is mounted replaces the drive's
  folder, with a power cycle as well.
- **File ▸ Unmount "<folder>"** unplugs the drive, with a power cycle.
- The mount survives a change of ROM variant but not a model switch, and is
  not kept across starts.
- **Preset:** `host-drive: { folder: <folder> }` mounts a folder before the
  preset's cold boot. The path is relative to the preset, or starts with
  `~/`. The window then shows the drive as **File ▸ Unmount "<folder>"**
  (the example `dwx/DiskWorks.pc1600`). A preset whose folder does not
  exist is refused before the machine is touched.
- **Without a window:** the harness's `c1600 host-drive <folder>|off`.

## Compared with a real MEP rev3

| Area | MEP rev3 | S3 |
|---|---|---|
| Subdirectories, CDIR / LDIR, the MEP's fixed ROM entries | Yes | Yes |
| Current directory after power-on | Top folder | Top folder |
| APPEND, DSKF, SET, GET LENGTH | ERROR 158 | Supported |
| Files open at once | One for reading, one for writing | As many as MAXFILES |
| Names | 8.3 recommended | Only 8.3 names are visible |
| INIT | Not handled | Refused |

How the drive is built (driver ROM, I/O protocol, FILE functions) is in
[PC1600-Host-Drive-Internals.md](https://github.com/tinue/Calc-U/blob/main/products/calc-u-1600/docs/developer/PC1600-Host-Drive-Internals.md).
