# Installing Calc-U-59

Calc-U-59 is free on every system. The Mac, Windows and Linux builds
are the files of a release of the repository
[tinue/calc-u on GitHub](https://github.com/tinue/calc-u/releases) (the
releases of Calc-U-59 are tagged `calc-u-59-v…`); the iPhone and iPad
build is on the App Store; on Android the browser installs the web app.

## iPhone and iPad

1. Install Calc-U-59 from the
   [App Store](https://apps.apple.com/us/app/calc-u-59/id6761413142).
2. Start it and pick the model in the control bar. **Settings ▸ General
   ▸ Startup model** chooses the model it starts with.
3. **More ▸ Load Preset…** opens a `.ti59`, `.ti58` or `.ti58c` file
   with the file picker.

It needs iOS or iPadOS 18 or later. On the iPhone the app stays in
portrait: the calculator fills the screen, and the printer and the
debugger are pages you turn with the arrows in the top corners (the
debugger only with **Settings ▸ Layout ▸ Debug Page in Portrait**). On the
iPad in landscape the calculator, the printer and the debugger are side
by side ([The window on each system](02-getting-started.md#the-window-on-each-system)).
Magnetic cards and the TI-58C's constant memory are kept in the app's
iCloud storage, so the iPhone and the iPad share them.

## Mac, Windows and Linux

The desktop app is the same on all three systems: the calculators, the
printer, cards, presets and the debugger, with folders of your own for
writing and keeping programs.

**Mac**

1. Download `Calc-U-59-mac-aarch64.dmg` from the latest Calc-U-59 release
   on [GitHub](https://github.com/tinue/calc-u/releases).
2. Open the DMG and drag Calc-U-59.app to the Applications folder.
3. Eject the disk image and start the app from Applications.

The app is signed and notarized by Apple, so macOS opens it like any
other program. It needs a Mac with Apple silicon and macOS 15.8 or later.

**Windows**

1. From the latest Calc-U-59 release on
   [GitHub](https://github.com/tinue/calc-u/releases), download
   `Calc-U-59-windows-x86_64.exe` (most PCs) or
   `Calc-U-59-windows-arm64.exe` (Windows on ARM).
2. Run it. The installer is not signed, so Windows SmartScreen may warn
   about an unrecognised app: choose **More info**, then **Run anyway**.
3. Start Calc-U-59 from the Start menu.

**Linux**

1. From the same release, download the AppImage for your machine:
   `Calc-U-59-linux-x86_64.AppImage` for most PCs,
   `Calc-U-59-linux-aarch64.AppImage` for ARM.
2. Make it executable (`chmod +x Calc-U-59-*.AppImage`, or the file's
   Properties dialog) and run it. Nothing else needs installing.

**Where your files go.** Magnetic cards and the TI-58C's constant memory
(`ti58c.memory.yaml`) are kept in the save folder, `Calc-U-59` in your
home folder unless **Settings ▸ Storage ▸ Save folder** names another;
traces go there too unless **Settings ▸ Tracing** names a trace folder.
**Settings ▸ Folders** sets the folder **Load Preset…** starts in
([Settings of this product](08-settings.md)). The desktop app does not use
iCloud: copy the card files across to have them on an iPhone or iPad.

**Opening presets.** Besides **File ▸ Load Preset…**, you can drop a
`.ti59`, `.ti58` or `.ti58c` file on the window. On the Mac, the Finder's
**Open With** offers Calc-U-59 for these files, and a file dropped on its
Dock icon opens; on Windows the installer makes Calc-U-59 open them with a
double click; on Linux, name the file on the command line
(`./Calc-U-59-*.AppImage program.ti59`).

## Android

There is no Android app in a store: the web app installs from the
browser, gets its own icon on the home screen and starts full screen.

1. Open [calc-u.org/59/calc/app/](https://calc-u.org/59/calc/app/) in
   Chrome.
2. Tap **Install** in the banner that offers it. Without a banner, open
   Chrome's **⋮** menu and choose **Add to Home screen** (in newer
   versions **Install app**), then confirm.
3. Start Calc-U-59 from its icon.

Other browsers, such as Samsung Internet, Firefox and Edge, have a
similar command in their menu. The installed app is the browser version,
laid out as the iPhone and iPad app and following the device's rotation.
It has the TI-59, TI-58 and TI-58C, all fourteen modules, the presets and
the settings, and runs without a connection once installed. A new
release takes over at the next start.

**Where your files go.** The browser keeps your magnetic cards and the
TI-58C's constant memory in its storage for the page, from one visit to
the next. **Import Media…** brings a card file in, **Saved Media…**
lists, downloads and deletes your cards (both in the File menu and
behind the More button). **Examples ▸** opens the bundled examples,
**Load Preset…** any other `.ti59`, `.ti58` or `.ti58c` file on the device; files are read on the
device and not uploaded anywhere.
