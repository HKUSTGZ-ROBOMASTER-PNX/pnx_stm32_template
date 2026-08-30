# Task brief

## Objective

Bring the F407 template to current shared modules/devices without importing
H7-only board code or weakening F407 Direct BSP and fail-closed boundaries.

## Baseline

- Parent dependency base: `717e4dc`
- BSP: `da09feb`
- Modules: `42a4b35`
- Devices: `2349cc1`
- Libraries: `e7c3e7a`
- Host: 37/37 PASS
- PS2 embedded build: PASS

## Constraints

- Local commits only; no push or hardware operation.
- Preserve the F407 PS2 1536 B stack.
- Preserve two unrelated pre-existing untracked task directories.
- Keep the downstream vehicle unchanged until user review.
