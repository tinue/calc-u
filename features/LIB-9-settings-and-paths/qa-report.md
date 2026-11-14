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

## Calc-U-59 (M-32; AC-23 in M-34)

| ID | Result | Step |
|---|---|---|
| AC-19 | Later | M-32 |
| AC-20 | Later | M-32 |
| AC-21 | Later | M-32 (`CalcU59NoPlatformSettings` over `check_platform_stores.sh`) |
| AC-22 | Later | M-32 |
| AC-23 | Later | M-34 (prepared in M-32, adapter A-5) |
| AC-24 | Later | M-32 |
| AC-25 | Later | M-32 |
| AC-34 | Later | M-32 |
| AC-35 | Later | M-32 |

## Calc-U-1600 (M-30)

| ID | Result | Step |
|---|---|---|
| AC-26 | Later | M-30 |
| AC-27 | Later | M-30 |
| AC-28 | Later | M-30 |
| AC-29 | Later | M-30 (the library part, `[folders]` with `fixed:`, passes now: `ac29_open_folder_fixed_else_last_used_else_home`, `startup_tests.cpp`) |
| AC-30 | Later | M-30 |
| AC-31 | Later | M-30 (`CalcU1600NoPlatformSettings` over `check_platform_stores.sh`) |
| AC-32 | Later | M-30 |
