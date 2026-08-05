---
document_type: engineering-task-brief
status: software-validated-with-precursor-hardware-evidence
created: "2026-08-05"
last_updated: "2026-08-06"
task_slug: "f407-ps2-validation"
project: "pnx-f4-minimal"
---

# Task Brief - F407 PS2 Validation

## Task identity

- Requester: repository owner
- Owner: Codex
- Repository: `pnx_f4_minimal`
- Vault project: not required for this repository-scoped task
- Workflow track: standard
- Current status: publication pin software-validated; attended PS2 evidence is
  retained for the precursor artifact

## Problem to solve

The F407 repository contains the former PS2 shared source and configuration
surface, but its default graph does not build a PS2 application. The official
template now has a newer PS2 mapping API in `pnx_modules@f3a3274`; merging the
whole H723 parent is not compatible with the F407 architecture.

## Deliverables

- [x] Pin the official latest PS2/Remoter module revision without replacing
      the F407 Direct BSP.
- [x] Provide an isolated `f407-ps2-debug` image using USART1 RX DMA.
- [x] Expose raw and unified PS2 mapping evidence through debugger telemetry.
- [x] Preserve all existing F407 application graphs and tests.
- [x] Validate receiver link state, connected frames, Cross, left stick, raw to
      unified mapping, and thread stack headroom on the F407 C-board.
- [x] Publish the measured PS2 stack fix from `pnx_modules/main@44ff9ab` on a
      feature branch and pin that reachable commit in the parent.

## Constraints

- Do not merge `upstream/main` parent content.
- Do not replace the F407 `pnx_bsp` gitlink.
- Do not enable motor output, regenerate CubeMX, push, or commit without
  separate authorization. On 2026-08-06, commit/push was authorized only for
  the modules feature branch and parent `F4_version`; modules `main` remains
  unchanged. Flash/debug authorization applied only to the earlier attended
  PS2 test.
- Preserve all pre-existing working-tree changes and task records.

## Acceptance plan

| Deliverable | Acceptance method | Pass criterion |
|---|---|---|
| Shared API | Host red/green contract | New PS2 mapping symbols compile and assertions pass |
| PS2 image | Fresh cross build | Configure, compile, and link exit 0 |
| Regression | Host and all F407 presets | Existing tests/builds remain passing |
| Isolation | Build graph inspection | PS2 sources absent from non-PS2 images |
| Hardware | Attended validation | Live frames, Cross, left stick and mapping pass without a fault |

Decision: publication software deliverables complete; precursor hardware
evidence retained with explicit artifact scope

Reason: the user approved the selective F407-only integration boundary on
2026-08-05, authorized the attended flash/debug run on 2026-08-06, and later
authorized publication to the modules feature branch and parent `F4_version`.
Fresh Host, build-matrix, graph, ELF-symbol, and linker-map verification passed
at `pnx_modules@42a4b35`; live-frame, input mapping, and stack high-water
evidence remains tied to the recorded precursor artifact.
