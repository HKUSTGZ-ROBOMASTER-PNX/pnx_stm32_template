# F407 Upstream Convergence Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Bring the official F407 template line forward to the current shared modules/devices state while preserving the F407-specific BSP and PS2 runtime contracts.

**Architecture:** Keep `pnx_template:F4_version`, `pnx_bsp:F4_version_bsp`, and `pnx_modules:fix/f407-ps2-stack` as explicit F407 authorities. Merge shared module changes forward into the published F407 modules branch, update the parent gitlinks, and prove that no H7-only board closure becomes selected. The school-competition vehicle remains untouched until a separate user review.

**Tech Stack:** Git worktrees/submodules, CMake/Ninja, CTest, GNU Arm Embedded Toolchain, STM32F407/ThreadX.

## Global Constraints

- Do not copy H7 HAL handles, IOC resources, TIM6/DMA heating-lease code, Cortex-M7 DCache/D2 RAM code, startup code, linker code, or generated H7 board bindings into F407.
- F407 Direct BSP sources remain the only providers of selected `bsp::*` implementations.
- Unsupported F407 hardware remains unselected or reports `not_configured`; no stub reports successful hardware behavior.
- Preserve the PS2 1536-byte runtime stack and fail-closed startup behavior.
- Normal Core/USB images remain zero-motor and do not initialize unselected DM IMU, referee, PS2, PWM, CAN, or other optional closures.
- Preserve the two pre-existing untracked historical `.codex/tasks/` directories.
- Create local commits only. Do not push, flash hardware, modify remotes, rebase published history, or touch `chassis_x_arm`.

---

### Task 1: Capture baseline and verify the published branch topology

**Files:**
- Read: `.gitmodules`
- Read: `CMakePresets.json`
- Read: `tests/host/CMakeLists.txt`
- Read: `pnx_modules/remoter/include/remoter.hpp`
- Read: `pnx_bsp/pwm/src/bsp_pwm.cpp`

**Interfaces:**
- Consumes: parent `354e257`, BSP `da09feb`, modules `42a4b35`, devices `2349cc1`, libs `e7c3e7a`.
- Produces: a fresh baseline test/build record and an exact before-state for later validation.

- [ ] **Step 1: Verify clean/known working state**

Run from `C:\Users\USER\Desktop\RM\rm_inschool\2026\firmware\pnx_f4_minimal`:

```powershell
git status --short --branch
git submodule status
git ls-tree HEAD pnx_bsp pnx_devices pnx_libs pnx_modules
```

Expected: only the two known untracked historical task directories; no tracked parent or submodule modification.

- [ ] **Step 2: Run the pre-change Host baseline**

```powershell
cmake -S . -B build/host-upstream-convergence -G Ninja -DPNX_HOST_TESTS=ON
cmake --build build/host-upstream-convergence --parallel 4
ctest --test-dir build/host-upstream-convergence --output-on-failure
```

Expected: configure/build succeeds and every discovered test passes. Record the exact count rather than assuming the earlier 29-test count.

- [ ] **Step 3: Run the pre-change PS2 build baseline**

```powershell
cmake --fresh --preset f407-ps2-debug
cmake --build --preset f407-ps2-debug --parallel 4
```

Expected: `pnx_embedded.elf` links successfully before any pin changes.

### Task 2: Converge the F407 modules branch with shared main

**Files:**
- Modify through merge: `pnx_modules/ahrs/include/ahrs.hpp`
- Modify through merge: `pnx_modules/ahrs/src/ahrs.cpp`
- Modify through merge: `pnx_modules/referee/include/referee.hpp`
- Modify through merge: `pnx_modules/referee/include/referee_protocol.hpp`
- Modify through merge: `pnx_modules/referee/src/referee.cpp`
- Preserve: `pnx_modules/remoter/include/remoter.hpp`

**Interfaces:**
- Consumes: `pnx_modules:fix/f407-ps2-stack@42a4b35` and `pnx_modules:main@5be2240`.
- Produces: one local non-rewriting merge commit containing shared DM IMU/AHRS and referee updates plus the F407 PS2 stack fix.

- [ ] **Step 1: Confirm the merge is conflict-free before committing**

In `C:\Users\USER\Desktop\RM\rm_inschool\2026\firmware\.codex-worktrees\pnx_modules-f407-ps2-stack`:

```powershell
git status --short --branch
git merge-base fix/f407-ps2-stack origin/main
git merge-tree 44ff9ab7da4335fb35f9bbed1a1c2a3e4f70d9c5 fix/f407-ps2-stack origin/main
```

Expected: the branch is clean; the merge adds only AHRS/referee main changes and reports no conflict marker.

- [ ] **Step 2: Merge shared main without rewriting published F407 history**

```powershell
git merge --no-ff origin/main -m "merge(modules): synchronize F407 PS2 branch with main"
```

Expected: one merge commit; `remoter/include/remoter.hpp` still contains the 1536-byte F407 PS2 runtime stack.

- [ ] **Step 3: Verify the retained F407 delta**

```powershell
git diff 5be224098caab9dcccbed38a34e0e65f0d0dad9f..HEAD -- remoter/include/remoter.hpp
git status --short --branch
```

Expected: the only F407-specific source delta against modules main is the PS2 runtime stack adjustment; worktree is clean and ahead only locally.

### Task 3: Update the F407 parent policy and submodule pins

**Files:**
- Modify: `.gitmodules`
- Modify gitlink: `pnx_bsp`
- Modify gitlink: `pnx_modules`
- Modify gitlink: `pnx_devices`
- Preserve gitlink: `pnx_libs`
- Modify: `AGENTS.md`
- Modify: `README.md`

**Interfaces:**
- Consumes: `F4_version_bsp@ee59c97`, the Task 2 modules merge commit, `pnx_devices@5c418a4`, `pnx_libs@e7c3e7a`.
- Produces: an explicit, reviewable F407 dependency policy and parent gitlinks pointing to the validated local commits.

- [ ] **Step 1: Add the F407 modules branch policy**

Add this line under `[submodule "pnx_modules"]` in `.gitmodules`:

```ini
	branch = fix/f407-ps2-stack
```

Keep the existing HTTPS URL. Do not change devices/libs branch behavior or any remote URL.

- [ ] **Step 2: Check out the selected submodule commits**

```powershell
git -C pnx_bsp checkout --detach ee59c97a852586eb7f6fb50b402f2fe9ed112cbc
$moduleWorktree = 'C:\Users\USER\Desktop\RM\rm_inschool\2026\firmware\.codex-worktrees\pnx_modules-f407-ps2-stack'
$modulesMerge = git -C $moduleWorktree rev-parse HEAD
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
git -C pnx_modules checkout --detach $modulesMerge
git -C pnx_devices checkout --detach 5c418a4d2c11bf56dddc81b8e374146b659d4c2a
git -C pnx_libs checkout --detach e7c3e7a2b9d825586ab3e0c413877180c4295df8
```

Expected: only `.gitmodules`, the BSP/modules/devices gitlinks, and planned documentation are modified. Record `$modulesMerge` as the exact local modules commit.

- [ ] **Step 3: Record ownership in repository instructions**

Update the existing dependency language in `AGENTS.md` without weakening its Direct BSP or fail-closed rules. Record exactly:

```text
F407 tracks pnx_bsp/F4_version_bsp and pnx_modules/fix/f407-ps2-stack.
The F407 modules branch absorbs shared main changes without rewriting its
published PS2 stack history. pnx_devices and pnx_libs remain shared main.
H7-only generated bindings and board resources are not F407 dependencies.
```

- [ ] **Step 4: Update the README dependency table**

Add or update one concise table listing the four selected branches and exact local pins. State that the modules merge SHA is local-only until user-approved publication. Do not add H7 feature documentation to the F407 README.

- [ ] **Step 5: Review the parent diff before testing**

```powershell
git diff -- .gitmodules AGENTS.md README.md
git diff --submodule=log -- pnx_bsp pnx_modules pnx_devices pnx_libs
git diff --check
```

Expected: no H7 board source change and no whitespace error.

### Task 4: Validate closure isolation and all affected F407 products

**Files:**
- Read: `CMakeLists.txt`
- Read: `configs/cmake/generate_config.cmake`
- Read: generated build command graphs under `build/`
- Modify only if a real regression is found: the narrowest affected F407 contract/test file.

**Interfaces:**
- Consumes: the updated parent worktree from Task 3.
- Produces: fresh evidence that shared source availability does not select H7-only or optional runtime closures.

- [ ] **Step 1: Run focused contract tests**

```powershell
cmake -S . -B build/host-upstream-convergence -G Ninja -DPNX_HOST_TESTS=ON
cmake --build build/host-upstream-convergence --parallel 4
ctest --test-dir build/host-upstream-convergence -R "^(shared_module_api|config_generation_contract|direct_bsp_source_layout|ps2_startup_lifecycle|pwm_contract|usart_contract)$" --output-on-failure
```

Expected: all selected tests pass. Diagnose any failure; do not weaken a contract to accept an H7-only source.

- [ ] **Step 2: Build every F407 preset affected by the dependency change**

```powershell
$presets = @(
  'f407-debug',
  'f407-release',
  'f407-usb-cdc-debug',
  'f407-usb-cdc-release',
  'f407-pwm-a2-debug',
  'f407-ps2-debug'
)
foreach ($preset in $presets) {
  cmake --fresh --preset $preset
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
  cmake --build --preset $preset --parallel 4
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
```

Expected: all six presets configure, compile and link successfully.

- [ ] **Step 3: Prove H7-only closures are absent**

For every preset, inspect Ninja's actual command graph:

```powershell
$forbidden = 'dmimu|referee|TIM6|heater_lease|ram_d2_bss|stm32h7|bsp_bindings.cpp'
foreach ($preset in $presets) {
  $commands = ninja -C "build/$preset" -t commands
  $hits = $commands | Select-String -Pattern $forbidden -CaseSensitive:$false
  if ($hits) { $hits; throw "forbidden closure selected by $preset" }
}
```

Expected: zero hits. Mentions in dependency source directories that are not compile commands do not count; this command examines selected compile/link commands only.

- [ ] **Step 4: Run the full Host suite once**

```powershell
ctest --test-dir build/host-upstream-convergence --output-on-failure
```

Expected: every discovered Host test passes. Record the exact count.

### Task 5: Record validation and create the local parent commit

**Files:**
- Modify: `HANDOFF.md`
- Modify: `VAULT_UPDATE.md`
- Modify: `.codex/tasks/2026-08-30-f407-upstream-convergence/README.md`
- Modify: `.codex/tasks/2026-08-30-f407-upstream-convergence/task-brief.md`
- Modify: `.codex/tasks/2026-08-30-f407-upstream-convergence/decision-record.md`
- Modify: `.codex/tasks/2026-08-30-f407-upstream-convergence/validation-report.md`

**Interfaces:**
- Consumes: exact commits and command results from Tasks 1–4.
- Produces: one reviewable local F407 parent commit and a compact evidence record; no Vault modification and no push.

- [ ] **Step 1: Write the compact engineering record**

Record only:

- objective and authority matrix;
- before/after parent and four submodule commits;
- why H7 heating lease/generated bindings were excluded;
- exact Host test count and six preset results;
- closure scan result;
- `HARDWARE=NOT_RUN`, `PUSH=NOT_RUN`, `VAULT=NOT_MODIFIED`.

Do not duplicate full command logs or create review snapshot diffs.

- [ ] **Step 2: Update top-level handoff documents**

Add one dated section to `HANDOFF.md` and one concise proposed entry to `VAULT_UPDATE.md`. Clearly separate local commits from published state and do not claim official branches have changed.

- [ ] **Step 3: Review and commit only planned files**

```powershell
git diff --check
git status --short
git diff --submodule=log
git add .gitmodules AGENTS.md README.md HANDOFF.md VAULT_UPDATE.md `
  .codex/tasks/2026-08-30-f407-upstream-convergence `
  docs/superpowers/plans/2026-08-30-f407-upstream-convergence.md `
  pnx_bsp pnx_modules pnx_devices
git commit -m "chore(f407): converge shared dependencies"
```

Expected: the two pre-existing untracked task directories remain untracked and unmodified; the new parent commit is local only.

### Task 6: Remove only superseded clean worktrees

**Files:**
- Remove worktree: `C:\Users\USER\Desktop\RM\rm_inschool\2026\firmware\pnx_f4_bsp_pin_update`
- Remove worktree: `C:\Users\USER\Desktop\RM\rm_inschool\2026\firmware\pnx_f4_chassis_clean`
- Remove worktree: `C:\Users\USER\Desktop\RM\rm_inschool\2026\firmware\.claude-worktrees\pnx_f4_mycar-arm-direct-bsp-sync`

**Interfaces:**
- Consumes: clean worktrees whose commits are already reachable from retained branches/history.
- Produces: a smaller local worktree set without deleting any branch or remote history.

- [ ] **Step 1: Re-verify exact targets and cleanliness**

```powershell
$targets = @(
  'C:\Users\USER\Desktop\RM\rm_inschool\2026\firmware\pnx_f4_bsp_pin_update',
  'C:\Users\USER\Desktop\RM\rm_inschool\2026\firmware\pnx_f4_chassis_clean',
  'C:\Users\USER\Desktop\RM\rm_inschool\2026\firmware\.claude-worktrees\pnx_f4_mycar-arm-direct-bsp-sync'
)
$targets | ForEach-Object {
  $resolved = (Resolve-Path -LiteralPath $_).Path
  if (-not $resolved.StartsWith('C:\Users\USER\Desktop\RM\rm_inschool\2026\firmware\')) {
    throw "refusing unexpected worktree path: $resolved"
  }
  git -C $resolved status --porcelain=v1
}
```

Expected: all three paths resolve under the intended firmware directory and produce no status output.

- [ ] **Step 2: Remove worktrees through Git**

From the retained `pnx_f4_minimal` worktree:

```powershell
$targets | ForEach-Object {
  git worktree remove $_
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
git worktree list
```

Expected: all three directories disappear from the worktree list; their branches remain. Do not use `--force`, and do not delete branches.

- [ ] **Step 3: Final local-only checkpoint**

```powershell
git status --short --branch
git log -2 --oneline
git submodule status
git worktree list
```

Expected: only the two preserved historical task directories are untracked; parent and all selected submodules are clean; no push or hardware operation occurred.
