# Inventory: agent knowledge

Pinned: Calc-U-59 `v2.3.0` @ `231792c`; Calc-U-1600 `v0.8.1` (until tagged,
`dev-0.8.1` @ `b4ea8c8`; read at `v0.8.0` @ `d976d3f`); calc-u-site `59/2.3.0` @ `a383fbc`; global skills,
research repositories and `~/.claude/projects/*/memory/` as on disk.
Research repository HEADs: `ti59-research` `bf423f9`,
`sharp/Sharp1500-1600-Ref` `43743ed`, `sharp/SC61860` `96977db`. Read on
2026-11-08.

Scope (Briefing §1.11): every skill, `CLAUDE.md`, memory file and
agent-facing developer document that must be updated to the glossary terms
once `docs/GLOSSARY.md` exists.

## Summary
- Calc-U-59 keeps its agent guidance in 9 skills, `CLAUDE.md`, an in-repo
  `memory/` folder (1 analysis, 1 index, 5 large CPU traces), a system
  prompt for TI-59 programming (`prompt/ti59-agent-prompt.md`), and
  `reference/*.md`, which `CLAUDE.md` names as the single home of
  architecture facts (`CLAUDE.md:27-40` at `v2.3.0`).
- Calc-U-1600 has no skills. Guidance is a short `CLAUDE.md` (1408 bytes),
  `docs/developer/` (11 files), `docs/background/Decisions.md` and 24 plans,
  plus a `SessionStart` hook in `.claude/settings.json`.
- calc-u-site has its own `CLAUDE.md` and two skills (`site-59`,
  `site-59-design`); no Calc-U-1600 site skill exists yet.
- Global skills relevant here: `PC1500` and `pc1500-build` (directories in
  `~/.claude/skills/`), `SC61860` (a symlink into the `SC61860` repository).
- No TI research skill exists outside Calc-U-59: `~/.claude/skills/`,
  `~/.agents/skills/` and `ti59-research` contain none (no `SKILL.md`
  mentions TI-59 except the Calc-U-59 product skills and the calc-u-site
  skills). The TI "smaller skills" of Briefing §5.1 are therefore the
  Calc-U-59 product skills, chiefly `calc-u-59-core` and
  `calc-u-59-statefiles`.
- `Sharp1500-1600-Ref` holds a second PC-1500/1600 skill,
  `skills/sharp-pc1500-1600/SKILL.md` (8597 bytes), not installed in
  `~/.claude/skills/`.
- The largest memory directories are Calc-U-1600's (68 files) and
  Calc-U-59's (34 files). This repository's own memory directory and
  ReleaseIt's are empty.

## Method
- Sizes are bytes: `git ls-tree -l` / `git show | wc -c` at the pin for
  repository files; `wc -c` for files on disk. A memory directory's size is
  the sum of its `.md` files.
- "Most-used terms" are case-insensitive regex counts of a fixed list of
  domain words (preset, state file, session, machine, model, variant, card,
  magnetic card, cue card, memory card, module, cartridge, media,
  floppy/disk, tape/cassette, connector, port, slot, bay, device,
  peripheral, printer, debugger, frontend/GUI, shared layer (`Shared/`),
  core (`Core/`), snapshot, faceplate, loader, keyboard), top six per
  element. The counts are approximate (for example "card" excludes "cue
  card" and "memory card"; "core" also matches "Core/"). Generic words (ROM,
  program, trace, platform) are left out.

## Elements

| Path | Kind | Owner | Size | Most-used terms (count) |
|---|---|---|---|---|
| Calc-U-59 `CLAUDE.md` | `CLAUDE.md` | Calc-U-59 | 5272 | shared layer 8, frontend 8, core 6, debugger 2, variant 1, state file 1 |
| Calc-U-59 `.claude/skills/calc-u-59-core/SKILL.md` | skill | Calc-U-59 | 5781 | core 14, variant 4, module 4, debugger 3, shared layer 2, printer 2 |
| Calc-U-59 `.claude/skills/calc-u-59-shared/SKILL.md` | skill | Calc-U-59 | 3446 | module 6, frontend 4, shared layer 3, snapshot 2, debugger 2, state file 1 |
| Calc-U-59 `.claude/skills/calc-u-59-swift/SKILL.md` | skill | Calc-U-59 | 2916 | frontend 3, debugger 3, snapshot 2, shared layer 2, core 2, card 2 |
| Calc-U-59 `.claude/skills/calc-u-59-qt/SKILL.md` | skill | Calc-U-59 | 4803 | debugger 6, frontend 3, state file 2, shared layer 2, preset 2, card 2 |
| Calc-U-59 `.claude/skills/calc-u-59-web/SKILL.md` | skill | Calc-U-59 | 4257 | shared layer 4, printer 3, preset 3, frontend 3, cue card 3, core 3 |
| Calc-U-59 `.claude/skills/calc-u-59-docs/SKILL.md` | skill | Calc-U-59 | 9442 | shared layer 5, core 5, debugger 3, module 2, variant 1, state file 1 |
| Calc-U-59 `.claude/skills/calc-u-59-statefiles/SKILL.md` | skill | Calc-U-59 | 3063 | card 5, cue card 3, state file 2, shared layer 2, frontend 2, preset 1 |
| Calc-U-59 `.claude/skills/calc-u-59-uitesting/SKILL.md` | skill | Calc-U-59 | 13408 | debugger 14, preset 6, device 4, state file 3, printer 1, model 1 |
| Calc-U-59 `.claude/skills/pre-release-cleanup/SKILL.md` | skill | Calc-U-59 | 9457 | frontend 8, shared layer 5, session 3, model 1, floppy 1, core 1 |
| Calc-U-59 `memory/MEMORY.md` | memory file (in repo) | Calc-U-59 | 557 | printer 2 |
| Calc-U-59 `memory/sbr444-quirk-analysis.md` | memory file (in repo) | Calc-U-59 | 18677 | keyboard 8, module 6, printer 5, session 4, machine 2, core 2 |
| Calc-U-59 `prompt/ti59-agent-prompt.md` | agent prompt | Calc-U-59 | 40237 | keyboard 15, printer 13, module 6, preset 3, card 3, state file 2 |
| Calc-U-59 `reference/AppArchitecture.md` | agent-facing developer doc | Calc-U-59 | 8940 | card 13, cue card 8, debugger 7, shared layer 4, model 4, printer 3 |
| Calc-U-59 `reference/CoreArchitecture.md` | agent-facing developer doc | Calc-U-59 | 49597 | printer 47, card 26, core 19, module 16, debugger 16, shared layer 15 |
| Calc-U-59 `reference/DebugAPI.md` | agent-facing developer doc | Calc-U-59 | 23906 | debugger 17, shared layer 15, session 13, core 9, machine 8, snapshot 6 |
| Calc-U-59 `reference/FrontendParity.md` | agent-facing developer doc | Calc-U-59 | 11561 | card 19, debugger 16, session 9, preset 9, printer 7, frontend 7 |
| Calc-U-59 `reference/NewGUIGuide.md` | agent-facing developer doc | Calc-U-59 | 4331 | frontend 13, shared layer 7, card 6, debugger 5, core 4, module 3 |
| Calc-U-59 `reference/SharedArchitecture.md` | agent-facing developer doc | Calc-U-59 | 27679 | card 58, frontend 20, core 18, session 17, model 17, cue card 17 |
| Calc-U-59 `reference/StateFileFormat.md` | agent-facing developer doc | Calc-U-59 | 25272 | card 87, cue card 25, model 19, printer 12, shared layer 9, preset 9 |
| Calc-U-59 `reference/WebClient.md` | agent-facing developer doc | Calc-U-59 | 15952 | preset 19, printer 13, debugger 10, cue card 10, model 9, card 9 |
| Calc-U-59 `tools/agentic/README.md` | agent-facing developer doc | Calc-U-59 | 12385 | printer 12, machine 5, card 4, preset 3, module 2, frontend 2 |
| Calc-U-59 `tools/headless/README.md` | agent-facing developer doc | Calc-U-59 | 1832 | machine 2, variant 1, printer 1, module 1, core 1 |
| Calc-U-59 `tools/goldens/README.md` | agent-facing developer doc | Calc-U-59 | 1267 | shared layer 4, port 3, state file 1, cue card 1, card 1 |
| Calc-U-1600 `CLAUDE.md` | `CLAUDE.md` | Calc-U-1600 | 1408 | debugger 6, core 6, frontend 4, shared layer 3, machine 2 |
| Calc-U-1600 `docs/background/Decisions.md` | agent-facing developer doc | Calc-U-1600 | 55468 | preset 54, card 38, slot 34, tape 31, machine 26, floppy 26 |
| Calc-U-1600 `docs/background/README.md` | agent-facing developer doc | Calc-U-1600 | 4127 | debugger 12, frontend 5, preset 4, loader 4, connector 4, tape 3 |
| Calc-U-1600 `docs/developer/Building.md` | agent-facing developer doc | Calc-U-1600 | 11000 | core 13, debugger 12, machine 9, frontend 5, shared layer 3, preset 2 |
| Calc-U-1600 `docs/developer/Cloud-Sessions.md` | agent-facing developer doc | Calc-U-1600 | 1856 | session 6, core 3, machine 1, frontend 1 |
| Calc-U-1600 `docs/developer/Debugger-Internals.md` | agent-facing developer doc | Calc-U-1600 | 3773 | debugger 13, session 6, preset 4, machine 4, core 3, shared layer 1 |
| Calc-U-1600 `docs/developer/Floppy-Image-Format.md` | agent-facing developer doc | Calc-U-1600 | 6054 | floppy 23, preset 2, peripheral 2, core 2, frontend 1, connector 1 |
| Calc-U-1600 `docs/developer/LCD-Text.md` | agent-facing developer doc | Calc-U-1600 | 5736 | core 8, machine 6, preset 4, model 3, debugger 3, session 1 |
| Calc-U-1600 `docs/developer/PC1600-Core-Limitations.md` | agent-facing developer doc | Calc-U-1600 | 24928 | core 33, port 26, card 25, slot 15, connector 13, model 8 |
| Calc-U-1600 `docs/developer/PC1600-Host-Drive-Internals.md` | agent-facing developer doc | Calc-U-1600 | 6082 | module 6, device 4, core 4, port 2, connector 2, model 1 |
| Calc-U-1600 `docs/developer/PC1600-Serial-Port.md` | agent-facing developer doc | Calc-U-1600 | 4895 | port 14, device 3, machine 2, shared layer 1, session 1, model 1 |
| Calc-U-1600 `docs/developer/README.md` | agent-facing developer doc | Calc-U-1600 | 5157 | debugger 7, core 7, card 7, preset 5, session 4, port 3 |
| Calc-U-1600 `docs/developer/Shared-Architecture.md` | agent-facing developer doc | Calc-U-1600 | 8070 | machine 15, session 12, core 12, frontend 11, preset 10, model 7 |
| Calc-U-1600 `docs/developer/Web-Client.md` | agent-facing developer doc | Calc-U-1600 | 6241 | preset 5, printer 4, session 3, keyboard 3, faceplate 3, shared layer 2 |
| Calc-U-1600 `docs/developer/screenshots/README.md` | agent-facing developer doc | Calc-U-1600 | 10209 | preset 20, model 8, tape 7, floppy 6, faceplate 4, card 4 |
| Calc-U-1600 `dev/README.md` | agent-facing developer doc | Calc-U-1600 | 1320 | preset 4, machine 3, loader 3, tape 2, card 2, memory card 1 |
| Calc-U-1600 `headless/README.md` | agent-facing developer doc | Calc-U-1600 | 984 | preset 3, debugger 2 |
| Calc-U-1600 `docs/background/plans/BASIC-Debugger-Planning.md` | agent-facing developer doc | Calc-U-1600 | 5916 | debugger 14, machine 9, core 4, session 3, variant 1, peripheral 1 |
| Calc-U-1600 `docs/background/plans/Cassette-Tape-Plan.md` | agent-facing developer doc | Calc-U-1600 | 10670 | tape 34, core 9, port 7, machine 4, frontend 3, connector 3 |
| Calc-U-1600 `docs/background/plans/Code-Cleanup-Plan.md` | agent-facing developer doc | Calc-U-1600 | 6626 | connector 9, card 7, core 6, preset 4, model 2, floppy 2 |
| Calc-U-1600 `docs/background/plans/DAP-Debugger-Plan.md` | agent-facing developer doc | Calc-U-1600 | 28866 | debugger 35, core 16, machine 14, session 10, preset 7, port 7 |
| Calc-U-1600 `docs/background/plans/Debugger-Restructuring-Plan.md` | agent-facing developer doc | Calc-U-1600 | 14774 | debugger 21, machine 15, core 15, preset 9, session 7, model 4 |
| Calc-U-1600 `docs/background/plans/Debugger-Use-Cases-Plan.md` | agent-facing developer doc | Calc-U-1600 | 10755 | debugger 44, preset 28, module 8, card 6, port 5, machine 5 |
| Calc-U-1600 `docs/background/plans/Drag-And-Drop-Plan.md` | agent-facing developer doc | Calc-U-1600 | 9474 | preset 20, model 8, core 7, machine 3, loader 3, session 2 |
| Calc-U-1600 `docs/background/plans/Expansion-Connectors-Plan.md` | agent-facing developer doc | Calc-U-1600 | 7811 | connector 13, card 12, machine 4, slot 3, debugger 3, variant 2 |
| Calc-U-1600 `docs/background/plans/Format-Freeze-Plan.md` | agent-facing developer doc | Calc-U-1600 | 12441 | card 41, slot 40, preset 28, core 10, module 8, frontend 6 |
| Calc-U-1600 `docs/background/plans/Inspector-Panel-Plan.md` | agent-facing developer doc | Calc-U-1600 | 12743 | core 14, slot 12, floppy 12, debugger 11, card 9, module 4 |
| Calc-U-1600 `docs/background/plans/KBII-Typing-Plan.md` | agent-facing developer doc | Calc-U-1600 | 10047 | core 8, preset 5, keyboard 5, frontend 5, variant 1, debugger 1 |
| Calc-U-1600 `docs/background/plans/LCD-Text-Plan.md` | agent-facing developer doc | Calc-U-1600 | 6238 | preset 8, core 8, session 2, model 2, machine 2, frontend 2 |
| Calc-U-1600 `docs/background/plans/Loader-File-Kind-Plan.md` | agent-facing developer doc | Calc-U-1600 | 11228 | machine 18, core 11, loader 10, preset 8, model 2, debugger 2 |
| Calc-U-1600 `docs/background/plans/Loader-Mode-Plan.md` | agent-facing developer doc | Calc-U-1600 | 11457 | module 16, loader 15, machine 12, preset 10, slot 5, debugger 3 |
| Calc-U-1600 `docs/background/plans/Media-Names-Plan.md` | agent-facing developer doc | Calc-U-1600 | 10730 | preset 42, floppy 17, card 15, model 12, core 10, module 5 |
| Calc-U-1600 `docs/background/plans/PC1500-RENUM-ROM-Plan.md` | agent-facing developer doc | Calc-U-1600 | 11526 | debugger 17, module 7, preset 4, core 4, peripheral 1, connector 1 |
| Calc-U-1600 `docs/background/plans/PC1600-LCD-Datasheet-Plan.md` | agent-facing developer doc | Calc-U-1600 | 6908 | model 5, core 4, port 2, frontend 2 |
| Calc-U-1600 `docs/background/plans/PC1600-SubCpu-CPC-Plan.md` | agent-facing developer doc | Calc-U-1600 | 12930 | core 5, port 4, model 2, machine 2, keyboard 2, frontend 2 |
| Calc-U-1600 `docs/background/plans/Pre-Execution-Snapshots-Plan.md` | agent-facing developer doc | Calc-U-1600 | 6857 | core 11, debugger 7, snapshot 2, preset 2, slot 1, session 1 |
| Calc-U-1600 `docs/background/plans/Preset-Review-Plan.md` | agent-facing developer doc | Calc-U-1600 | 10280 | preset 34, slot 12, core 10, debugger 9, model 7, module 6 |
| Calc-U-1600 `docs/background/plans/Shared-Layer-Followup-Plan.md` | agent-facing developer doc | Calc-U-1600 | 10462 | session 27, model 10, bay 10, shared layer 9, slot 5, frontend 5 |
| Calc-U-1600 `docs/background/plans/Shared-Layer-Plan.md` | agent-facing developer doc | Calc-U-1600 | 13457 | shared layer 26, core 17, session 13, preset 12, frontend 11, machine 8 |
| Calc-U-1600 `docs/background/plans/Sixty-Pin-Connector-Plan.md` | agent-facing developer doc | Calc-U-1600 | 8904 | card 11, connector 10, model 5, tape 4, machine 4, core 4 |
| Calc-U-1600 `docs/background/plans/Web-GUI-Plan.md` | agent-facing developer doc | Calc-U-1600 | 13425 | preset 15, tape 13, card 13, frontend 11, shared layer 10, model 9 |
| calc-u-site `CLAUDE.md` | `CLAUDE.md` | calc-u-site | 2911 |  |
| calc-u-site `.claude/skills/site-59/SKILL.md` | skill | calc-u-site | 6333 | debugger 4, preset 3, module 3, session 1, machine 1, keyboard 1 |
| calc-u-site `.claude/skills/site-59-design/SKILL.md` | skill | calc-u-site | 3645 | module 1, card 1 |
| calc-u-site `59/README.md` | agent-facing developer doc | calc-u-site | 13434 | module 6, card 6, device 4, variant 1, machine 1 |
| calc-u-site `59/.seo/README.md` | agent-facing developer doc | calc-u-site | 13394 | printer 1, module 1, keyboard 1, floppy 1, core 1 |
| calc-u-site `README.md` | agent-facing developer doc | calc-u-site | 854 |  |
| `~/Development/public/Calc-U/CLAUDE.md` | `CLAUDE.md` | Calc-U | 1886 | machine 1 |
| `~/.claude/CLAUDE.md` | `CLAUDE.md` | global | 1897 | session 1 |
| `~/.claude/skills/PC1500/SKILL.md` | skill | global | 23629 | module 15, tape 10, shared layer 9, machine 9, peripheral 8, slot 6 |
| `~/.claude/skills/PC1500/references/pc1600-disassembly.md` | skill | global | 4195 | port 3, slot 2, device 1 |
| `~/.claude/skills/pc1500-build/SKILL.md` | skill | global | 4307 | variant 5, preset 4, model 2, machine 1, loader 1 |
| `~/.claude/skills/SC61860/SKILL.md` | skill | research | 6794 | model 16, machine 11, tape 6, shared layer 6, port 6, card 3 |
| `~/.claude/skills/SC61860/evals.md` | skill | research | 3422 | shared layer 3, model 3, machine 3, tape 2, session 1, port 1 |
| `~/Development/sharp/Sharp1500-1600-Ref/skills/sharp-pc1500-1600/SKILL.md` | skill | research | 8597 | machine 8, module 7, tape 6, peripheral 6, port 4, floppy 3 |
| `~/.claude/projects/-Users-me-Development-public-Calc-U-59/memory/` | memory dir | Calc-U-59 | 96679 | printer 78, card 46, debugger 24, preset 19, core 18, state file 17 |
| `~/.claude/projects/-Users-me-Development-claude-ti59-emul/memory/` | memory dir | Calc-U-59 (former path `~/Development/claude/ti59-emul`, *unverified*) | 1863 | state file 3, core 2, machine 1 |
| `~/.claude/projects/-Users-me-Development-sharp-Calc-U-1600/memory/` | memory dir | Calc-U-1600 | 147386 | preset 73, card 62, debugger 58, floppy 53, loader 47, model 45 |
| `~/.claude/projects/-Users-me-Development-Calc-U-16/memory/` | memory dir | Calc-U-16 (separate repository `tinue/Calc-U-16`, not an emulator); out of scope (Briefing §1.11) | 2385 | device 5 |
| `~/.claude/projects/-Users-me-Development-sharp-Calc-U-1600-Binary/memory/` | memory dir | Calc-U-1600 (*unverified*; directory `sharp/Calc-U-1600-Binary` no longer exists) | 1086 |  |
| `~/.claude/projects/-Users-me-Development-public-calc-u-site/memory/` | memory dir | calc-u-site | 1607 | preset 1, port 1, memory card 1, keyboard 1, debugger 1, card 1 |
| `~/.claude/projects/-Users-me-Development-ti59-research/memory/` | memory dir | research | 7857 | keyboard 18, printer 10, session 1 |
| `~/.claude/projects/-Users-me-Development-sharp-Sharp1500-1600-Ref/memory/` | memory dir | research | 22296 | module 14, slot 12, preset 11, floppy 8, tape 5, frontend 4 |
| `~/.claude/projects/-Users-me-Development-sharp-pc1500/memory/` | memory dir | research | 1466 |  |
| `~/.claude/projects/-Users-me-Development-sharp-pc1600/memory/` | memory dir | research | 2938 | debugger 1 |
| `~/.claude/projects/-Users-me-Development-sharp-PC-1600-ROM/memory/` | memory dir | research | 1173 | module 1 |
| `~/.claude/projects/-Users-me-Development-sharp-SC61860/memory/` | memory dir | research | 2268 |  |
| `~/.claude/projects/-Users-me-Development-sharp-SharpDataExchange/memory/` | memory dir | SDE (upstream dependency) | 6145 | tape 7, device 3, core 2, variant 1, module 1, frontend 1 |
| `~/.claude/projects/-Users-me-Development-sharp-SharpBasicReference/memory/` | memory dir | research | 2348 |  |

Elements without counted terms:

| Path | Kind | Owner | Size | Notes |
|---|---|---|---|---|
| Calc-U-59 `memory/errorblink.txt`, `pr-after-halt1.txt`, `pr-after-halt2.txt`, `rs.txt`, `sbr444.txt` | memory file (CPU traces, in repo) | Calc-U-59 | 309 784; 3 620 816; 238 387; 1 138 549; 366 914 | `tools/read_trace.py` text output, cited by `memory/sbr444-quirk-analysis.md` and `rom/TI59-commented.asm` (`memory/MEMORY.md` at `v2.3.0`) |
| Calc-U-1600 `.claude/settings.json` | Claude Code settings | Calc-U-1600 | 295 | `SessionStart` hook running `tools/cloud_session_start.sh` (timeout 180 s) |
| `~/.claude/plans/rustling-napping-pelican.md` | plan file | calc-u-site | 9110 | Content outline for calc-u.org/1600, referenced by the calc-u-site memory file `site-1600-content-plan.md` |
| `~/Development/ti59-research/.claude/settings.local.json` | Claude Code settings | research | 4884 | Local settings only; no guidance text (not read further) |
| `~/.claude/projects/-Users-me-Development-public-Calc-U/memory/` | memory dir | Calc-U | 0 | Empty |
| `~/.claude/projects/-Users-me-Development-public-ReleaseIt/memory/` | memory dir | ReleaseIt | 0 | Empty |

## Product-specific names for shared concepts

Names found in the guidance above that phase 2 maps (counts are
case-insensitive `git grep -c` sums over the files listed):

| Concept | Calc-U-59 guidance (`CLAUDE.md`, `.claude/`, `reference/`, `prompt/`, `memory/*.md`, `tools/agentic/README.md`) | Calc-U-1600 guidance (`CLAUDE.md`, `docs/developer/`, `docs/background/`) |
|---|---|---|
| Running machine plus app logic | `SessionController` (63), `EmulatorSession` (18), for example `reference/DebugAPI.md:12`, `:62` | `AppSession` (39), `MachineSession` (29), for example `docs/background/Decisions.md:545`, `:579` |
| File that sets up the machine | "preset" (60) for the user-facing term, "state file" (18) for the format; both deliberately distinct (memory file `state_vs_preset_terminology.md`) | "preset" |
| Removable item | "module" (94), "Solid State" (7), "card" / "magnetic card" | "slot" (142), "bay" (37), "media" (38), "memory module" (5), "memory card" (3) |

## Memory files per directory

Names only (`.md` dropped):

- `-Users-me-Development-public-Calc-U-59/memory/` (34 files): calendar_speed_project, card_timing_audio, changelog_scope, cuecard_template_semantics, debugger_documentation_insights, dynamic_type_accessibility, fastmode_interrupt_handover, features_both_frontends_shared_first, file_importer_architecture, fullspeed_scanloop_range_fix, hardware_test_setup, headless_printer_lines, headless, ios_27_1_fold_runtime, live_debug_panel, MEMORY, memwr_memrd_implementation, mnemonics_workflow, no_thin_wrappers, pc100a_head_map, pdcm_example_branch, preg_rtn_bug_fix, printer_timing_audio, project_memory_dir, qt6_platform_scope, real_hardware_timing, rom_comment_policy, rom_documentation_effort, run_time_measurement, screenshot_convention, simulator_upgrade_automation, solid_state_pc_display, state_vs_preset_terminology, trace_display_capture
- `-Users-me-Development-claude-ti59-emul/memory/` (2 files): MEMORY, project_state_file_format
- `-Users-me-Development-sharp-Calc-U-1600/memory/` (59 files): feedback_autonomous_phased_implementation, feedback_cards_know_only_the_bus, feedback_cleanup_no_relocation, feedback_commit_plans_to_docs, feedback_commit_to_dev_branch, feedback_commit_todo_with_user_edits, feedback_debug_output_to_console, feedback_decisions_md, feedback_docs_three_levels, feedback_fix_loader_not_presets, feedback_gui_test_launch_quit, feedback_headless_dir_for_probe_output, feedback_lcd_text_before_images, feedback_machine_config_on_control_bar, feedback_no_signal_modelling_while_research_open, feedback_one_time_verification_own_dir, feedback_original_sources_only, feedback_original_work_not_fork, feedback_paste_vs_inbound_typing, feedback_pc1500_no_multistatement_direct_mode, feedback_pc1600_maxfiles_before_open, feedback_pre_1_0_breaking_changes, feedback_rom_disassembly_not_bin, feedback_rom_loading_test_via_s3, feedback_user_manuals_use_case_structure, MEMORY, project_buzzer_audio, project_cassette_tape, project_ce150_pc1600_lprint_bug, project_ce150_plot_x_offset_step_scale, project_ce158_interface, project_ce1600f_format_resolved, project_ce1600p_paper_command_vertical_window, project_ce1600p_paper_geometry, project_ce1601m_ramdisk_init_bug, project_dap_debugger_plan, project_developer_id_cert_rotation, project_fast_basic_binary_loader, project_floppy_yaml_format, project_loader_matrix_pc1500, project_me1_not_mirror, project_pc1500_ram_powerup_ff_window, project_pc1600_host_drive, project_pc1600_interrupt_model, project_pc1600_lcd_hd61102_model, project_pc1600_lh5803_lcd_forwarding, project_pc1600_native_basic_and_preset_program, project_pc1600_new_ml_reserve, project_pc1600_ramdisk_geometry_table, project_pc1600_rtc_power_off_lag_fixed, project_pc1600_serial_port, project_pc1600_timing_residual, project_pre_1_0_format_decisions, project_preset_parameterless_wait, project_pu_pv_connector_pins, project_scripted_screenshots, project_shared_layer_wasm, project_sixty_pin_connector_model, reference_vintage_manuals_pdf_folder
- `-Users-me-Development-Calc-U-16/memory/` (3 files): MEMORY, no-xcode-project-scaffolding, test-devices
- `-Users-me-Development-sharp-Calc-U-1600-Binary/memory/` (2 files): feedback_no_manual_commits, MEMORY
- `-Users-me-Development-public-calc-u-site/memory/` (2 files): MEMORY, site-1600-content-plan
- `-Users-me-Development-ti59-research/memory/` (3 files): keyboard_loop_analysis_status, MEMORY, reset_initialization_findings
- `-Users-me-Development-sharp-Sharp1500-1600-Ref/memory/` (17 files): calc-u-1600-headless-cli, ce150-service-manual, ditze-pc1600-tips-pdf, drop-emulator-refs-when-primary-exists, hitachi-lcd-datasheets, MEMORY, pc1500-cold-start-needs-new0, pc1500emul-retired-use-calc-u-1600, pc1600-new-is-pro-mode-only, pc1600-operation-manual-pdf, pc1600-ram-modules-auto-merge, pc1600-software-info-bulletins, pc1600-trm-pdf, pc2-assembly-articles-pdf, pockemul-not-cited, schlieker-rom-unterprogramme-pdf, tc8576af-cpc-datasheet
- `-Users-me-Development-sharp-pc1500/memory/` (2 files): ce163f-v2-not-buggy, MEMORY
- `-Users-me-Development-sharp-pc1600/memory/` (3 files): lh5803-disasm-toolchain, MEMORY, z80-assembler-choice
- `-Users-me-Development-sharp-PC-1600-ROM/memory/` (2 files): MEMORY, repo-visibility
- `-Users-me-Development-sharp-SC61860/memory/` (3 files): MEMORY, research-latest-only, sc61860-knowledge-base
- `-Users-me-Development-sharp-SharpDataExchange/memory/` (4 files): cli-format-flag-convention, MEMORY, rust-convert-poc, wav-cassette-support
- `-Users-me-Development-sharp-SharpBasicReference/memory/` (2 files): MEMORY, pc1600-extension

Directories under `~/.claude/projects/` that are related but outside this
project's scope (names only, not read): `-Users-me-Development-sharp`,
`-Users-me-Development-sharp-SharpAgentInstructions`,
`-Users-me-Development-sharp-SharpBasicShared`,
`-Users-me-Development-sharp-SharpBasicPlugin`,
`-Users-me-Development-sharp-preset-pc1500emu`,
`-Users-me-Development-sharp-SC61860-calc-u-1403h-ui`,
`-Users-me-Development-sharp-SharpDataExchangeRust`,
`-Users-me-Development-public-ti59-printer-art`,
`-Users-me-Development-claude-ti-59-reader`,
`-Users-me-SynologyDrive-Dokumente-PDF-Vintage-TI-ppc-notes-ppc`.

## Open questions
- ~~Is `calc-u-1403h-ui` in scope?~~ Answered 2026-11-09: no, out of scope
  for now, as is Calc-U-16 (Briefing §1.11).
- Should `Sharp1500-1600-Ref/skills/sharp-pc1500-1600/SKILL.md` (not
  installed) be treated as a research skill to update, or is it superseded
  by `~/.claude/skills/PC1500`?
- Are the Calc-U-1600 `docs/background/plans/*.md` (historic plans) in
  scope for the terminology update, or only `Decisions.md` and
  `docs/developer/`?
- Should the stale Calc-U-59 memory directory for
  `-Users-me-Development-claude-ti59-emul` and the one for the removed
  `sharp/Calc-U-1600-Binary` be updated, or left as they are?
- Calc-U-1600's `MEMORY.md` index is 64 lines and 15 260 bytes; Calc-U-59's
  is 35 lines. Whether either exceeds what Claude Code loads at session
  start is not known here.
