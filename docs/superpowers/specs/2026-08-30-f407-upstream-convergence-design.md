# F407 upstream convergence design

Date: 2026-08-30

## Objective

Maintain two explicit deliverables without mixing their ownership:

1. `HKUSTGZ-ROBOMASTER-PNX/pnx_template:F4_version` is the pure DJI
   C-board STM32F407 architecture and validation line.
2. `shiuhou/pnx_newtemplate_F4:chassis_x_arm` is the school-competition
   vehicle line built on the accepted F407 architecture.

This task updates and validates the pure F407 line first. The vehicle line
will not be changed until the user reviews the F407 result.

## Authoritative repositories and branches

| Component | F407 authority | Relationship to H7 |
|---|---|---|
| Parent template | `pnx_template:F4_version` | Separate board/product branch |
| BSP | `pnx_bsp:F4_version_bsp` | Separate implementation branch |
| Modules | `pnx_modules:fix/f407-ps2-stack` | F407 branch periodically absorbs `main` |
| Devices | `pnx_devices:main` | Shared |
| Libraries | `pnx_libs:main` | Shared |
| Vehicle | `shiuhou/pnx_newtemplate_F4:chassis_x_arm` | Separate downstream product |

The H7 template and BSP remain authoritative for H7 board resources. Their
board-generated files, HAL handles and H7-only runtime mechanisms are not
portable contracts and must not be copied into F407.

## Baseline

- Local F407 parent: `feat/f407-ps2-validation@354e257`, equal to the current
  official `pnx_template:F4_version`.
- F407 BSP pin: `da09feb`; latest published `F4_version_bsp` is `ee59c97`.
- F407 modules pin: `42a4b35`, the published PS2 stack branch.
- Devices pin: `2349cc1`; current shared main is `5c418a4` and adds DM IMU.
- Libraries pin and shared main are both `e7c3e7a`.
- Current H7 template main is `5378de1`; current H7 BSP main is `7fd24f1`;
  current modules main is `5be2240`.
- The parent working tree has two pre-existing untracked historical task
  directories. They are not part of this task and must be preserved.

## Selected approach

Use selective forward convergence. Do not merge H7 parent or BSP history
wholesale into the F407 branches.

### Modules

Merge the latest `pnx_modules:main` into the published
`fix/f407-ps2-stack` branch without rewriting published history. Preserve the
F407 PS2 1536-byte runtime-stack correction. The result contains the shared
DM IMU/AHRS and referee changes while retaining the F407-specific PS2 fix.

If the merge changes the remoter contract or PS2 runtime behavior, preserve
the existing F407 mapping and fail-closed startup behavior and add focused
tests. Do not introduce a second F407-only copy of otherwise shared modules.

### BSP

Advance the parent pin to `F4_version_bsp@ee59c97`. Keep the Direct BSP model
already required by the F407 repository rules.

The following H7-main changes were reviewed but are excluded from direct
copying:

- The PWM heating lease in `7fd24f1` is tied to H7 `TIM3_CH4`, `TIM6` DMA,
  Cortex-M7 DCache and D2 RAM. F407 must not claim or emulate this hardware.
- H7 generated HAL bindings must not replace the F407 Direct BSP sources.
- H7 IOC, timer, DMA, startup, linker and board-handle changes remain H7-only.

CAN restart/diagnostics, GPIO, ADC, PWM and enabled-resource APIs are ported
only when an existing selected F407 closure or shared consumer requires the
public contract. Any port must use real F407 resources and return
`not_configured` or remain unselected when the resource is absent. No stub may
report successful hardware behavior.

### Devices and libraries

Advance `pnx_devices` to shared main `5c418a4`. DM IMU source availability
does not select or initialize the device in a normal F407 image. Keep
`pnx_libs@e7c3e7a` unchanged.

### Parent template

Update only the required gitlinks, configuration closure and tests. Do not
copy H7 DM IMU/referee demos, H7 TIM6 heating configuration or unrelated H7
generated board files. The F407 Core image remains zero-motor and optional
hardware remains explicitly selected.

## Validation gates

1. Record the exact pre-change parent and submodule commits.
2. Run focused modules/remoter/PS2 tests after the modules merge.
3. Run BSP and configuration contract tests after pin/config changes.
4. Run the full F407 Host CTest suite once before the local parent commit.
5. Build every F407 preset affected by the changed closure, including Core,
   USB, PWM and PS2 validation.
6. Confirm normal images do not acquire unselected DM IMU, referee, H7 timer,
   DMA or heating-lease sources.
7. Record exact local commits, build results and unresolved hardware gates.
8. Do not flash hardware and do not push any repository in this phase.

## Vehicle follow-up gate

Only after user review of the F407 result:

- synchronize `chassis_x_arm` to the accepted F407 parent/submodule state;
- change the validated J3 `servo_pe14` seed from `1500` to `900`;
- run combined Host and embedded validation;
- request a second review before pushing the personal vehicle branch.

## Worktree cleanup

After verifying they remain clean and their commits are reachable, remove
only these superseded worktrees, not their Git history:

- `pnx_f4_bsp_pin_update`;
- `pnx_f4_chassis_clean`;
- `.claude-worktrees/pnx_f4_mycar-arm-direct-bsp-sync`.

Preserve `pnx_f4_arm_clean` until the J3 value is transferred, and preserve
all dirty MaixCam, old ARM and task-record worktrees until separately audited.

## Alternatives rejected

- **Merge H7 main wholesale into F407:** rejected because it imports H7 HAL,
  IOC, timer/DMA and generated-binding ownership into the F407 board line.
- **Freeze every existing F407 pin:** rejected because modules/devices would
  drift from shared development and the F407 BSP pin is already behind its
  own published branch.
- **Move F407 back to every shared main branch:** rejected because BSP and
  PS2 stack contain proven F407-specific implementation/runtime differences.

## Rollback

All work is performed as local commits on the existing F407 branches. Before
push, rollback is branch-local: return the parent to `354e257`, BSP to
`ee59c97` or its prior pin as appropriate, modules to `42a4b35`, devices to
`2349cc1`, and leave the vehicle branch untouched.
