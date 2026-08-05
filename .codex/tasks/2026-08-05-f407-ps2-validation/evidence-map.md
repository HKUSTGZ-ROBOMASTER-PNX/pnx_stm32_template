---
document_type: engineering-evidence-map
status: software-validated-with-precursor-hardware-evidence
created: "2026-08-05"
last_updated: "2026-08-06"
task_slug: "f407-ps2-validation"
project: "pnx-f4-minimal"
---

# Evidence Map - F407 PS2 Validation

| ID | Claim | Class | Evidence | Status |
|---|---|---|---|---|
| E1 | Official parent latest fetched state is `9430817` | Verified fact | `git rev-parse upstream/main` | verified |
| E2 | Official PS2 module revision is `f3a3274` | Verified fact | `git ls-tree upstream/main pnx_modules` | verified |
| E3 | New shared change touches only Remoter PS2 mapping files | Verified fact | `git diff --name-status 8ba925b..f3a3274` | verified |
| E4 | Whole-parent merge is incompatible with the F407 scope | Verified fact | 622-file parent diff including H723 board replacement | verified |
| E5 | Physical UART2 is MCU USART1 with RX DMA | Verified fact | DJI C-board manual, IOC PA9/PB7 and DMA2 Stream2, live RX | verified |
| E6 | `42a4b35` software image compiles and preserves regressions | Verified fact | Fresh six-preset builds and Host CTest 37/37 | verified |
| E7 | PS2 closure remains isolated from existing images | Verified fact | Ninja source counts and ELF USART-init symbols | verified |
| E8 | Legacy shared RAM sections are placed in F407 RAM | Verified fact | `f407-ps2-debug` linker map | verified |
| E9 | Physical receiver/controller works on USART1 for the precursor artifact | Verified fact | Attended GDB telemetry: connected frames, Cross and left stick; artifact `F7BE...` | verified |
| E10 | PS2 startup and runtime stacks are sufficient | Verified fact | Three RED/GREEN lifecycle contracts plus ThreadX high-water telemetry | verified |
| E11 | Publication modules base is official `main@44ff9ab` | Verified fact | `git ls-remote origin refs/heads/main` before branch push | verified |
| E12 | PS2 stack fix is remotely reachable without modifying modules `main` | Verified fact | `fix/f407-ps2-stack@42a4b35`; remote `main` remained `44ff9ab` | verified |

## Evidence gaps

| Needed claim | Missing evidence | Minimum verification | Blocking? |
|---|---|---|---|
| Receiver unplug/offline recovery | Physical receiver power interruption | Attended timeout test | no |
| Exhaustive input map | Every button and all four axis extremes | Extended attended input sweep | no |
| Current publication artifact on hardware | `42a4b35` image was not reflashed | Optional attended reflash and focused input replay | no for software publication |

Decision: publication software evidence complete; scoped physical evidence is
retained only for its recorded precursor artifact
