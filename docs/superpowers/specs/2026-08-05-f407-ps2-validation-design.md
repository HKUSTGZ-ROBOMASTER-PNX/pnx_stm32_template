# F407 PS2 Validation Design

Status: approved by the user on 2026-08-05

Publication amendment, approved 2026-08-06: retain `f3a3274` as the mapping
provenance, but publish the measured PS2 stack correction from latest
`pnx_modules/main@44ff9ab` as `fix/f407-ps2-stack@42a4b35` and pin that commit.

## Objective

Make the UART PS2 receiver from the latest official `pnx_template` usable on
the DJI C-board STM32F407 without merging the H723 parent application into
`pnx_f4_minimal`.

## Compared approaches

1. Merge `upstream/main` into the F407 branch. Rejected: the parent histories
   have diverged and the merge replaces the F407 board graph with the H723
   graph across hundreds of files.
2. Copy the three PS2/Remoter source files into the existing submodule
   checkout. Rejected: this leaves an unpinned dirty submodule and loses the
   official provenance of the shared API.
3. Pin only `pnx_modules` to official commit
   `f3a32742c78ff5c531d4fd7211e9235fca3db74f`, then adapt the F407 validation
   image to that API. Selected: it preserves the F407 parent and Direct BSP
   while consuming the official PS2 mapping change as an atomic dependency.

## Architecture

The validation-only path is:

```text
PS2 UART receiver
  -> physical UART2 / MCU USART1 RX DMA on the F407 board
  -> F407 Direct BSP `bsp::usart`
  -> `remoter::ps2` at 9600 8N1
  -> `remoter::ps2_state::data`
  -> `remoter::service`
  -> unified `remoter::state`
  -> debugger-visible F407 validation telemetry
```

`f407-ps2-debug` is mutually exclusive with Core, USB CDC, and PWM/A2
applications. The normal Core/USB/PWM source graphs and their generated UART
initialization remain unchanged. Only the PS2 image may select the USART BSP,
message bus, Remoter service, and PS2 validation demo.

## Compatibility decisions

- Keep `pnx_bsp@da09febe8f5bfe66993010247d4d6731d0ca492b`; do not replace it
  with the H723-oriented template gitlink.
- Keep `pnx_devices` and `pnx_libs` at their current pins because the official
  template does not change them for this PS2 update.
- Consume the new PS2 mapping from the `f3a3274` lineage, published at
  `pnx_modules@42a4b35`: link state, buttons,
  edges, and event count live in the nested unified `state` object; raw axes,
  frame count, signal count, and last signal tick remain in `ps2_state`.
- Preserve a dedicated F407 validation demo even though the official parent
  removed its standalone PS2 demo. The F407 copy is validation infrastructure,
  not a second shared protocol implementation.
- Bind PS2 to USART1 and keep USART6 as the configured report port. USART3 is
  deliberately not used because it is the historical DR16/DBUS port.
- The PS2 image initializes only its bound USART. Do not restore the old
  all-USART initialization behavior.

## Failure behavior

- Configuration must fail if PS2 is selected without a real RX-DMA UART or if
  the PS2 and report UARTs conflict.
- Startup must leave validation telemetry failed when the Remoter service,
  subscriptions, or monitor thread cannot be created.
- A connected result requires a valid PS2 frame, an online unified Remoter
  state, `active_source == ps2`, and matching raw/unified button mappings.
- Receiver heartbeat `0xAB` means the receiver is online but the controller is
  disconnected. Absence of frames and heartbeat beyond the configured timeout
  means receiver offline.

## Verification

1. A Host contract test must fail against the old module because the new
   `state.ps2_*`, `ps2_button`, and nested `ps2_state.data` API is absent.
2. After the gitlink update and demo adaptation, the focused Host test and the
   full Host CTest suite must pass.
3. Fresh configure/build must pass for `f407-ps2-debug` and every existing F407
   preset with no observed compiler or linker warnings.
4. Source graph checks must show PS2/USART/Remoter only in the PS2 image and
   exactly one generated USART init selected there.
5. Runtime controller behavior requires a connected receiver and explicit
   flash/debug authorization. That attended gate passed on 2026-08-06; exact
   observations are retained in the task validation report and `HANDOFF.md`.

## Rollback

Restore the parent `pnx_modules` gitlink to `8ba925b`, remove the PS2-only
preset/source closure and validation files, and leave all established F407
presets untouched. No remote history rewrite is required.
