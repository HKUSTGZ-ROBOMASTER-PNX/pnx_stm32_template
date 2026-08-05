# F407 PS2 Validation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use
> superpowers:subagent-driven-development (recommended) or
> superpowers:executing-plans to implement this plan task-by-task. Steps use
> checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the latest official UART PS2 mapping as an isolated
STM32F407 validation image without importing the H723 parent graph.

**Architecture:** Keep the F407 parent and Direct USART BSP, consume the
official mapping introduced at `pnx_modules@f3a3274`, and adapt the branch-local
validation telemetry to the new nested `ps2_state.data` and unified
`state.ps2_*` API. Publication pins `fix/f407-ps2-stack@42a4b35`, based on
latest `main@44ff9ab`, so the measured PS2 stack fix and newer shared fixes are
both reproducible. All non-PS2 presets keep their explicit source closures.

**Tech Stack:** C++17, CMake/Ninja, GNU Arm Embedded 13.3.1, STM32F407 HAL,
ThreadX, CTest, Git submodules.

## Global Constraints

- Do not merge `upstream/main`; it is the H723 parent application.
- Keep `pnx_bsp@da09febe8f5bfe66993010247d4d6731d0ca492b`.
- Do not change `pnx_devices` or `pnx_libs` gitlinks.
- Do not regenerate CubeMX, enable motor output, tag, or modify remotes. The
  initial implementation did not authorize commit/push; a 2026-08-06 follow-up
  authorized the modules feature branch and parent `F4_version` publication.
  A separate attended follow-up authorized flash/debug only
  on DAP `482752183208`.
- Preserve pre-existing working-tree edits and `.codex` task packets.
- Record physical PS2 behavior only after attended evidence. The authorized
  2026-08-06 run is recorded in the validation report.

---

### Task 1: Capture the pre-integration baseline

**Files:**
- Read: `AGENTS.md`
- Read: `CMakeLists.txt`
- Read: `configs/boards/dji_c_board_f407/params.ps2.json`
- Read: `demo/ps2/ps2_demo.cpp`

**Interfaces:**
- Consumes: parent `9f87072`, `pnx_modules@8ba925b`, existing uncommitted PS2
  closure.
- Produces: exact baseline Host and cross-build results in the validation
  report.

- [ ] **Step 1: Verify the current branch and submodule are unchanged**

Run:

```powershell
git status --short --branch
git -C pnx_modules status --short --branch
git -C pnx_modules rev-parse HEAD
```

Expected: parent branch `feat/f407-ps2-validation`; module clean at
`8ba925b60b11fec511a57622c199b57bb23f8f4e`.

- [ ] **Step 2: Configure, build, and run the existing Host suite**

Run:

```powershell
cmake -S . -B build/host-ps2-validation -G Ninja -DPNX_HOST_TESTS=ON
cmake --build build/host-ps2-validation --parallel 4
ctest --test-dir build/host-ps2-validation --output-on-failure
```

Expected: existing tests pass. Any failure is recorded as baseline evidence
and stops implementation until its relation to the active WIP is understood.

- [ ] **Step 3: Configure and build the existing PS2 WIP image**

Run:

```powershell
cmake --fresh --preset f407-ps2-debug
cmake --build --preset f407-ps2-debug --parallel 4
```

Expected: either a passing old-API baseline or an attributable WIP failure.
Record the exact result; do not weaken the build.

### Task 2: Add the official PS2 mapping contract test

**Files:**
- Modify: `tests/host/shared_module_api_host_tests.cpp`

**Interfaces:**
- Consumes: official types `remoter::ps2_button`,
  `remoter::state::ps2_buttons`, `remoter::ps2_state::data`.
- Produces: a compile-time contract that rejects the old module revision.

- [ ] **Step 1: Add compile-time assertions for the new API**

Add after the existing `ps2_config` assertion:

```cpp
static_assert(std::is_same_v<
              decltype(remoter::state{}.ps2_buttons),
              std::uint16_t>);
static_assert(std::is_same_v<
              decltype(remoter::ps2_state{}.data),
              remoter::state>);
static_assert(std::is_same_v<
              decltype(remoter::ps2_state{}.frame_count),
              std::uint32_t>);
static_assert(
    static_cast<std::uint16_t>(remoter::ps2_button::square) == 0x8000U);
static_assert(
    remoter::is_held(0x4000U, remoter::ps2_button::cross));
```

- [ ] **Step 2: Run the focused target and verify RED**

Run:

```powershell
cmake --build build/host-ps2-validation --target pnx_shared_module_api_host_tests --parallel 4
```

Expected: compilation fails because module `8ba925b` lacks `state.ps2_buttons`,
`ps2_state.data`, or `ps2_button`. A syntax/path error is not an acceptable RED.

### Task 3: Pin the official module and make the contract GREEN

**Files:**
- Modify gitlink: `pnx_modules`
- Do not modify files inside the submodule.

**Interfaces:**
- Consumes: official commit
  `f3a32742c78ff5c531d4fd7211e9235fca3db74f`.
- Produces: the new PS2 mapping API at a remotely reachable atomic revision.

- [ ] **Step 1: Reconfirm the submodule is clean**

Run:

```powershell
git -C pnx_modules status --short
git -C pnx_modules cat-file -e f3a32742c78ff5c531d4fd7211e9235fca3db74f^{commit}
```

Expected: no dirty paths and exit 0 for the target commit.

- [ ] **Step 2: Switch only the submodule checkout**

Run:

```powershell
git -C pnx_modules switch --detach f3a32742c78ff5c531d4fd7211e9235fca3db74f
```

Expected: detached HEAD at `f3a3274`; parent status shows only the
`pnx_modules` gitlink as modified in addition to preserved WIP.

- [ ] **Step 3: Rebuild the focused Host target and verify GREEN**

Run:

```powershell
cmake --build build/host-ps2-validation --target pnx_shared_module_api_host_tests --parallel 4
ctest --test-dir build/host-ps2-validation -R '^shared_module_api$' --output-on-failure
```

Expected: build and one focused test pass.

### Task 4: Adapt the F407 validation telemetry to the official mapping

**Files:**
- Modify: `demo/ps2/ps2_demo.cpp`
- Modify: `demo/common/demo_debug.hpp`
- Modify: `demo/ps2/README.md`
- Preserve: `demo/cboard/ps2_validation/ps2_validation.cpp`

**Interfaces:**
- Consumes raw state:
  `ps2_state.data.ps2_link`, `ps2_state.data.ps2_buttons`,
  `ps2_state.data.ps2_pressed`, `ps2_state.data.ps2_released`,
  `ps2_state.frame_count`, `ps2_state.signal_count`, and
  `ps2_state.last_signal_tick`.
- Consumes unified state:
  `state.ps2_link`, `state.ps2_buttons`, `state.ps2_pressed`,
  `state.ps2_released`, and `state.ps2_event_count`.
- Produces debugger telemetry proving raw-to-unified mapping and button enum
  states without implementing a second decoder.

- [ ] **Step 1: Compile the PS2 image and verify the old demo fails**

Run:

```powershell
cmake --fresh --preset f407-ps2-debug
cmake --build --preset f407-ps2-debug --parallel 4
```

Expected: compile errors identify old direct fields such as `ps2_state.link`,
`buttons`, `pressed`, `released`, or `last_update_tick`.

- [ ] **Step 2: Change raw field access to the official nested layout**

Use the raw fields exactly as follows:

```cpp
const auto& raw_state = latest_raw.data;
state.link = static_cast<std::uint32_t>(raw_state.ps2_link);
state.buttons = raw_state.ps2_buttons;
state.pressed = raw_state.ps2_pressed;
state.released = raw_state.ps2_released;
state.last_update_tick = latest_raw.last_signal_tick;
state.connected_frame_count = latest_raw.frame_count;
```

Track an edge only when `raw_state.ps2_event_count` changes. Compare the
unified mapping with:

```cpp
const bool mapping_match =
    latest_generic.ps2_link == raw_state.ps2_link &&
    latest_generic.ps2_buttons == raw_state.ps2_buttons &&
    latest_generic.ps2_pressed == raw_state.ps2_pressed &&
    latest_generic.ps2_released == raw_state.ps2_released &&
    latest_generic.ps2_event_count == raw_state.ps2_event_count;
```

- [ ] **Step 3: Add debugger fields for official mapping evidence**

Add to `ps2_unit_state`:

```cpp
bool mapping_match = false;
std::uint32_t event_count = 0;
std::uint32_t frame_count = 0;
std::uint32_t signal_count = 0;
bool square = false;
bool cross = false;
bool circle = false;
bool triangle = false;
```

Populate buttons with `remoter::is_held` and the official `ps2_button` enum.
Do not duplicate the numeric masks in the demo.

- [ ] **Step 4: Rebuild the PS2 image and verify GREEN**

Run:

```powershell
cmake --fresh --preset f407-ps2-debug
cmake --build --preset f407-ps2-debug --parallel 4
```

Expected: configure, compile, and link exit 0 with no observed warning lines.

### Task 5: Verify F407 configuration and source-graph isolation

**Files:**
- Modify: `tests/host/config_generation_contract.cmake`
- Review: `CMakeLists.txt`
- Review: `boards/dji_c_board_f407/Core/Src/main.c`

**Interfaces:**
- Consumes: `params.ps2.json`, config generator, per-UART CMake variables.
- Produces: a repeatable contract for PS2 enabled on USART1 and default PS2
  disabled.

- [ ] **Step 1: Extend the config contract with a PS2 generation pass**

Generate into a second directory using `params.ps2.json` and
`-DPNX_ENABLE_PS2_VALIDATION=ON`, then require:

```text
inline constexpr bool enable_ps2 = 1
inline constexpr bsp::usart::port ps2 = bsp::usart::usart1
inline constexpr std::uint32_t ps2_offline_timeout_ticks = 600
inline constexpr std::uint32_t ps2_frame_timeout_ticks = 20
inline constexpr float ps2_deadzone = 0.08f
```

- [ ] **Step 2: Run the focused config contract**

Run:

```powershell
ctest --test-dir build/host-ps2-validation -R '^config_generation_contract$' --output-on-failure
```

Expected: the default-disabled and PS2-enabled generations both pass.

- [ ] **Step 3: Inspect actual Ninja graphs**

Run:

```powershell
ninja -C build/f407-debug -t commands
ninja -C build/f407-ps2-debug -t commands
```

Expected: Core contains no `bsp_usart.cpp`, `msg.cpp`, `remoter.cpp`, or
`ps2.cpp`; PS2 contains all four and compiles
`PNX_F407_USART1_ENABLED=1`, with USART3/USART6 disabled.

### Task 6: Run regression verification and finalize evidence

**Files:**
- Modify: `HANDOFF.md`
- Modify: `.codex/tasks/2026-08-05-f407-ps2-validation/validation-report.md`
- Modify: `.codex/tasks/2026-08-05-f407-ps2-validation/task-brief.md`
- Review: `README.md`

**Interfaces:**
- Consumes: all focused and matrix results.
- Produces: evidence-bounded software handoff; later attended hardware evidence
  is appended only after separate authorization.

- [ ] **Step 1: Run the complete Host suite**

Run:

```powershell
cmake --build build/host-ps2-validation --parallel 4
ctest --test-dir build/host-ps2-validation --output-on-failure
```

Expected: all tests pass with zero failures.

- [ ] **Step 2: Fresh-build every F407 preset**

Run each preset with `cmake --fresh --preset <name>` followed by
`cmake --build --preset <name> --parallel 4`:

```text
f407-debug
f407-release
f407-usb-cdc-debug
f407-usb-cdc-release
f407-pwm-a2-debug
f407-ps2-debug
```

Expected: all six configure/build/link operations exit 0 with no observed
compiler or linker warnings.

- [ ] **Step 3: Review repository integrity**

Run:

```powershell
git diff --check
git status --short --branch
git diff --stat
git diff --submodule=log
```

Expected: no whitespace errors; only PS2 validation/task documentation and the
single approved module gitlink differ. Preserve any pre-existing unrelated
files without staging or rewriting them.

- [ ] **Step 4: Record evidence without committing**

Update the validation report and handoff with exact command results, RAM/Flash
figures, submodule SHAs, and these boundaries:

```text
PS2_SOFTWARE_CLOSURE=<PASS or FAIL>
PRECURSOR_PS2_HARDWARE_LIVE_FRAME=PASS
PRECURSOR_FLASH_VERIFY=PASS
PNX_MODULES_FEATURE_BRANCH_PUSH=PASS
PARENT_PUBLICATION_TARGET=F4_version
HARDWARE_REVALIDATION_42A4B35=NOT_RUN
VAULT_UPDATE=NOT_RUN
```
