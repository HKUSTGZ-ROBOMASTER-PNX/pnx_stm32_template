---
document_type: engineering-implementation-plan
status: completed
created: "2026-08-05"
last_updated: "2026-08-06"
task_slug: "f407-ps2-validation"
project: "pnx-f4-minimal"
---

# Implementation Plan - F407 PS2 Validation

## Goal and baseline

- Goal: build the official latest PS2 mapping as an isolated F407 image.
- Baseline: parent `9f87072`, module `8ba925b`, existing uncommitted validation
  work preserved.
- Decision: `DR-F407-PS2-001`.

## Staged plan

| Stage | Change | Verification | Stop condition | Status |
|---:|---|---|---|---|
| 1 | Capture current Host and PS2 build baseline | CMake/CTest/build commands | Unexpected destructive or toolchain failure | completed |
| 2 | Add Host contract for new official mapping API | Observe compile/test failure on `8ba925b` | Failure is unrelated to missing API | completed |
| 3 | Checkout official module `f3a3274` and adapt telemetry | Focused Host test and PS2 build | Shared API/BSP incompatibility | completed |
| 4 | Verify graph isolation and regressions | Full Host suite and all F407 presets | Existing image changes behavior/closure | completed |
| 5 | Record evidence and remaining hardware gate | Diff review and validation report | Unsupported completion claim | completed |
| 6 | Correct physical UART binding to USART1 | Config contract plus live DMA evidence | No bytes on PB7 | completed |
| 7 | Fix startup/runtime stack faults | TDD lifecycle contract and ThreadX high-water | Any repeated fault | completed |
| 8 | Run attended controller validation | Live frames, Cross, left stick and mapping | Motor output or wrong DAP target | completed |
| 9 | Publish measured stack fix from modules `main@44ff9ab` | One-file diff, remote branch SHA | Any shared fix is reverted | completed |
| 10 | Refresh parent pin and software evidence | Host 37/37, six presets, closure/symbol/hash checks | Regression or unreachable gitlink | completed |

## Risk register

| Risk | Detection | Mitigation |
|---|---|---|
| H723 parent leaks into F407 | Parent diff/source scan | Never merge parent; update one gitlink only |
| New nested PS2 API breaks old demo | TDD compile failure | Adapt validation telemetry to official API |
| USART initialization affects unrelated ports | Compile definitions and source graph | Keep per-UART guards and assert only USART1 |
| Shared legacy RAM sections orphan at link | Link/map inspection | Retain explicit F407 linker placement |
| Existing presets regress | Fresh build matrix | Stop and fix before reporting completion |

## Git and hardware gates

- Commit and push were authorized on 2026-08-06 for the modules feature branch
  and parent `F4_version`. Merge to modules `main`, force push, rebase, tag,
  and branch deletion remain unauthorized.
- CubeMX regeneration and motor output: not authorized.
- Flash/debug operation was authorized for DAP `482752183208`; the other DAP
  `482752132243` remained untouched.

Decision: completed with scoped Git publication, without modules-main merge,
CubeMX regeneration, new hardware operation, or motor output
