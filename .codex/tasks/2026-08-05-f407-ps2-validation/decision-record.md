---
document_type: engineering-decision-record
status: approved
created: "2026-08-05"
last_updated: "2026-08-06"
decision_id: "DR-F407-PS2-001"
task_slug: "f407-ps2-validation"
project: "pnx-f4-minimal"
---

# Decision Record - Selective Official PS2 Module Integration

## Context

`upstream/main@9430817` contains the desired PS2 mapping but is an H723 parent.
The F407 branch has 39 local commits after the shared merge base and an active
Direct-BSP architecture. A whole-parent merge changes 622 files and conflicts
with the active F407 work.

## Candidates

| Candidate | Result | Reason |
|---|---|---|
| Merge the complete official parent | Rejected | Replaces the F407 board/build graph with H723 content |
| Copy shared source into a dirty submodule | Rejected | Loses official gitlink provenance and reproducibility |
| Pin official PS2 mapping plus the measured F407 stack fix on a feature branch | Approved | Preserves current shared fixes and makes the F407 result reproducible without directly changing modules `main` |

## Approved decision

Keep the F407 parent and `pnx_bsp@da09feb`, update only the `pnx_modules`
gitlink, and adapt the isolated PS2 validation image to the new nested
PS2/unified state API. For publication, branch from official
`pnx_modules/main@44ff9ab`, add only the measured PS2 stack correction, and pin
`fix/f407-ps2-stack@42a4b35` in the parent.

## Consequences

- The PS2 validation demo changes to the official button enum, event counter,
  nested link state, raw axis, frame, and signal fields.
- Default images remain free of PS2/USART/Remoter consumers.
- The parent depends on remotely reachable `42a4b35`. The feature branch is
  published, while modules `main` remains at `44ff9ab` pending a separate
  review/merge decision.
- Hardware functionality requires a separately authorized attended test. That
  gate later passed on 2026-08-06; see `validation-report.md` for evidence.

## Human approval

- User response: "confirm recommended scope; use PS2 in my F4 pnx_f4_minimal"
- Date: 2026-08-05
- Scope authorized: pull/compare official PS2 changes and implement the
  F407-compatible validation closure.
- Publication approval: on 2026-08-06 the user authorized organized parent
  commits to `pnx_template/F4_version` and a new modules branch from
  `main@44ff9ab`.
