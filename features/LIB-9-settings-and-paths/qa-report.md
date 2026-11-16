# LIB-9: QA report

Results per acceptance criterion and edge case of `spec.md`, per column.

## Library (M-20, 2026-11-14)

Built and run on an M4 Pro, macOS 27.2, Apple clang 21, CMake 4.5, Ninja.
The settings code is `library/settings/` (namespace `calcu::settings`) in
`calcu_library`; its tests are `library/settings/tests/` in
`calcu_library_tests` (CTest entry of the same name, label `frequent`),
with the product declared for the tests in
`library/settings/tests/TestProduct.hpp` (`Test-Product`, models `Model-A`
and `Model-B`, fallback `Model-B`). Every platform's places are checked on
every host: `settingsBaseFor` and `Paths` are pure functions of what the
caller passes (the exit check of M-20). Result of the run:
`calcu_library_tests` 87 tests (40 of LIB-1, 47 of LIB-9), 722 checks
passed, 0 failed, 0.14 s; `LibraryPlatformStoreCheck` passed, 0.03 s
(CTest, in the full Frequent run of `build/tests`, all 7 entries passed;
the machine was loaded by other builds at the time).

| ID | Result | Test case (file in `library/settings/tests/`) |
|---|---|---|
| AC-1 | Pass | `ac1_declaration_names_kebab_case` (`settings_file_tests.cpp`: `checkDeclaration` finds a camelCase key, a `[preset]` key that is no model, a section not in kebab-case, a key declared twice and a default its key does not accept; the new file has `[section]` headers and `key = value` lines); `ac2_product_name_as_written` (`paths_tests.cpp`: `<Product>.ini`) |
| AC-2 | Pass | `ac2_settings_place_macos_ios`, `ac2_settings_place_windows_appdata`, `ac2_settings_place_linux_xdg_and_fallback`, `ac2_settings_place_web`, `ac2_product_name_as_written` (`paths_tests.cpp`) |
| AC-3 | Pass | `ac3_missing_key_has_default` (`settings_file_tests.cpp`) |
| AC-4 | Pass | `ac4_unknown_keys_sections_comments_order_survive_set`, `ac4_unchanged_document_round_trips_bytes` (`ini_document_tests.cpp`); `ac4_settings_file_keeps_unknown_keys_on_write` (`settings_file_tests.cpp`) |
| AC-5 | Pass | `ac5_first_start_writes_version_and_every_key` (`settings_file_tests.cpp`) |
| AC-6 | Pass | `ac6_change_survives_reopen`, `ac6_file_not_reread_while_running` (`settings_file_tests.cpp`) |
| AC-7 | Pass | `ac7_no_version_line_starts_with_defaults_and_says_so` (`settings_file_tests.cpp`: no version, another version, a version inside a section; each starts with the defaults, gives a problem text and leaves the file untouched) |
| AC-8 | Pass | `ac8_declaration_holds_no_hardware` (`settings_file_tests.cpp`); by construction `ValueKind` has no kind for hardware. The products' lists of every key are M-30 (AC-26) and M-32 (AC-19, AC-22) |
| AC-9 | Pass | `ac9_model_last_or_name_else_last` (`startup_tests.cpp`) |
| AC-10 | Pass | `ac10_preset_per_model_empty_means_none` (`startup_tests.cpp`) |
| AC-11 | Pass | `ac11_trace_size_missing_zero_negative_is_50` (`settings_file_tests.cpp`; also empty and not a number) |
| AC-12 | Pass | `ac12_every_place_from_the_passed_dirs` (`paths_tests.cpp`: settings file, save folder, tapes, traces, screenshots, serial links and `ti58c.memory.yaml` from `HostDirs`; a folder chosen through the platform passed in as `HostDirs::chosenTraceFolder`). That no frontend works out a place itself holds once the products use `Paths` (M-30, M-32) |
| AC-13 | Pass | `ac13_qt6_save_folder_home_product_or_override` (`paths_tests.cpp`) |
| AC-14 | Pass | `ac14_swift_save_folder_is_passed_folder` (`paths_tests.cpp`) |
| AC-15 | Pass | `ac15_web_save_folder_inside_passed_fs` (`paths_tests.cpp`) |
| AC-16 | Pass | `ac16_files_at_root_tapes_in_Tapes`, `ac16_tape_override` (`paths_tests.cpp`) |
| AC-17 | Pass | `ac17_qt6_traces_screenshots_links_default_to_save_folder`, `ac17_swift_screenshots_save_folder_traces_by_location`, `ac17_web_has_no_trace_or_screenshot_folder` (`paths_tests.cpp`). The Web download itself comes to the products with LIB-10 and LIB-6 (design §5, A-3, A-4) |
| AC-18 | Pass | `ac18_own_host_dirs_touch_nothing_else` (`paths_tests.cpp`: a "user" folder with a settings file and a save folder is unchanged after a run on its own `HostDirs`); `ac18_defaults_only_reads_and_writes_nothing` (`settings_file_tests.cpp`) |
| AC-33 | Pass | `ac33_grammar_comments_both_line_endings`, `ac33_value_is_rest_of_line_trimmed`, `ac33_names_case_insensitive`, `ac33_last_duplicate_wins`, `ac33_bom_or_bad_utf8_is_unreadable` (`ini_document_tests.cpp`); `ac33_booleans_true_false` (`settings_file_tests.cpp`); `ac33_paths_absolute_tilde_is_home` (`paths_tests.cpp`) |
| EC-1 | Pass | `ec1_last_model_not_a_model_gives_fallback` (`startup_tests.cpp`) |
| EC-2 | Pass | `ec2_swift_save_folder_follows_each_start` (`paths_tests.cpp`) |
| EC-3 | Pass | `ec3_override_save_folder_keeps_Tapes_inside` (`paths_tests.cpp`) |
| EC-4 | Pass | `ec4_two_settings_bases_two_files` (`paths_tests.cpp`) |
| EC-5 | Pass | `ec5_bad_value_reads_default_and_stays_in_file` (`settings_file_tests.cpp`) |
| EC-6 | Pass | `ec6_unreadable_file_untouched_defaults_logged` (`settings_file_tests.cpp`; a line that is not INI, a BOM). Writing the problem to the log is the product's (it reads `SettingsFile::problem()`; M-30, M-32) |
| EC-7 | Pass | `ec7_interrupted_write_leaves_old_or_new` (`settings_file_tests.cpp`; the test hook `testing::failBeforeRename` in `AtomicFile`) |
| EC-8 | Pass | `ec8_missing_folder_created_on_first_write` (`paths_tests.cpp`) |
| EC-9 | Pass | `ec9_unusable_override_reported_no_fallback` (`paths_tests.cpp`; an override below a plain file). Telling the user is the frontend's, from `Paths::Ensured::problem` |
| EC-10 | Pass | `ec10_two_writers_last_wins_unknown_keys_kept` (`settings_file_tests.cpp`) |

Also in M-20 (design T-14): `library/settings/check_platform_stores.sh`,
the source check the products run over their frontends in M-30 and M-32
(AC-21, AC-31, AC-34), with its self-test
`library/settings/tests/check_platform_stores_test.sh` as the CTest entry
`LibraryPlatformStoreCheck` (label `frequent`; not registered on Windows,
where no bash is assumed). Allow-list format: one `<path>: <text>` entry
per line.

Library-only configure (what CI's `library` job does):
`cmake -S . -B build/library -G Ninja -DCALCU_PRODUCT_59=OFF
-DCALCU_PRODUCT_1600=OFF -DCALCU_FRONTEND_QT=OFF`, build and `ctest`
pass (2 tests). Emscripten: `calcu_library` with the settings sources
builds in `build/web` for Calc-U-1600's `calcu_web` (Homebrew Emscripten,
`products/calc-u-1600/frontends/web/build.sh` ran through and wrote
`dist/`). The Web frontends do not use the settings code yet (M-30, M-32).

### Deviations from the design

- `Paths(const SettingsFile&, HostDirs)` and `startupModel(const
  SettingsFile&)` take the product's declaration from the `SettingsFile`
  instead of a separate `ProductDecl` argument (§3 sketch); one source for
  it, the same behaviour.
- `settingsBaseFor`'s last argument is `passedFolder`: the Application
  Support folder on macOS and iOS, and the settings folder inside the Web
  frontend's file system on the Web (`/persist/settings`, §4).
- Two cases the spec does not name, chosen so that the path is never
  relative: macOS or iOS without a passed folder gives
  `<home>/Library/Application Support` (the non-sandboxed place of AC-2);
  Windows without `%APPDATA%` gives `<home>\AppData\Roaming`, its usual
  value. Open for review; a frontend always passes them in practice.
- EC-10 merge on write (D-3): a write applies to the file on disk the keys
  this instance changed that have not reached the file yet (normally just
  the one being set; after a failed write, also the earlier ones), not
  every key it ever set. So the last write wins per key, as EC-10 says.
- `SettingsFile::set` in a run where the file is not written (unreadable,
  no or another version) keeps the change in memory and returns `ok =
  false` with the reason, so the frontend can say that it is not saved.
  `defaultsOnly` returns `ok = true` (nothing is meant to be written).
- Extra test cases beside §6's list: `ac4_settings_file_keeps_unknown_keys_on_write`,
  `ac8_declaration_holds_no_hardware`, `ac12_every_place_from_the_passed_dirs`,
  `ac33_booleans_true_false`, `ac33_paths_absolute_tilde_is_home`.
- `TestProduct.hpp` is included as `settings/tests/TestProduct.hpp` (rooted
  at `library/`), so `calcu_library_tests` needs no new include folder.

## Calc-U-59 (M-32, 2026-11-15; AC-23 in M-34)

Built and run on an M4 Pro, macOS 27.2, Apple clang 21, CMake 4.5, Ninja.
The product's declaration is `cu59::settings::settingsDecl()` in
`products/calc-u-59/app/Settings.hpp/.cpp`, with typed calls
(`cu59::settings::Settings`) over a `calcu::settings::SettingsFile&`;
`app/SettingsKeys.hpp` is deleted. Qt6 opens `Calc-U-59.ini` in the
platform's settings folder (`frontends/qt6/app/AppPaths.cpp`: on macOS
`~/Library/Application Support`, the development build being unsandboxed;
`%APPDATA%` on Windows; `$XDG_CONFIG_HOME` or `~/.config` on Linux) and
takes the save and trace folders from `calcu::settings::Paths` (adapter
A-2); the Swift frontend opens it in its Application Support folder through
`Bridge/TISettings` and the observable `SettingsModel`
(`App/AppSettings.swift`), and under `--UITesting` uses a file in memory with
the `--setting` launch values; the Web frontend opens it at
`/settings/Calc-U-59/Calc-U-59.ini` in Emscripten's file system, filled from
and mirrored to the page's `localStorage` entry `calcu59/Calc-U-59.ini`
(adapter A-5). `saveModuleID` is a no-op in all three (adapter A-6). Tests in
`products/calc-u-59/tests/shared_settings_tests.cpp` (in `CalcU59Tests`,
Frequent). Result of the run: `CalcU59Tests` 234 tests, 36598 checks
passed, 0 failed, 8.8 s; `CalcU59NoPlatformSettings` passed; the full
Frequent run of `build/tests` passed, 16 of 16 entries. Golden runs:
`golden_runs_tests.cpp` passed unchanged (740 checks). Built once: the Qt6 app
(`tools/build_qt.sh`), the XCFramework and the Swift app for macOS and the iOS
simulator (`xcodebuild … build`), the Web frontend (`frontends/web/build.sh`);
`CalcU59WebPlaywright` 67 passed, 3 skipped (the existing conditional ones),
with the new case. The Qt6 app ran twice with `--grab` on a temporary
`$HOME` (the user's settings were not touched): the first start wrote a
fresh `Calc-U-59.ini` with its version and every key (the exit check), the
quit wrote `[startup] last-model` and the `[window]` numbers, the second start
read them. The Swift app was built, not run (its settings file would be the
user's own; `--UITesting` runs use the file in memory).

| ID | Result | Test case |
|---|---|---|
| AC-19 | Pass | `lib9_ac19_declaration`: every key in order with its default, the app-written ones, the accepted values of `[startup] model`, `led-style`, `feedback`, `location`; the typed calls over those keys, `[folders]` (fixed, last used, Reset; `other` declared, D-1) |
| AC-20 | Pass | `lib9_ac20_startup_last_without_last_model_is_ti59` (also EC-1 and AC-9: a value that is neither `last` nor a model) |
| AC-21 | Pass | `lib9_ac21_first_start_defaults_and_writes_file` (the file at its place with its version and every key, written at the first open; a change read again at the next start); CTest `CalcU59NoPlatformSettings` (allow-list `tools/platform_stores_allowed.txt`: the bookmark key, the page's two entries until M-34, the Playwright check); Longer: Playwright `lib9_ac21_settings_in_the_ini_file_survive_a_reload` (2.3.0's `calcu59/startupModel` is not read; the INI text is stored and a change survives a reload) |
| AC-22 | Pass | `lib9_ac22_no_key_holds_a_module`; no frontend stores or reads the module (`saveModuleID` no-op, A-6; the start module is `cu59::kStartModuleID`) |
| AC-23 | Pass (M-34) | the Web frontend's settings are `/persist-calc-u-59/settings/Calc-U-59/Calc-U-59.ini` in its persistent file system (LIB-15 59; adapter A-5 `WebIniMirror` gone): `web_part_tests.cpp` `lib9_ac23_web_settings_in_the_ini_file` (Frequent); Playwright `storage.spec.js` `lib9_ac23 no calcu59 key read or written` (every `Storage` call of a run with a settings change and a model switch recorded: none with the prefix `calcu59/`) and `ac20 settings survive a page load, in Calc-U-59.ini` (the INI in IndexedDB holds the change, the reload starts with it) (Longer) |
| AC-24 | Pass | `lib9_ac24_trace_location_app_support_icloud_folder` (on `Paths` with `FrontendKind::Swift`) |
| AC-25 | Pass | `lib9_ac25_one_declaration_for_every_frontend` (keys another frontend has no use for are kept on write; the launch values of the UI tests); the three frontends build against the one declaration |
| AC-34 | Pass | the bookmark is the only allowed platform-store use (`CalcU59NoPlatformSettings`); `AppSettings.traceDirectory()` resolves it and passes its folder as `HostDirs::chosenTraceFolder` (`lib9_ac24_…` covers the rule) |
| AC-35 | Pass | `lib9_ac35_window_numbers_round_trip`; the Qt6 run above wrote and read `[window]` |

Exit check: a first start writes a fresh INI file (`lib9_ac21_…`; the Qt6
run above; on the Web the Playwright case).

### Deviations and notes

- The Qt6 window keys are those of D-7; `x`, `y` and the three panel widths
  have no default (empty until the app writes them: the window system places
  the window, the splitter splits by its rules). A maximised window comes
  back at its last normal size (row 59-37).
- `[folders]` (D-2) as in Calc-U-1600: a pick while a fixed folder is set is
  not remembered, and Reset starts in the former fixed folder (row 59-36).
- iOS: the settings screen no longer forces the trace location to iCloud; the
  default is Application Support (row 59-30, D-8). iOS still has no picker for
  it.
- The UI tests' module (`LE` in `ScreenshotTests`) is no setting (AC-22): it
  is a launch value of its own, `--module <id>`, read only under
  `--UITesting`.
- On the Web the INI is created at the first start and its text posted to the
  page at once; in private mode (no `localStorage`) every load starts at the
  defaults, as today.
- `reference/WebClient.md` already says where the Web settings are now
  (it named `SettingsKeys.hpp`, which is gone); its full update stays with
  M-34.

**M-34 (2026-11-15, branch `m34-web-59`).** AC-23 passes (row above): the Web
frontend runs on the library's shell, its settings file is a file of the
persistent file system, and no frontend source reads or writes a
`localStorage` entry (the allow-list keeps only the Playwright check that
2.3.0's entries are not read). The Calc-U-59 column is Done: every criterion
of it passes (AC-19–25, AC-34, AC-35). What the Web page offers of the
settings (startup model, trace size, keyboard feedback) are `TIWebPart`'s
commands; the last used model is written when the model changes.

## Calc-U-1600 (M-30, 2026-11-15)

Built and run on an M4 Pro, macOS 27.2, Apple clang 21, CMake 4.5, Ninja.
The product's declaration is `shared::settingsDecl()` in
`products/calc-u-1600/app/Settings.hpp/.cpp`; `shared::Settings` keeps its
accessor names over a `calcu::settings::SettingsFile&`, and
`shared::AppPaths` takes its places from `calcu::settings::Paths` (adapter
A-1). Qt6 opens `Calc-U-1600.ini` in the platform's settings folder
(`AppPaths.cpp`: Application Support from `MacAppSupport.mm` on macOS,
`%APPDATA%` on Windows, `$XDG_CONFIG_HOME` or `~/.config` on Linux); the Web
frontend at `/persist/settings/Calc-U-1600/Calc-U-1600.ini`. Tests in
`products/calc-u-1600/app/tests/shared_settings_tests.cpp` (in `CoreTests`,
Frequent). Result of the run: `CoreTests` 855 tests, 63275 checks passed,
0 failed, 39.0 s; `CalcU1600NoPlatformSettings` passed; the full Frequent run
of `build/tests` passed, 14 of 14 entries. Golden runs: `golden_runs_tests.cpp`
passed unchanged (Calc-U-1600 and Calc-U-59). Longer, run once: the Web
frontend built (`frontends/web/build.sh`), `CalcU1600WebPlaywright` 45 of 45
passed (with the new case), `tools/build_wasm_check.sh` passed; the Qt6 app
built (`tools/build_app.sh`) and the screenshot scenarios
`02-getting-started`, `07-com-ports` and `09-appendix` ran through (`--shots`
on its own temporary home and settings folder; the user's Application
Support folder was not touched).

| ID | Result | Test case |
|---|---|---|
| AC-26 | Pass | `lib9_ac26_declaration`: every key in order with its default, the app-written ones, `[startup] model` accepting `last` and the three models, the typed calls over those keys |
| AC-27 | Pass | `lib9_ac27_startup_last_without_last_model_is_pc1500a` (also EC-1: a last model that is not one of the product's) |
| AC-28 | Pass | `lib9_ac28_dap_port_outside_range_is_32168` (also not a number) |
| AC-29 | Pass | `ac29_open_folder_fixed_else_last_used_else_home` (library): the folder last used, a fixed folder (`fixed:`) that picks do not change, Reset back to "last used"; `lib9_ac29_folders_four_kinds`: Calc-U-1600's four kinds and their keys (its own run of the rule went with `shared::Settings`' open-folder helpers, Part D simplification S-5) |
| AC-30 | Pass | `lib9_ac30_window_size_default_900x500_and_kept` (written, read again from the file) |
| AC-31 | Pass | `lib9_ac31_first_start_defaults_and_writes_file` (the file at its place, with its version and every key, written at the first open); CTest `CalcU1600NoPlatformSettings` (`library/settings/check_platform_stores.sh` over `frontends/`, allow-list `tools/platform_stores_allowed.txt`: AppKit's two registered defaults and the Playwright check itself); Longer: Playwright "settings survive a reload" finds no `calcu1600/` key in `localStorage` |
| AC-32 | Pass | `lib9_ac32_web_and_qt6_share_declaration`, `lib9_ac32_web_keeps_and_ignores_dap_serial_window_folders`; Longer: `CalcU1600WebPlaywright` "settings survive a reload" (root and sub-path) |

Exit check: a first start writes a fresh INI file (`lib9_ac31_…`; in the
app, the first read of a setting opens the file, `AppPaths.cpp`).

### Deviations and notes

- The startup model's values are the glossary's model names in the file and
  in both settings dialogs (`PC-1500`, `PC-1500A`, `PC-1600`; today
  `PC1500`, …), as AC-26 says. The Web's `settingsState().defaultPresets` keeps
  its `PC1500`-style object keys (not a stored value).
- `[folders]` (D-2): today a pick was remembered even while a fixed folder was
  set, and Reset went back to it. With one key, a pick while a folder is fixed
  is not remembered; Reset makes the fixed folder the "last used" one, so the
  dialog starts where it did before Reset (closest to today; nothing else
  changes).
- `shared::HostDirs` became `calcu::settings::HostDirs` (host folders) plus
  `shared::BundleDirs` (the two bundle folders), as design §4 says.
  `MemorySettingsStore` is replaced by `app/tests/TestSettings.hpp`, a
  `SettingsFile::defaultsOnly` over the product's declaration (nothing read
  or written, AC-18), instead of a file in a temporary folder; the tests that
  need the file itself open one in a temporary folder.
- A settings file that cannot be read, or has no or another version, goes to
  the log (Qt6: `qWarning`; Web: the inspector's log), as EC-6 asks; no
  notice is shown.
- `cmake/CalcU.cmake`: `calcu_core` links `calcu_library` without the
  `if(TARGET calcu_library)` guard (design §8).
