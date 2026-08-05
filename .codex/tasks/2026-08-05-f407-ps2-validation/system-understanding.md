---
document_type: engineering-system-understanding
status: software-validated-with-precursor-hardware-evidence
created: "2026-08-05"
last_updated: "2026-08-06"
task_slug: "f407-ps2-validation"
project: "pnx-f4-minimal"
---

# System Understanding - F407 PS2 Validation

## Context snapshot

```text
repository: pnx_f4_minimal
branch: feat/f407-ps2-validation
head: 9f8707280d56ae1c8f8061cb9ec12e75f527d1f8
working_tree: pre-existing uncommitted PS2 validation work
staged_state: none observed
applicable_instructions: root AGENTS.md
latest_handoff: HANDOFF.md Direct BSP ownership section
```

## Boundary and flow

```text
physical UART2 -> MCU USART1 RX DMA (PA9 TX / PB7 RX)
-> F407 Direct USART BSP -> PS2 parser
-> ps2_state.data -> Remoter merge thread -> unified state
-> F407 validation monitor -> demo_debug_instance
```

| Component | Responsibility | Failure effect |
|---|---|---|
| IOC/generated F407 USART | Pin/DMA/IRQ ownership | No RX events or invalid build binding |
| `pnx_bsp/usart` | Configure 9600 8N1 and deliver RX-to-idle frames | PS2 init fails closed |
| `pnx_modules/remoter` | Parse frames and map raw/unified state | Offline or incorrect controls |
| `pnx_libs/msg` | Publish PS2 and unified state | Service/subscriber creation fails |
| PS2 validation demo | Report mapping and timing through debugger | No observable acceptance evidence |

## Known limitations

- Official `upstream/main` is an H723 parent and cannot be merged wholesale.
- Physical UART labels do not match MCU instance numbers: C-board UART2 is
  USART1, while C-board UART1 is USART6.
- Connected-frame, Cross, left-stick, and raw/unified mapping behavior passed.
- That attended result belongs to the recorded `f3a3274`-based precursor
  artifact. The published `pnx_modules@42a4b35` image has fresh software but
  no new hardware evidence.
- Receiver power-loss recovery and exhaustive four-axis/all-button coverage
  were not run.

Decision: attended PS2 validation passed for the tested precursor artifact;
the published pin has complete software closure
