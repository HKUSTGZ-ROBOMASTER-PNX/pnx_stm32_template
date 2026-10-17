# 设备使用指南

本目录提供 BMI088、DMIMU、LED、DJI/DM/LK 电机、DR16/VT03/PS2 遥控器及裁判系统 UI。
应用负责初始化顺序、调用周期、收发调度和错误处理，设备不会自动创建任务。

## 接入工程

根工程已链接 `app_config`，可以直接包含设备头文件和 `user_config.hpp`。
其他应用 target 使用用户配置时链接 `app_config`，只使用设备接口时链接 `devices`：

```cmake
target_link_libraries(my_app PRIVATE app_config)
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
| `boards.<板名>.ps2_cmd / ps2_data / ps2_clk / ps2_cs` | PS2 GPIO 资源 |
| `boards.<板名>.ps2_spi` | 可选 PS2 硬件 SPI |
| `ui.timeout_ms` | UI 发送超时，毫秒 |
| `ps2` | GPIO 时序、SPI 超时及摇杆死区 |
| `boards.<板名>.dmimu_can` | DMIMU CAN 资源，默认 H723 FDCAN1 / F407 CAN1 |
| `dmimu` | 设备/反馈 ID、工作模式、组装窗口和掉线超时（毫秒） |

资源名称引用对应板卡 `config.hpp` 中已有的资源，`none` 表示未配置。
新增引脚需先配置 CubeMX，再在板级配置中导出资源。板载 BMI088 和 LED 接线随选板切换，
不在 JSON 中重复填写。

CMake 配置和构建会检测 JSON 变化并生成头文件。只更新头文件、不编译固件时运行：

```sh
cmake -P configs/scripts/generate_config.cmake
```

生成头包含两块板的配置分支，随仓库保存，首次打开无需先构建。
配置只描述资源，不会自动启动采样、加热或发送。

## BMI088

用板级 SPI、片选和加热 PWM 构造实例，`configure()` 成功后再读取：

```cpp
#include "bmi088.hpp"

imu::bmi088 imu_device{{board::bmi088::spi, board::bmi088::acc_cs,
    board::bmi088::gyro_cs, board::bmi088::heater_timer, board::bmi088::heater_channel}};

pnxtypes::status init_imu()
{
    return imu_device.configure();
}

pnxtypes::status read_imu(math::vector3& accel, math::vector3& gyro)
{
    auto result = imu_device.read_acc(&accel);
    if (result != pnxtypes::status::ok)
    {
        return result;
    }
    return imu_device.read_gyro(&gyro);
}
```

加速度单位为 m/s²，角速度为 rad/s，温度为 °C。读数写入调用方变量；上述两次读取不是
原子采样，陀螺仪读取失败时加速度可能已更新。

`configure()` 自动初始化 DWT，寄存器配置及复位等待使用 DWT 忙等。
校准采用逐次累计，函数本身不读硬件、不延时，也不关闭加热：

```cpp
imu::bmi088::calibration progress{};
// 每个采样周期读取成功后调用一次，输入为 read_gyro/read_acc 的结果。
auto result = imu_device.calibrate(progress, gyro, accel);
```

`busy` 表示继续累计，`ok` 表示收满 4000 次有效样本并更新零偏，`error` 表示运动或
统计量不符合静止条件。完成或失败后结果保持不变，令 `progress = {}` 开始下一次校准；
重新配置设备或采集失败后也应丢弃未完成的累计。失败不会修改原零偏。
累计期间不要更改 `gyro_offset`，输入不得再做安装方向或向心补偿；函数会还原原始角速度
以估计完整零偏。调用方负责采样节奏，不能重复提交同一次读取的数据。
使用现有 AHRS 时每个采样周期累计一次，并照常温控、发布进度和休眠。

初始化与 SPI 读取参考 Bosch BMI08x SensorAPI v1.9.0 的
[`bmi08a.c`](https://github.com/boschsensortec/BMI08x_SensorAPI/blob/master/bmi08a.c)、
[`bmi08g.c`](https://github.com/boschsensortec/BMI08x_SensorAPI/blob/master/bmi08g.c) 和
[`bmi08_defs.h`](https://github.com/boschsensortec/BMI08x_SensorAPI/blob/master/bmi08_defs.h)：
加速度计先 dummy read 再检查 ID，软复位等待 1 ms 后重新 dummy read；陀螺仪复位等待
30 ms。加速度计两次电源配置各等待 5 ms，测量配置等待 40 ms；陀螺仪测量配置等待
10 ms，电源配置等待 30 ms。普通测量寄存器写入另留 2 µs 间隔。
底层读取统一丢弃加速度计的 SPI dummy byte，调用方长度只计有效数据；配置寄存器按掩码
修改并读回校验。仍保留原有上电等待 10 ms、轮询模式、量程、输出单位和温控策略，
没有引入官方驱动的 FIFO、同步采样或配置固件上传功能。

启用温控前按实际硬件设置 `thermal` 和 `temperature_pid`，默认参数对应 H723，
切到 F407 不会自动适配温控参数。应用每 125 ms 调用 `read_temperature(&temp)`，
成功后将新温度传给 `temperature_control(temp)`；温度读取失败会停止加热。
停止采样线程或销毁设备前调用 `shutdown()` 并检查结果。

初始化、校准、读取和温控由同一线程独占执行，SPI 总线也不能同时由其他任务访问。
驱动没有后台线程或任务停滞时的自动温控看门狗，偏置应按实际传感器校准。

## DMIMU

DMIMU 直接提供传感器数据和内部解算的姿态，不再经过软件 AHRS。
在 JSON 中选择 CAN、`can_id`（命令目标）、`master_id`（反馈 ID），并设置 `mode` 为
`active` 或 `request`。默认组装窗口 20 ms、掉线超时 100 ms，须按设备发送周期调整。
设备必须输出加速度、角速度、欧拉角和四元数四类帧才能形成完整结果。

```cpp
#include "user_config.hpp"

imu::dmimu attitude_sensor;

pnxtypes::status configure_dmimu()
{
    return attitude_sensor.configure(user_config::dmimu);
}

// 应用线程从接收队列取出帧后调用，received_ms 为中断接收时刻。
pnxtypes::status process_dmimu(bsp::can::handle bus, const bsp::can::rx_frame& frame,
    uint32_t received_ms)
{
    return attitude_sensor.receive(bus, frame, received_ms);
}

bool read_attitude(imu::dmimu::sample& sample, uint32_t now_ms)
{
    return attitude_sensor.take_sample(sample, now_ms);
}
```

`configure()` 只配置本地解析器，不初始化 CAN、不发送命令、不改设备持久参数。
应用先注册 CAN 回调，再调用 `bsp::can::init(user_config::dmimu_can)`；共享总线只初始化一次。
使用经典 CAN 标准数据帧，即使硬件是 FDCAN，也不要为 DMIMU 选择 FD 帧模式。
回调只把帧和接收时间复制到应用队列，线程按接收顺序调用 `receive()`。
队列溢出后在线程中调用 `clear()`，丢弃旧队列数据，再恢复组装。
停止接收或销毁回调对象前，按 BSP 要求停止总线并解除注册；不要销毁仍被中断使用的对象。

所有时间参数使用同一单调毫秒时钟，不直接传入非 1 kHz 的 ThreadX tick。
四类帧收齐后 `take_sample()` 返回一次最新结果，多组未取数据会覆盖旧结果。
超时、重复分量会重新开始组装；无新结果或结果已过期时返回 false，输出参数不变。
`online(now_ms)` 只依据完整结果的新鲜度，命令应答和零散数据不会使设备上线。
调用 `clear()` 后结果立即无效，软件序号继续递增；默认构造的四元数不代表已经收到有效姿态。

`sample` 的加速度为 m/s²、角速度和欧拉角为 rad/s 与 rad，四元数使用 WXYZ 顺序。
数据保留设备坐标及旋转约定，不自动归一化四元数或补偿安装方向。替换 BMI088 + 软件 AHRS
时须先对齐坐标系、四元数方向和安装变换。`received_ms` 是组内最早帧的接收时刻，
`sequence` 是本地发布计数；协议没有采样序号，组装窗口只能限制时间跨度，不能保证严格同步。

主动模式由设备自行发送。请求模式由应用依次调用
`read_register(imu::dmimu::register_id::accel_data/gyro_data/euler_data/quaternion_data)`，
分别发送四条请求，保留各条请求的进度；发送返回 `busy` 时稍后重试该条，不能每次从头重发。
选择 `request` 只描述应用工作方式，不自动切换设备模式。切换可显式调用
`write_register(register_id::change_active, 0)`（请求）或传 `1`（主动）。
其余校准、清零、保存等命令使用 `register_id` 中对应项，通过 `read_register/write_register` 发送。

命令返回值仅表示 CAN 提交结果。用 `take_reply(reply)` 获取设备应答，检查寄存器和 ACK，
应用负责等待超时及命令串行化。应答槽只保留最近一条；协议没有事务序号，超时后的迟到应答
不能作为新命令成功的可靠依据。修改设备 ID、重启或恢复设置后清空接收队列，并重新配置本地实例。

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

H723 默认使用 CMD PE13、DATA PE9、CLK PA2、CS PA0（低有效），F407 默认未配置。
使用前在 CubeMX 配置 CMD/CLK/CS 为输出、DATA 为输入，再在 JSON 选择资源。

```cpp
#include "user_config.hpp"

remotes::ps2::gpio gamepad{user_config::ps2_cmd, user_config::ps2_data,
    user_config::ps2_clk, user_config::ps2_cs, user_config::ps2_timing};

pnxtypes::status init_gamepad()
{
    return gamepad.init();
}

pnxtypes::status read_gamepad(remotes::ps2::sample& sample)
{
    remotes::ps2::frame raw;
    return remotes::ps2::poll(gamepad, sample, raw, user_config::ps2_deadzone);
}
```

初始化成功后在线程中周期读取，`sample` 提供按键位图和归一化摇杆值。
手柄须支持模拟模式，驱动不自动切换模式。硬件 SPI 可将传输对象替换为
`remotes::ps2::spi{user_config::ps2_spi, user_config::ps2_cs, user_config::ps2_spi_timeout_ms}`，
使用同样的 `init/poll` 接口。两板默认均未配置该 SPI，使用前设置为低位先行、时钟空闲高、
第二边沿采样，并满足手柄时序，不能直接复用 BMI088 或 WS2812 的现有总线配置。

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
