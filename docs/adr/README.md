# Architecture decisions

One ADR per decision (`CLAUDE.md`). Phase 3 also produced
[`../oddities-triage.md`](../oddities-triage.md); the skill structure is
deferred until after the migration (Briefing §6).

## Products and repository

| ADR | Decision |
|---|---|
| [0001](0001-one-product-per-model-family.md) | One product per model family; the Sharp product stays Calc-U-1600 |
| [0002](0002-products-and-library-directories.md) | Repository layout: one directory per product, the library beside them |
| [0003](0003-one-root-cmake-project.md) | One root CMake project with options per product and frontend |
| [0004](0004-one-ci-workflow-per-product.md) | One CI workflow per product |
| [0005](0005-cpp20-everywhere.md) | C++20 for the whole repository |

## Machine architecture

| ADR | Decision |
|---|---|
| [0006](0006-the-machine-is-the-only-core-seam.md) | The machine is the only seam into a core |
| [0007](0007-small-machine-interface-with-capabilities.md) | A small machine interface plus generic capabilities |
| [0008](0008-machines-are-composed.md) | Machines are composed of components and peripherals |
| [0009](0009-single-threaded-sessions.md) | Single-threaded sessions, hosted by the frontend |
| [0010](0010-peripherals-keyed-by-connector.md) | Peripherals are keyed by connector |
| [0011](0011-ti59-printer-and-module-slot-as-functional-connectors.md) | The TI-59's printer and module slot become functional connectors |
| [0012](0012-component-boundaries-follow-observable-behaviour.md) | Component boundaries follow observable behaviour |

## Swift frontend

| ADR | Decision |
|---|---|
| [0013](0013-swift-frontend-links-an-xcframework.md) | The Swift frontend links an XCFramework built by CMake |
| [0014](0014-objcxx-bridge-translates-only.md) | The Swift bridge is ObjC++ and only translates |
| [0015](0015-swift-frontend-hosts-its-session-on-a-background-thread.md) | The Swift frontend hosts its session on a background thread |

## Presets

| ADR | Decision |
|---|---|
| [0016](0016-a-preset-always-cold-boots.md) | Loading a preset always builds its machine from a cold boot |
| [0017](0017-presets-are-parsed-all-or-nothing.md) | Presets are parsed all-or-nothing |
| [0018](0018-one-family-preset-format.md) | One preset format for the family |
| [0037](0037-releases-read-only-the-current-version-of-own-formats.md) | The migration release reads only the current version of its own formats (2026-11-10; a one-time exception, afterwards one version back) |

## Media and settings

| ADR | Decision |
|---|---|
| [0019](0019-media-live-in-the-save-folder.md) | Media live in the save folder |
| [0020](0020-magnetic-cards-are-written-in-the-current-version.md) | Magnetic cards are written in the current version |
| [0021](0021-settings-hold-preferences-startup-presets-hold-machines.md) | Settings hold preferences; startup presets hold machines |
| [0022](0022-settings-files-are-ini.md) | Settings files are INI |

## Frontends

| ADR | Decision |
|---|---|
| [0023](0023-frontend-components-per-technology.md) | Frontend components are shared per technology |
| [0024](0024-one-family-menu-and-control-bar-structure.md) | One menu and control-bar structure for the family |
| [0025](0025-modules-are-chosen-in-the-control-bar.md) | Modules are chosen in the control bar |

## Harness

| ADR | Decision |
|---|---|
| [0026](0026-harness-is-one-cli-with-a-session-server.md) | The harness is one CLI with a session server |

## Versions and releases

| ADR | Decision |
|---|---|
| [0027](0027-versions-per-product-library-unversioned.md) | Each product has its own version; the library has none |
| [0028](0028-releases-from-the-source-repository-with-product-tags.md) | Releases come from the source repository, tagged per product |
| [0029](0029-trunk-with-maintenance-branches.md) | Trunk with maintenance branches |
| [0030](0030-old-repositories-are-archived-and-keep-the-history.md) | The old repositories are archived and keep the history |

## Testing

| ADR | Decision |
|---|---|
| [0031](0031-three-test-groups-with-budgets.md) | Three test groups with time budgets |
| [0032](0032-gui-tests-are-thin-and-share-scenario-ids.md) | GUI tests are thin and share scenario IDs |
| [0033](0033-harness-attaches-to-running-apps-in-simulators.md) | The harness attaches to running apps in simulators |
| [0038](0038-one-test-runner.md) | One C++ test runner: Calc-U-59's TestHarness (2026-11-10) |

## ROMs

| ADR | Decision |
|---|---|
| [0034](0034-roms-live-in-rom-repositories.md) | ROMs live in ROM repositories and are fetched at build time |
| [0035](0035-one-rom-lookup-rom-less-builds-supported.md) | One ROM lookup; a build without ROMs is supported |

## Help

| ADR | Decision |
|---|---|
| [0036](0036-help-is-markdown-in-the-source-repository.md) | Help is Markdown in the source repository; the site pins it |
