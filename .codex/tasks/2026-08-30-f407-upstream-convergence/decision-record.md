# Decision record

## Decision

Use selective forward convergence. Merge current shared modules main into the
published F407 PS2 branch, advance shared devices main, and pin the latest
published F407 BSP. Do not merge H7 parent or BSP main wholesale.

## Reason

The F407 PS2 stack correction and Direct BSP implementation are proven
board-specific differences. H7 BSP main's newest heating lease is not a public
watchdog contract: it directly depends on H7 TIM3_CH4, TIM6 DMA, Cortex-M7
DCache and D2 RAM. Copying it would either fail to build or falsely claim F407
hardware support.

## Rejected alternatives

- Freeze the old pins: leaves shared modules/devices behind.
- Use every shared main directly: loses the F407 PS2 stack and BSP ownership.
- Merge H7 BSP/template wholesale: imports incompatible board resources.

## Rollback

Before publication, return modules to `42a4b35`, devices to `2349cc1`, BSP to
`da09feb`, and the parent to `717e4dc`. The vehicle branch is untouched.
