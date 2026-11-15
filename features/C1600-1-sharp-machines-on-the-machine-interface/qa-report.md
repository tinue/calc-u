# C1600-1: QA report

Results per acceptance criterion and edge case of `spec.md`.

## M-26 (2026-11-15, branch `m26-sharp-interface`)

Built and run on an M4 Pro, macOS 27.2, Apple clang 21, CMake 4.5, Ninja,
RelWithDebInfo (`build/tests`, Qt off). The tests are in
`products/calc-u-1600/tests/`, program `CoreTests` (CTest entry of the same
name, label `frequent`), plus the CTests `CalcU1600CheckPresets` and
`CalcU1600MachineIncludeCheck`. Result of the run: `CoreTests` 850 tests,
63,146 checks passed, 0 failed, 38.1 s; the C1600-1 cases alone (29
tests, the golden replay included) 50,761 checks passed. `ctest --test-dir
build/tests`: 13 of 13 passed; the Calc-U-1600 part of Frequent 39.3 s.

| ID | Result | Test case (file in `products/calc-u-1600/tests/`) |
|---|---|---|
| AC-1 | Pass | `c1600_1_ac1_tick_is_cpu_cycle_or_t_state` (`sharp_machine_tests.cpp`; 1,300,000 on the PC-1500/1500A, 3,580,000 on the PC-1600; the LH5803's cycles counted in T-states) |
| AC-2 | Pass | `c1600_1_ac2_whole_instructions_of_the_bus_owner` (overshoot below the last instruction; on the PC-1600 the LH5803's steps while it has the bus) |
| AC-3 | Pass | `c1600_1_ac3_off_machine_counts_ticks_clock_runs_display_off` (core fix in this step: the PC-1600's display output said "on" while the sub-CPU had switched the system off) |
| AC-4 | Pass (one finding, see "Open") | `c1600_1_ac4_golden_runs_through_the_interface` (`golden_runs_tests.cpp`; the 23 golden runs, 3 of them recorded in T0 of this step before the core changed, replayed through `calcu::machine::Machine`) |
| AC-5 | Pass | all of `CoreTests` (the core tests named by AC-5 run unchanged in what they check, through the adapters where they reached the old classes) |
| AC-6 | Pass | `c1600_1_ac6_reset_keeps_memory`, `c1600_1_ac6_power_cycle_keeps_all_memory`, `c1600_1_ac6_cold_boot_empties_memory_and_ce151_ce155`, `c1600_1_ac6_cold_boot_keeps_battery_module`, `c1600_1_ac6_operations_end_at_key_readiness` (row 1600-16 in effect) |
| AC-7 | Pass | `c1600_1_ac7_off_key_then_on_runs_again` |
| AC-8 | Pass (reads two ways, see "Open") | `c1600_1_ac8_rtc_after_reset_and_cold_boot` |
| AC-9 | Pass | `c1600_1_ac9_pc1500_has_65_key_ids` |
| AC-10 | Pass | `c1600_1_ac10_pc1600_has_68_key_ids` |
| AC-11 | Pass | `c1600_1_ac11_on_switches_on_and_breaks` |
| AC-12 | Pass | `c1600_1_ac12_two_keys_and_on_down_together` |
| AC-13 | Pass | `c1600_1_ac13_dots_and_indicator_names_per_model` (156 × 7 and 14 indicators; 156 × 32 and 18) |
| AC-14 | Pass | `c1600_1_ac14_buzzer_48000_mono_same_at_any_slicing` (`BEEP 1` on each model, the samples the same in one advance and in slices) |
| AC-15 | Pass | `c1600_1_ac15_each_peripheral_only_through_places` |
| AC-16 | Pass | `c1600_1_ac16_ce150_cassette_listed_on_pc1500` (the media place in the descriptor; its capability is LIB-7's, M-47, LIB-1 D-1) |
| AC-17 | Pass | `c1600_1_ac17_first_peripheral_that_answers_wins_in_attach_order` |
| AC-18 | Pass | `c1600_1_ac18_ce1600p_variant_keeps_memory_and_disk_side` (one replace, one power cycle; design D-5) |
| AC-19 | Pass | `c1600_1_ac19_product_kinds_with_owners` |
| AC-20 | Pass | `c1600_1_ac20_display_on_slots_cassette_one_way_on_every_model` |
| AC-21 | Pass | `c1600_1_ac21_pc1500_ready_in_wait_4_kb_with_7b0e_bit0_clear`, `c1600_1_ac21_pc1500_not_ready_while_bit0_set_or_running`, `c1600_1_ac21_pc1600_ready_after_15_ready_frames` (design D-3) |
| EC-1 | Pass | `c1600_1_ec1_unknown_key_refused` |
| EC-2 | Pass | `c1600_1_ec2_off_steps_256_on_1600_tick_by_tick_on_1500` |
| EC-3 | Pass | `c1600_1_ec3_ce1601m_in_slot1_refused` |

Technical Requirements:
- **Golden runs:** AC-4 above.
- **No model above the machine:** CTest `CalcU1600MachineIncludeCheck`
  (`tools/check_machine_includes.sh`, Frequent, so in CI's `core-tests`
  job): no file in `app/`, `frontends/`, `tools/`, `dev/` includes
  `PC1500Machine.hpp`, `PC1600Machine.hpp` or a core header that brings
  one in, and none names the two classes outside a comment (design T13,
  D-9). Passes.

Other checks of the step (design T14):
- `pc1500_cli`, `pc1600_cli`, the three probes and the two loader
  matrices are on the interface. `tools/check_presets.sh`
  (`CalcU1600CheckPresets`): passes.
- M-4's references made again with the moved CLIs and compared with
  `tests/reference/v0.8.1-examples/`: 32 of 32 examples the same (`.out`,
  `.log`, `.paper`, exit code). Two things are masked because they differ
  with the CLIs before the move too: the path `Qt6/resources` →
  `frontends/qt6/resources` in the logs and the wall-clock time of
  `syncclock`.
- Loader matrices: PC-1500 72 of 80 (the same 8 ML-OVER cells fail as
  recorded in `dev/loader-matrix/README`), PC-1600 250 of 250.
- `pc1600_plotter_probe`: 26,887 points, as the reference. The power and
  UART probes run end to end.
- `core/Serial/PtySerialLink.*` and `LazySerialLink.hpp` are deleted: the
  library's host links (`calcu::peripherals::PtyHostLink`, `HostLinks`)
  took their place and nothing used them any more.
- Qt6 frontend (`build/all`, Calc-U-1600 only, Homebrew Qt): builds,
  after `core/Machine/LegacyCalls.hpp` stopped including
  `Preset/PresetMedia.hpp` (its member `slots` met Qt's macro).
- Web: `tools/build_wasm_check.sh` (Homebrew Emscripten): `wasm_smoke`
  all passed; `frontends/web/build.sh`: builds `dist/`. Playwright
  (`CalcU1600WebPlaywright`, Longer) was not run.

## Deviations from the design

- `SharpPlaces` (design §2, §3.4) is folded into `SharpMachine`: the
  places, the core attach and detach per place and the product refusals
  are there.
- One adapter more, `LegacyCalls::atOffPoint`: the session's changes that
  must wait for the off point of a power cycle (the disk into a CE-1600F
  plugged in then) go through it.
- A machine that switches itself off makes the waiting changes: an attach
  or detach on a running machine waits for an off point (D-2), and now
  also for the one the ROM reaches by itself; any change on a machine
  that is off makes the waiting ones first, in order.
- `core/HostClock.hpp` stays until M-46 (`core/Preset/PresetRunner.hpp`
  still uses it).
- The headers of the typing helpers (`PC1500TypedInput`,
  `PC1600TypedInput`) no longer include a machine class, so the app layer
  can use them under the include check.
- Key IDs are spelled as AC-9 and AC-10 list them (`core/Machine/
  SharpKeyIds.cpp`); the core maps them to its own key names.
- The PC-1500 machine is reset when the session rebuilds it, as the old
  session did on a rebuild.
- `sharpLcdFont` (design §3.5) reads the ROM font from the core's classes,
  not through `c1600.loader-memory`.
- A module change is a detach, an attach and a power cycle (behaviour
  change 1600-4); ledger rows 1600-4, 1600-16 and 1600-22 are in effect
  from this step.
- An attach of a CE-150 while a CE-1600P is plugged in is refused (the
  place `printer` takes one, LIB-2 AC-29), as is the CE-1600P with a
  CE-158 (1600-22).
- The tools (design §7): the WAV yield hook stays a hook
  (`LegacyCalls::setYieldHook`) instead of a drain between `advance`
  calls; the tools reach what they still read of the core through
  `LegacyCalls::coreParts`, `presetMedia`, `typeLine` (all go in M-51).
  Differences that no reference or matrix shows:
  - the probes' `t=` and settle figures use the real T-states of
    `advance(1)`;
  - the probes' display dumps say "(display off)" while the display is
    off (LIB-1 AC-43);
  - `--ce158-rx-hold` and the loader matrices' pauses and byte gaps are
    counted in character times (ticks at the port's baud rate, 300 if
    none is set), so a byte may come up to one character time later;
  - the loader matrices `static_cast` the serial port to
    `SerialPortQueues`;
  - the CLIs still do not set the clock from the host; the probes set a
    fixed date through `setClocksFromHost`;
  - the plotter probe still uses the module folder ".", as before.
- `dev/loader-matrix/build.sh` and `build_pc1600.sh` link
  `libcalcu_core.a` and `libcalcu_library.a` from `build/tools` (or
  `CALCU_BUILD`); still macOS only.

## Open (for the user)

Ruled 2026-11-15 (user): the status-words exception is accepted (option a,
not re-recorded); AC-8 keeps today's behaviour (spec text and decision
log updated). Both items below are settled.

- **Golden runs, status words while the display is off (AC-4).** In
  `pc1500a-reset-allreset` and `pc1600-reset-allreset` the record after
  `> reset` is taken while the display is off. The old text readers read
  the status line from its RAM even then (blank rows, `status DEG PRO
  I`); through the interface a display that is off shows no indicator
  (LIB-1 AC-43), so the replay has `status` there. The status words of a
  record taken while the display is off are therefore not compared; every
  other line, these two runs' included, is compared exactly. Not
  re-recorded: a finding for the user's ruling.
- **AC-8 reads two ways.** "A cold boot of the PC-1600 lets the ROM
  clear it": the core keeps a clock seeded from the host through the
  first boot after it (`PC1600Machine::seedClock`'s guard), as today, so
  a clock set before a cold boot survives that boot and the next cold
  boot clears it. The test follows today's behaviour.
- **AC-21, design D-3.** PC-1600 key readiness follows today's code (15
  ready frames in a row); a frame in which a running BASIC program polls
  for BREAK (reads the key matrix) counts as ready, so a running program
  can report key readiness.
- **EC-2.** A PC-1600 that switches itself off inside an advance may
  overshoot that advance by up to 255 ticks (the off steps of 256), as
  EC-2 says for a machine that is off.
