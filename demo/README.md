# demo/

Application sources for this repository. Two directories with different status.

## `cboard/` — built

These are the selectable application closures. Exactly one is compiled into any
given image, chosen at configure time by the preset:

| Directory              | Selected by            | Status                    |
| ---------------------- | ---------------------- | ------------------------- |
| `cboard/board_smoke/`  | default (no option)    | product baseline          |
| `cboard/usb_cdc/`      | `PNX_ENABLE_USB_CDC`   | product, optional         |
| `cboard/pwm_a2/`       | `PNX_ENABLE_PWM_A2`    | validation closure only   |

`app.cpp` is the composition root and is always compiled. It contains a
preprocessor check that exactly one `PNX_APP_*` macro is defined, so a broken
selection fails to compile rather than producing a silently different image.

## `_reference/` — NOT built

Nothing in `_reference/` is in any build graph. No `CMakeLists.txt` names these
paths, and no built source includes them. They do not compile as-is: they call
`ahrs::`, `djimotors::`, `referee::` and `remoter::` APIs whose backing BSP
implementations are not part of any current F407 image.

They are kept because they are useful wiring examples, not because they are
pending work. Each one is a trimmed version of the corresponding
`pnx_template` demo, with the parts that do not apply to a DJI C-board F407
removed — for instance the tactical ESKF diagnostic fields, PS2 gamepad
configuration, and IMU temperature-control telemetry are gone from the F4 copies.
So they show the intended shape of an F407 peripheral application, not a
half-finished one.

| Directory              | Shows                                            |
| ---------------------- | ------------------------------------------------ |
| `_reference/imu/`      | AHRS service startup and BMI088 sampling loop    |
| `_reference/motor/`    | DJI motor setup over CAN                         |
| `_reference/remoter/`  | DR16 receiver wiring and offline detection       |
| `_reference/referee_ui/` | Referee-system serial protocol and UI drawing  |
| `_reference/usart/`    | Framed USART debug protocol                      |
| `_reference/common/`   | Shared debug-struct and packet helpers used by the above |
| `_reference/tools/`    | Host-side Python scripts for the USART demo      |

### Rules

- Do not add these paths to a `CMakeLists.txt` to "make them build again".
  Bringing a peripheral up means writing a new closure under `cboard/`, using
  the reference as a guide.
- When a peripheral does get a real closure, delete the corresponding
  `_reference/` directory in the same change. Two live examples of the same
  peripheral is worse than none.
- Do not refactor code in here. It is a snapshot for reading; edits give the
  false impression that something depends on it.
