---
document_type: engineering-validation-report
status: software-validated-with-precursor-hardware-evidence
created: "2026-08-05"
last_updated: "2026-08-06"
task_slug: "f407-ps2-validation"
project: "pnx-f4-minimal"
---

# Validation Report - F407 PS2 Validation

## Repository and environment

```text
repository: pnx_f4_minimal
branch: feat/f407-ps2-validation
baseline commit: 9f8707280d56ae1c8f8061cb9ec12e75f527d1f8
baseline pnx_modules: 8ba925b60b11fec511a57622c199b57bb23f8f4e
mapping provenance: f3a32742c78ff5c531d4fd7211e9235fca3db74f
publication base pnx_modules: 44ff9ab7da4335fb35f9bbed1a1c2a3e4f70d9c5
target pnx_modules: 42a4b35b63ef7b8528ad756e03d17f1c09bab9af
hardware: precursor artifact PASS on DAP 482752183208; target not reflashed
date: 2026-08-06
```

## Baseline and TDD evidence

- Baseline module `8ba925b`: Host CTest `36/36 PASS`.
- Baseline PS2 image: RAM `53,024 B`, Flash `48,068 B`.
- New mapping contract RED: the old module failed compilation because
  `state.ps2_buttons` and `ps2_state.frame_count` were absent.
- After switching only `pnx_modules` to `f3a3274`, the focused Host contract
  passed and the old demo failed on its obsolete direct PS2 fields.
- Startup lifecycle RED exposed pre-scheduler mutex use; moving service init
  and subscriptions into the monitor thread made it GREEN.
- Live ThreadX diagnostics exposed 768-byte overflows in the validation
  monitor, unified Remoter service, and PS2 driver. Each stack-capacity
  contract failed before the corresponding stack was increased to 1536 B.
- The publication commit was recreated from latest `pnx_modules/main@44ff9ab`
  so its shared `wheel`, DR16 stack, and service stack changes are retained;
  only the missing PS2 stack correction was added.

## Test matrix

| Test | Expected | Observed | Status |
|---|---|---|---|
| Baseline Host CTest | Existing suite passes | `36/36 PASS` on `8ba925b` | pass |
| TDD new mapping RED | Fails on missing new API | Expected missing-field compile failure | pass |
| TDD new mapping GREEN | Passes with `f3a3274` | Focused contract `1/1 PASS` | pass |
| PS2 config contracts | Success plus conflict/RX-DMA rejection | Focused CTest `1/1 PASS` | pass |
| Fresh Host suite | All tests pass | `37/37 PASS` | pass |
| Six F407 presets | Fresh configure/build/link, warning-free | Six passed, zero warning lines | pass |
| Source graph isolation | PS2 sources only in PS2 image | Non-PS2 `0`; PS2 sources each `1` | pass |
| USART init isolation | PS2 initializes only USART1 | Five ELFs none; PS2 ELF USART1 only | pass |
| Legacy RAM placement | No orphan PS2 shared sections | D1 32 B at `0x20005300`; D2/D3 0 B at `0x20005320` | pass |
| UART electrical path | Bytes reach PB7 through DMA2 Stream2 | `0xAB` filled DMA buffer, UART error 0 | pass |
| Hardware PS2 behavior | Attended GDB telemetry on precursor artifact | Connected frames, Cross, left stick and mapping passed | pass, precursor only |
| Runtime stack margin | No ThreadX overflow under connected input on precursor artifact | PS2 628 B, service 532 B, monitor 508 B free | pass, precursor only |

## Fresh build matrix

| Preset | Result | Warnings | RAM | Flash |
|---|---|---:|---:|---:|
| `f407-debug` | pass | 0 | 49,792 B | 23,736 B |
| `f407-release` | pass | 0 | 49,792 B | 15,188 B |
| `f407-usb-cdc-debug` | pass | 0 | 66,040 B | 67,952 B |
| `f407-usb-cdc-release` | pass | 0 | 66,000 B | 40,056 B |
| `f407-pwm-a2-debug` | pass | 0 | 49,864 B | 27,656 B |
| `f407-ps2-debug` | pass | 0 | 55,456 B | 50,856 B |

The current software artifact at `pnx_modules@42a4b35` has SHA-256
`7950C1561B81C8B037E500EA39629D8FCA89419EE20F2BB114C70E3F0F69E2C0`.

## Attended hardware evidence - precursor artifact

This evidence applies to the earlier `f3a3274`-based artifact with the same
three 1536-byte runtime stacks. Its SHA-256 is
`F7BE6A16A009118715E53A936762D8F9E6E386604D7A98BA182C2B5B92F436A0`.
The current `42a4b35` artifact was not reflashed.

- Only CMSIS-DAP serial `482752183208` was selected; `482752132243` was not
  touched.
- The receiver was connected to physical C-board `UART2`, which maps to MCU
  USART1 on PA9/PB7. USART1 ran at 9600 8N1 with DMA2 Stream2 and no UART error.
- Before controller pairing, the DMA and parser received repeated `0xAB`,
  correctly producing `link=1` and zero connected frames.
- After pairing, telemetry reached `passed=true`, `stage_mask=0x1ff`,
  `failure_mask=0`, `link=0`, `generic_source=3`, and `mapping_match=true`.
- Connected frames reached 14,478 during the final stability snapshot. Cross
  produced `buttons=0x4000` and `cross=true`; the left-stick extreme produced
  raw Y 0 and normalized `left_y=1.0`.
- Motor output remained zero throughout.

## Validation conclusion

Status: current software closure passed; scoped precursor hardware closure passed

Supported completion claim: the published PS2 module pin configures, compiles,
and links in the isolated F407 USART1 RX-DMA validation image while all existing
F407 presets and Host tests remain passing. Live controller input passed on the
recorded precursor artifact; no live claim is made for the current ELF.

```text
PS2_SOFTWARE_CLOSURE=PASS
PRECURSOR_PS2_HARDWARE_LIVE_FRAME=PASS
PRECURSOR_PS2_CROSS_MAPPING=PASS
PRECURSOR_PS2_LEFT_STICK_MAPPING=PASS
PRECURSOR_FLASH_VERIFY=PASS
PRECURSOR_MOTOR_OUTPUT=ZERO
PNX_MODULES_FEATURE_BRANCH_PUSH=PASS
PARENT_PUBLICATION_TARGET=F4_version
HARDWARE_REVALIDATION_42A4B35=NOT_RUN
VAULT_UPDATE=NOT_RUN
```

Receiver unplug/offline recovery, exhaustive button/axis coverage, and a
separate long-duration soak remain outside this attended validation scope.
