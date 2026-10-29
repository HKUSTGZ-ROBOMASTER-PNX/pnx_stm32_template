#pragma once

// Configuration types for peripherals enabled in f407_cboard.ioc; ADC is not enabled.

#include <cstdint>

namespace bsp
{

enum class active_level : uint8_t
{
    low,
    high
};

struct dma_config
{
    bool rx = false;
    bool tx = false;
};

namespace spi
{
enum class handle : uint8_t
{
    none,
    spi1
};

struct config
{
    handle instance = handle::none;
    dma_config dma{};
    bool irq = false;
};
} // namespace spi

namespace gpio
{
enum class port : uint8_t
{
    none,
    a,
    b,
    c,
    d,
    e,
    f,
    g,
    h,
    i
};

struct config
{
    port instance = port::none;
    uint8_t pin = 0; // Pin number 0..15, not a HAL bit mask.
    active_level active = active_level::high;
};
} // namespace gpio

namespace pwm
{
enum class handle : uint8_t
{
    none,
    tim1,
    tim4,
    tim8,
    tim10
};
enum class channel : uint8_t
{
    none,
    ch1,
    ch2,
    ch3,
    ch4
};

struct config
{
    uint32_t timer_clock_hz = 0; // Kernel clock before the timer prescaler.
    handle instance = handle::none;
    uint8_t channel_mask = 0; // Bits 0..3 describe enabled CH1..CH4.
};
} // namespace pwm

namespace usart
{
enum class handle : uint8_t
{
    none,
    usart1,
    usart2,
    usart3,
    usart6
};

struct config
{
    handle instance = handle::none;
    dma_config dma{};
};
} // namespace usart

} // namespace bsp

// Resource values mirror the board IOC; these declarations do not initialize hardware.
namespace bsp::peripherals
{
// DMA flags are {rx, tx}; SPI1_IRQn is not enabled in the IOC.
inline constexpr spi::config spi1{spi::handle::spi1, {true, true}, false};
inline constexpr usart::config usart1{usart::handle::usart1, {true, true}};
inline constexpr usart::config usart2{usart::handle::usart2, {true, true}};
inline constexpr usart::config usart3{usart::handle::usart3, {true, false}};
inline constexpr usart::config usart6{usart::handle::usart6, {true, true}};

// Timer kernel clocks before prescaling, not PWM output frequencies.
// APB2: RCC.APB2TimFreq_Value; APB1: RCC.APB1TimFreq_Value.
inline constexpr pwm::config tim1{168000000U, pwm::handle::tim1, 0x0FU};   // CH1/2/3/4
inline constexpr pwm::config tim4{84000000U, pwm::handle::tim4, 0x04U};    // CH3
inline constexpr pwm::config tim8{168000000U, pwm::handle::tim8, 0x07U};   // CH1/2/3
inline constexpr pwm::config tim10{168000000U, pwm::handle::tim10, 0x01U}; // CH1

// GPIO locations only. Initial levels do not determine logical active levels;
// active retains the type default (high) and must be chosen by the consumer.
inline constexpr gpio::config pc8{gpio::port::c, 8};
inline constexpr gpio::config pg6{gpio::port::g, 6};

inline constexpr spi::config spi_configs[]{spi1};
inline constexpr usart::config usart_configs[]{usart1, usart2, usart3, usart6};
inline constexpr pwm::config pwm_configs[]{tim1, tim4, tim8, tim10};
} // namespace bsp::peripherals

// Onboard wiring follows penix/configs/boards/f407_cboard/board.json.
namespace board
{
namespace bmi088
{
inline constexpr auto spi = bsp::spi::handle::spi1;
inline constexpr bsp::gpio::config acc_cs{bsp::gpio::port::a, 4, bsp::active_level::low};
inline constexpr bsp::gpio::config gyro_cs{bsp::gpio::port::b, 0, bsp::active_level::low};
// PC4 (accelerometer DRDY) is not configured in this IOC; no usable pin is exported.
// Reference wiring uses active-high DRDY; the current IOC has falling-edge EXTI.
// Configure the sensor/EXTI consistently before enabling interrupt-driven sampling.
inline constexpr bsp::gpio::config gyro_drdy{bsp::gpio::port::c, 5, bsp::active_level::high};
inline constexpr auto heater_timer = bsp::pwm::handle::tim10;
inline constexpr auto heater_channel = bsp::pwm::channel::ch1;
} // namespace bmi088

namespace led
{
inline constexpr bsp::gpio::config red{bsp::gpio::port::h, 12, bsp::active_level::high};
inline constexpr bsp::gpio::config green{bsp::gpio::port::h, 11, bsp::active_level::high};
inline constexpr bsp::gpio::config blue{bsp::gpio::port::h, 10, bsp::active_level::high};
} // namespace led
} // namespace board
