# PnX DJI C-board STM32F407

這個 repository 是 DJI C-board（STM32F407）的最小 firmware 基礎模板。
它負責 F407 board support、共用 submodule 整合、安全 Core image、可選
USB CDC image，以及隔離的 PWM/A2 與 PS2 驗證 image；正式 chassis、gimbal、
shooter 等車組 application 應放在各自 repository。

本 repository 只實作 F407；不在這裡加入、修改或驗收 H7 source。
F407 與 H7 的 BSP 私有實作及發布流程彼此獨立；兩者的 public API 形式
協調屬於另一個跨 repository 任務，不是本 RC 的發布依賴。

## 術語與核心規則

- **BSP contract（板級支援契約）**：`pnx_bsp/*/include` 中不含 MCU/HAL
  型別的公共 API。Application、Device 與 Module 只依賴這些 header。
- **Backend（後端／板級實作）**：舊結構中指 public BSP function 之後的
  另一組 `detail::backend_*` 轉發函式。本 repository 已移除這種無額外
  語意的獨立轉發層；F407 source 直接定義 public `bsp::*` symbols。
- **Closure（功能閉包）**：一個 firmware image 實際選入的 application、
  BSP implementation 與必要依賴 source。未選取的 closure 不得進入該
  image 的 compile/link graph。
- **Fail-closed（未確認即停用）**：必要設定或授權未確認時，功能拒絕
  啟動，不使用猜測值繼續執行。USB identity 即採此策略。

固定邊界如下：

```text
Application / Device / Module
              │
              ▼
      public pnx_bsp header
              │
              ▼
       pnx_bsp/*/src/bsp_*.cpp
              │
              ▼
       STM32F4 HAL / CMSIS
```

每個 firmware target 只連結一份 public symbol implementation。沒有
runtime registry、factory、service locator、function-pointer table，亦
沒有用 runtime `if (F407/H7)` 選 MCU。

## 目前可建置範圍

| Image | Preset | 目前本地軟體驗證 | 用途 |
| --- | --- | --- | --- |
| Core Debug | `f407-debug` | RC2 local PASS，RAM 49,792 B／Flash 23,736 B | 日常安全起點 |
| Core Release | `f407-release` | RC2 local PASS，RAM 49,792 B／Flash 15,188 B | 最小 release build |
| USB CDC Debug | `f407-usb-cdc-debug` | RC2 local PASS，RAM 66,040 B／Flash 67,952 B | 可選 USB CDC |
| USB CDC Release | `f407-usb-cdc-release` | RC2 local PASS，RAM 66,000 B／Flash 40,056 B | 可選 USB CDC |
| PWM/A2 Debug | `f407-pwm-a2-debug` | RC2 local PASS，RAM 49,864 B／Flash 27,656 B | attended hardware validation，不是產品 composition |
| PS2 Debug | `f407-ps2-debug` | software PASS，RAM 55,456 B／Flash 50,856 B；precursor artifact hardware PASS | USART1 RX-DMA PS2 remoter validation，不是產品 composition |

原有五個 image 數字與 `5f222b68530b135fb46bc3c12de8127ef5fff6f0`
一致；PS2 數字已在 `pnx_modules@42a4b35` 上 fresh configure/build/link。
F407 Direct BSP 仍 pin 已發布的
`da09febe8f5bfe66993010247d4d6731d0ca492b`。原有五個 preset 與新增的
PS2 preset 均為 0
compiler/linker warnings、恰有一個
`app_start`，且沒有重複的 strong `bsp::*` definition。

BMI088、DBUS RX 與 CAN/M2006 validation closure 已撤下。它們先前依賴
非 template 的 Device/Lib/Module API；在三個 submodule 回到 template
gitlinks 後，本 repository 不再提供其 preset、demo、CI build 或目前
支援宣稱。若仍傳入下列舊選項，CMake 會明確拒絕：

```text
PNX_ENABLE_BMI088
PNX_ENABLE_DBUS_RX
PNX_ENABLE_CAN_M2006_VALIDATION
```

CAN、SPI 與 USART 的 public contract 與 F407 direct source 仍存在，供
之後的 application repository 使用；它們目前沒有被連結成 public BSP
Core closure，也沒有在此 repository 宣稱新的硬體 validation。Flash
source 也保留，但在 linker 保留資料 partition 前不屬於任何受支持 image。
Host fake 測試驗證的是公共 API contract，不等於 F407 HAL runtime 或
硬體通過。

## 第一次使用

需求：CMake 3.22+、Ninja、GNU Arm Embedded Toolchain
（`arm-none-eabi-gcc`、`arm-none-eabi-g++`）。

取得 parent 與固定 submodules：

```powershell
git clone --branch f407-architecture-rc.2 --recurse-submodules `
  https://github.com/HKUSTGZ-ROBOMASTER-PNX/pnx_template.git pnx_f4_minimal
cd pnx_f4_minimal
```

先建置 Core：

```powershell
cmake --preset f407-debug
cmake --build --preset f407-debug
```

若已取得燒錄授權，可用 CMSIS-DAP：

```powershell
openocd -f interface/cmsis-dap.cfg -f target/stm32f4x.cfg `
  -c "adapter speed 1000" `
  -c "program build/f407-debug/pnx_embedded.elf verify reset exit"
```

Core 的既有 attended smoke 標準是 PH11 綠燈約每 500 ms 切換一次，
代表 startup、`main`、ThreadX composition 與 Core thread 已執行。Core
不會啟動 USB、PWM、CAN BSP service、SPI/BMI088、DBUS 或 Flash 操作。
歷史硬體證據與限制見 [HANDOFF.md](HANDOFF.md)；本次架構整理沒有操作
硬體。

執行 native host tests：

```powershell
cmake -S . -B build/host -G Ninja -DPNX_HOST_TESTS=ON
cmake --build build/host
ctest --test-dir build/host --output-on-failure
```

`tests/host` 必須由 repository root configure，不是獨立 CMake project。
目前完整 suite 為 **37/37 PASS**，其中
`direct_bsp_source_layout` 保證 F407 Direct BSP implementation 位於
`pnx_bsp/*/src`，且 parent Board tree 不再保存另一份 public symbol
implementation。

## 高階目錄

```text
pnx_f4_minimal/
├─ CMakeLists.txt / CMakePresets.json
│  └─ host/embedded 分流與 compile-time image composition
├─ cmake/
│  └─ GNU Arm toolchain
├─ configs/
│  ├─ boards/dji_c_board_f407/
│  └─ cmake/
│     └─ IOC-derived config 與 build-local header generation
├─ demo/
│  ├─ app.cpp
│  └─ cboard/
│     ├─ board_smoke/
│     ├─ usb_cdc/
│     ├─ pwm_a2/
│     └─ ps2_validation/
├─ pnx_bsp/
│  ├─ */include/  MCU-neutral public BSP contracts
│  └─ */src/      F407 direct implementations selected by CMake
├─ pnx_devices/ / pnx_libs/ / pnx_modules/
│  └─ template-pinned reusable layers
├─ boards/dji_c_board_f407/
│  ├─ Core/ / AZURE_RTOS/ / USBX/
│  ├─ Drivers/ / Middlewares/
│  └─ startup_stm32f407xx.s / STM32F407XX_FLASH.ld
└─ tests/host/
   └─ direct fake implementations與 public contract tests
```

Device 與 Module 可以知道 `bsp::can`、`bsp::spi`、`bsp::usart` 等公共
contract，但不可知道 `hcan1`、`huart3`、GPIO pin、DMA stream、IRQ 名稱
或 `HAL_*`。這些硬體細節只能出現在 F4 branch 的 `pnx_bsp/*/src` 或
Board generated/platform files。

為了讓未分支的 template Device/Module 直接編譯，公共 contract 保留
下列相容名稱：

- CAN 的 `fdcan1`/`fdcan2` 是 F407 `can1`/`can2` 的邏輯別名；
- SPI 的 `spi2` 對應 F407 IMU 邏輯 slot（board 內實際是 SPI1），
  `spi6` 在本板未配置；
- PWM 的 `tim3_ch4`/`tim12_ch2` 可通過編譯，但在 C-board 映射為
  unsupported slot，操作會 fail-closed，不會誤驅動 TIM1_CH2/PE11。

這些只是 template consumer API 相容識別字，不是 H7 source、H7 HAL
支援或硬體驗證宣稱。

## CMake flow

```text
root CMakeLists.txt
├─ PNX_HOST_TESTS=ON
│  ├─ native compiler
│  ├─ tests/host/fake_*.cpp 直接定義 public bsp::* symbols
│  └─ CTest
└─ embedded build
   ├─ 讀取 F407 IOC + params.json + robot.json
   ├─ 產生 build/<preset>/generated/config.hpp
   ├─ 加入共同 Core source
   ├─ compile-time 選擇
   │  ├─ no optional selector → board_smoke + Core BSP
   │  ├─ PNX_ENABLE_USB_CDC → bsp_usb.cpp + usb_cdc
   │  └─ PNX_ENABLE_PWM_A2 → bsp_pwm.cpp + pwm_a2
   ├─ 由實際 source list 推導 generated peripheral init guards
   ├─ add_executable(pnx_embedded)
   └─ board CMake 連結 CubeMX / HAL / ThreadX
```

`PNX_ENABLE_USB_CDC`、`PNX_ENABLE_PWM_A2` 與 `PNX_ENABLE_PS2_VALIDATION`
互斥。這些 selector 是 isolated image composition，不應演變成
chassis/gimbal/shooter 等產品子系統選單。正式 robot application 應在
另一個 repository 組合所需 Device 與 Module。

CubeMX 仍可能把 `can.c`、`usart.c` 等 generated capability source 放進
Core build；這不代表 `bsp_can.cpp` 或 `bsp_usart.cpp` 已啟動或連結。
root CMake 以同一份 `PNX_EMBEDDED_SOURCES` 判斷 Direct BSP source 是否
實際被選入，並據此 guard `main.c` 的 CAN／USART generated init call。
目前 Core / USB / PWM 這些 RC2 image 都沒有選入這兩個 source，因此不
執行 CAN1、CAN2 或 USART1/3/6 初始化；PS2 validation branch 另外新增了
per-UART init guard，讓帶入 `bsp_usart.cpp` 的 image 只啟動實際綁定的 UART。

## Configuration authority

- `boards/dji_c_board_f407/dji_c_board_f407.ioc`：clock、generated
  CAN/UART/USB capability、一般 GPIO 與開機安全狀態。
- `configs/boards/dji_c_board_f407/*.json`：board policy 與 public
  contract binding；輸出只寫到 build-local generated headers。
- `pnx_bsp/*/src/`：F407 direct implementation，持有 HAL handle、pin、
  IRQ、DMA 與 peripheral instance；不再增加一層 backend forwarding。

SPI1/BMI088 與 TIM1_CH2/PE11 目前是手寫 board resource；完整 ownership
與 CubeMX regeneration 規則見
[Board README](boards/dji_c_board_f407/README.md)。

## USB identity

`bsp::usb::init(config)` 採非阻塞、異步 startup。返回 `ok` 只代表 config
與 callback ownership 已接受、必要 ThreadX resources 已建立且 startup
worker 已安排；不代表已枚舉、host 已連接或 CDC ready。只有 CDC transport
實際可用時 `connected()` 才為 true。startup failure 進入可觀察的 fault
狀態；`write()` 在未 ready、斷線或 bounded queue 滿時明確返回非 `ok`，
不會無限等待或累積。

完全相同的 config 可重複初始化；不同 callback、user context 或不相容
參數會被拒絕，不會靜默改寫 ownership。F407 callback 的 thread context、
buffer lifetime 與 disconnect 行為見
[Board README](boards/dji_c_board_f407/README.md)。

USB closure 預設為 fail-closed：VID/PID 是 `0x0000`，serial 是
`UNASSIGNED`。只有取得團隊授權 identity 後，才可同時設定：

```text
PNX_USB_DEVICE_IDENTITY_CONFIRMED=ON
PNX_USB_DEVICE_VID=<authorized VID>
PNX_USB_DEVICE_PID=<authorized PID>
PNX_USB_DEVICE_MANUFACTURER=<authorized string>
PNX_USB_DEVICE_PRODUCT=<authorized string>
PNX_USB_DEVICE_SERIAL=<stable authorized serial>
```

## RC2 deferred capability matrix

| Capability | RC2 status | Required before use |
| --- | --- | --- |
| Flash | `UNSUPPORTED_UNTIL_RESERVED_PARTITION` | Linker 必須保留資料 partition；`layout()` 只暴露該區；erase/program 必須拒絕 firmware address。 |
| BMI088/AHRS | `NOT_IN_RC2_PRODUCT_GRAPH` | 保留 template Device source；正式使用前處理 SPI bootstrap、transaction ownership、EXTI/DRDY mapping 與 ThreadX concurrency；不建立 F407 Device fork。 |
| Public typed USB adapter | `UNSUPPORTED_UNTIL_CONTRACT_FIX` | RC2 只支持 raw config/API；正式使用 `make_config<T>()` 前處理 alignment、object lifetime、trivially-copyable 與 caller-owned context。 |

這三項是使用前條件，不是 Direct BSP 架構、Core、raw USB API 或 PWM
closure 的失敗。

## Submodule 與發布邊界

目前 submodule pins 如下；Device/Lib 保留既有 baseline，BSP 使用已發布的
F407-specific commit，而 Modules 使用從官方 `main@44ff9ab` 建立的已發布
PS2 stack 修正分支：

```text
pnx_bsp      da09febe8f5bfe66993010247d4d6731d0ca492b
pnx_devices  2349cc108c9ed477ccdcd700e802ea888975cdfd
pnx_libs     e7c3e7a2b9d825586ab3e0c413877180c4295df8
pnx_modules  42a4b35b63ef7b8528ad756e03d17f1c09bab9af
```

`pnx_modules/fix/f407-ps2-stack` 僅補 F407 實測所需的 PS2 1536 B stack；
沒有合併或直接修改 modules `main`。`pnx_bsp` 的 F407-specific branch
則同時保有 `*/include` public contract 與 `*/src` HAL direct
implementation。Parent repository 只 pin 可重現 commit、提供 CubeMX
generated handles，並由 CMake 選擇 image 所需 source。

目前選取的 `pnx_modules` revision 內含 PS2 receiver 與 Tactical AHRS source，但 RC2
image graph 不選入兩者。此 branch 另外加入 `f407-ps2-debug` validation
closure，使用獨立 params 與 demo 驗證 F407 上的單一來源 PS2 路徑；它不
宣稱 DR16↔PS2 自動 fallback 已可用。PS2 會把綁定 UART 重設為 9600 / 8N1 /
DMA RX，因此不能與 DR16 共享同一條 UART。C-board 外殼的 `UART2` 對應
MCU `USART1`（PA9/PB7），已完成接收器斷線碼、正常 frame、Cross 按鍵與
左搖桿實機驗證；外殼 `UART1` 對應 `USART6` 並保留為 report port。
Direct USART BSP 仍提供
board-neutral line configuration contract，讓之後的 application repository
不必恢復 backend。BMI088 source 仍需要正式的 F407 `GYRO_INT_Pin`／EXTI
mapping，因此 BMI088/AHRS 的 deferred status 不變。

Host suite 會直接編譯 template 的新 BMI088、LK9025、PID 與 PS2 public
API，並驗證 F407 config generation；SPI/PWM/USART contract tests 也會
檢查相容入口。Host 測試只證明 API 與 fail-closed software contract；
PS2 的實機證據另記錄於 `HANDOFF.md` 與 `demo/ps2/README.md`。

新的 F407 `pnx_bsp` SHA 已由官方 `F4_version_bsp` remote branch 提供；
PS2 modules SHA 亦由官方 `fix/f407-ps2-stack` remote branch 提供。
`F4_version` 發布後仍應從 remote recursive clone 重跑六個 presets、完整
Host suite、F407 source graph 與 symbol gates；全部通過後才能建立
annotated tag。H7 regression 不屬於此流程。
