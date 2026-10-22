# 设备使用指南

本目录提供 LED、DJI/DM/LK 电机、DR16/VT03/PS2 遥控器及裁判系统 UI。
应用负责初始化顺序、调用周期、收发调度和错误处理，设备不会自动创建任务。

## 接入工程

应用 target 链接 `devices` 后即可包含设备头文件，由调用方提供资源配置：

```cmake
target_link_libraries(my_app PRIVATE devices)
```

先完成 HAL/CubeMX 初始化，再在线程中初始化设备。同一设备及共享总线的访问须串行化。
多数接口返回 `pnxtypes::status`，成功后再使用输出；电机接口使用 `bool` 和 `motor::result`。

## 应用资源选择

修改 [`configs/user_config.json`](../configs/user_config.json)，不要直接编辑生成的
[`user_config.hpp`](../configs/include/user_config.hpp)。

| JSON 字段 | 用途 |
| --- | --- |
| `boards.<板名>.ui_uart` | UI 串口 |
| `boards.<板名>.dr16_uart / vt03_uart` | 遥控器串口 |
| `boards.<板名>.ps2_cs` | PS2 低有效 GPIO 片选 |
| `boards.<板名>.ps2_spi` | PS2 硬件 SPI，启用 PS2 时必需 |
| `ui.timeout_ms` | UI 发送超时，毫秒 |
| `ps2` | SPI 超时及摇杆死区 |

资源名称引用对应板卡 `config.hpp` 中已有的资源，`none` 表示未配置。
新增引脚需先配置 CubeMX，再在板级配置中导出资源。板载 LED 接线随选板切换，
不在 JSON 中重复填写。

CMake 配置和构建会检测 JSON 变化并生成头文件。只更新头文件、不编译固件时运行：

```sh
cmake -P configs/scripts/generate_config.cmake
```

生成头包含两块板的配置分支，随仓库保存，首次打开无需先构建。
配置只描述资源，不会自动启动接收或发送。

## LED

H723 使用板载 WS2812，F407 使用三路 GPIO LED，接口相同：

```cpp
#include "led.hpp"

pnxtypes::status show_ready()
{
    return led::on(led::color::green);
}
```

| 接口 | 行为 |
| --- | --- |
| `init()` / `all_on()` | 点亮全部颜色 |
| `all_off()` | 熄灭全部颜色 |
| `on(color)` | 替换整组颜色，支持 red、green、blue、white |
| `toggle(color)` | 推进该颜色的切换状态 |
| `blink(color)` | 默认每 250 次调用切换一次，首次为熄灭半周期 |

CubeMX 已初始化外设，`init()` 会亮灯，不是单纯注册设备。闪烁速度取决于调用周期，
`on/all_off/all_on/init` 不重置闪烁计数。GPIO LED 只有开关，WS2812 的亮度和超时由
`led.hpp` 常量控制。所有调用须串行；WS2812 使用阻塞 SPI，不在中断中调用。

## 电机

按型号包含头文件，通过对应 manager 组织接收、计算和发送：

| 系列 | 型号头文件 | manager | `register_motor()` 参数 |
| --- | --- | --- | --- |
| DJI | `m3508.hpp`、`m2006.hpp`、`gm6020.hpp` | `dji_manager.hpp` | 电机、总线号、反馈 ID |
| DM | `dm4310.hpp`、`dm8009p.hpp` | `dm_manager.hpp` | 电机、总线号、控制 ID、反馈 ID |
| LK | `lk8016.hpp`、`lk9025.hpp` | `lk_manager.hpp` | 电机、总线号、反馈 ID |

以 DJI 为例，应用实现发送回调，将逻辑总线号映射到实际 BSP CAN 资源：

```cpp
#include "m3508.hpp"
#include "dji_manager.hpp"

// 应用提供实现：成功提交发送返回 true，失败返回 false。
bool send_dji(void* context, uint8_t bus, const motor::dji::frame_t& frame);

motor::dji::m3508 wheel;
motor::dji::manager motors{send_dji};

bool init_motor()
{
    motors.set_period(0.001f); // update() 的实际调用周期，秒
    return motors.register_motor(wheel, 0, 0x201);
}

void control_tick(uint32_t now_us)
{
    wheel.motor_mode = motor::mode::current;
    wheel.cmd.current = 0;
    motors.update();
    motors.transmit(now_us);
}
```

初始化成功后周期调用 `control_tick()`。收到至少 8 字节的完整反馈后调用
`motors.receive(bus, feedback_id, data)`。接收与控制不能并发修改同一电机，可先复制反馈，
再交给控制线程处理。发送回调中的帧只在调用期间有效，异步发送前须复制到持久缓冲区。

逻辑总线号范围为 0～2，不等于 HAL 实例。manager 保存电机引用，电机须保持存活。
`set_tx_period()` 和 `transmit(now_us)` 使用微秒；`set_period()` 使用秒。
manager 的 `update()` 已包含健康检查，不要重复调用 `health_tick()`。

使用速度或位置控制前设置对应 PID，并用 `supports(mode)` 检查支持情况。
单独调用电机 `update()` 时检查 `motor::result`，manager 不向上传递该结果。
位置和速度使用 rad、rad/s；电流、电压命令的原始单位由型号协议决定。
DM 构造参数 `firmware_mode` 须与电机固件配置一致，特殊指令通过其 manager 的 `special()` 发送。
离线和堵转状态只用于诊断，不会自动停机，应用负责处理输出。

## 遥控器

### DR16 / VT03

默认 DR16 使用 H723 UART5 / F407 USART3，VT03 使用 H723 UART7 / F407 USART6。
先在 JSON 确认接线，再初始化串口：

```cpp
#include "remoter.hpp"
#include "user_config.hpp"

pnxtypes::status init_dr16_uart()
{
    auto result = bsp::usart::init(user_config::dr16_uart, bsp::usart::mode::dma);
    if (result != pnxtypes::status::ok)
    {
        return result;
    }
    return bsp::usart::configure(user_config::dr16_uart.instance, user_config::dr16_line);
}

pnxtypes::status decode_dr16(const uint8_t* frame, uint16_t size, remotes::reading& reading)
{
    return remotes::dr16::decode(frame, size, reading);
}
```

之后用 `bsp::usart::start_rx_to_idle()` 注册缓冲区和回调，约束见 [BSP 文档](../bsp/README.md)。
DR16 为 18 字节一帧、100000 baud、8E1；VT03 为 21 字节一帧、115200 baud、8N1，
使用对应的 `vt03_uart/vt03_line` 和 `remotes::vt03::decode()`。
DR16 的 HAL 9 位字长包含偶校验位。

RX 回调在中断中执行，向线程传递时先复制数据。一次 idle 回调可能包含半帧或多帧，
应用组出完整帧后再解码。错误帧不更新输出；摇杆为归一化值。
掉线超时、控制器选择及按键边沿由应用处理。

### PS2

H723 使用 SPI1：CMD PD7、DATA PB4、CLK PB3、CS PE15（低有效）；F407 默认未配置。

```cpp
static uint8_t tx[remotes::ps2::frame_size]{};
static uint8_t rx[remotes::ps2::frame_size]{};
remotes::ps2::spi gamepad{user_config::ps2_spi, user_config::ps2_cs, tx, rx};
```

调用 `init()` 后，用 `start()` 发起 IT 收发；`read(raw)` 返回 `ok` 后调用 `decode()`。
`busy` 时让出 CPU 等待；超时调用 `cancel()` 并检查结果，缓冲区保留到完成或成功取消。
SPI 使用模式 3、LSB first 和中断；H723 字节间隔为 10 个 SCK 周期，手柄使用模拟模式。

## 裁判系统 UI

在 JSON 选择 `ui_uart`，默认 USART1。初始化串口和 canvas，再设置实际发送方、接收方 ID：

```cpp
#include "user_config.hpp"

pnxtypes::status init_ui(uint16_t sender_id, uint16_t receiver_id)
{
    auto result = bsp::usart::init(user_config::ui_uart);
    if (result != pnxtypes::status::ok)
    {
        return result;
    }
    auto& canvas = ui::canvas::instance();
    result = canvas.init(user_config::ui);
    if (result != pnxtypes::status::ok)
    {
        return result;
    }
    canvas.set_sender_receiver_id(sender_id, receiver_id);
    const int8_t id = canvas.create_line(2, ui::object_color::green, 0, 100, 100, 300, 100);
    if (id < 0)
    {
        return pnxtypes::status::error;
    }
    return canvas.update();
}
```

波特率沿用 CubeMX 配置，需要修改时在发送前调用 `bsp::usart::configure()`。
`create_*()` 返回对象 ID，负值表示失败；保存 ID 后可用 `move_to/set_color/set_float` 等接口更新。
周期调用 `update()`，每次最多发送一帧，发送失败保留待更新内容。
`remove(id)` 标记单个对象删除，`remove_all/remove_layer` 返回发送状态。

`create_string/set_string` 借用字符串，内容需保持有效直到对象删除或被替换；
原地修改字符串后调用 `set_string_changed(id)`。同一 canvas 的调用须串行，
共享串口由应用统一初始化并协调发送，不能交错写入其他协议帧。
