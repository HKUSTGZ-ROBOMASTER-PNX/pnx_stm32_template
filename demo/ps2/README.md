# PS2 遥控器实机测试 Demo（F407 minimal）

本 demo 用于验证 `pnx_f4_minimal` 的 F407 PS2 UART 接收器、协议解析以及统一 remoter 服务。
测试结果通过调试器中的 `demo_debug_instance.ps2_unit` 观察，不依赖串口打印。

## 启用方式

使用专用 preset：

```bash
cmake --preset f407-ps2-debug
cmake --build --preset f407-ps2-debug
```

该 preset 使用 [configs/boards/dji_c_board_f407/params.ps2.json](configs/boards/dji_c_board_f407/params.ps2.json)，当前默认：

- `bindings.remoter_uart = usart1`
- `remoter.source = ps2`
- `test.report_uart = usart6`

DJI C-board 外壳标注的 `UART2` 对应 MCU `USART1`，使用 `PA9 TX / PB7 RX`；
外壳标注的 `UART1` 对应 MCU `USART6`，使用 `PG14 TX / PG9 RX`。因此本
validation 把 PS2 接收器接到实体 `UART2`，而 report port 保留在实体
`UART1`。

如需改接别的 UART，请同时保证：

1. 该 UART 在 `board.ioc` 中存在；
2. 该 UART 具有 RX DMA；
3. 不与当前 image 的 `test.report_uart` 冲突；
4. 不与 DR16 复用同一 UART。

## 重要限制

- 这是 **单一来源 PS2 validation closure**，不是 DR16↔PS2 自动 fallback 验证。
- PS2 驱动启动时会把绑定 UART 重设为 **9600 / 8N1 / DMA RX**。
- 因此若某 UART 原本用于 DR16（例如 100000、特殊帧格式），不能同时给 PS2 共用。

## 调试字段

连接调试器后观察 `demo_debug_instance.ps2_unit`：

- `link`：`0` 已连接，`1` 接收器在线但手柄未连接，`2` 接收器离线。
- `buttons`：当前 16 位按键位图；`square`、`cross` 等布尔字段使用官方
  `remoter::ps2_button` 位序展开。
- `pressed` / `released`：最近一帧的边沿；应用层应以 `event_count`
  的变化作为一次性消费条件。
- `last_pressed` / `last_released`：最近一次非零边沿。
- `mapping_match`：raw PS2 状态与统一 remoter 中的链路、按键、边沿和
  event counter 一致。
- `frame_count` / `signal_count`：正常帧数量，以及正常帧加 `0xAB`
  心跳的总信号数量。
- `raw_left_x` ～ `raw_right_y`：四轴原始值。
- `left_x` ～ `right_y`：统一 remoter 输出，已归一化并应用死区。
- `connected_frame_count`：收到的正常帧数量。
- `remote_disconnected_count` / `receiver_offline_count`：链路状态切换次数。
- `passed`：已收到正常帧、统一 remoter 来源为 PS2 且当前在线。

## 建议验证步骤

1. 接收器上电但手柄未连接，确认 `link == 1`。
2. 打开手柄，确认 `link == 0`、`connected_frame_count` 持续增加。
3. 逐个按键，核对 `buttons`、`pressed`、`released`。
4. 分别将四个摇杆推到极限，核对原始范围和归一化方向。
5. 关闭手柄，确认返回 `link == 1`。
6. 断开接收器供电或 UART，超过超时后确认 `link == 2`。

## 已验证实机结果

以下结果对应 `f3a3274` lineage 的 precursor ELF
`F7BE6A16A009118715E53A936762D8F9E6E386604D7A98BA182C2B5B92F436A0`。
当前发布 pin `pnx_modules@42a4b35` 已完成 software closure，但未重新上板。

2026-08-06 在 DJI C-board STM32F407 上使用实体 `UART2` 完成 attended
validation。接收器未配对时持续收到 `0xAB`；配对后 `link == 0`，正常帧
持续增长，raw 与 unified remoter 映射一致。Cross 按住时观察到
`buttons == 0x4000`、`cross == true`，左摇杆极限观察到归一化
`left_y == 1.0`。最终 `passed == true`、`failure_mask == 0`。
