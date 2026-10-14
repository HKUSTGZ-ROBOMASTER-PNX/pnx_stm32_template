#pragma once

// Configuration types for peripherals enabled in h723_mc02.ioc.

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
    spi2,
    spi6
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
    h
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
    tim3,
    tim12
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

namespace adc
{
enum class handle : uint8_t
{
    none,
    adc1
};

enum class sampling_time : uint8_t
{
    cycles_1_5,
    cycles_2_5,
    cycles_8_5,
    cycles_16_5,
    cycles_32_5,
    cycles_64_5,
    cycles_387_5,
    cycles_810_5
};

struct config
{
    uint8_t channel = 0;
    uint8_t rank = 0;
    sampling_time sampling = sampling_time::cycles_1_5;
    handle instance = handle::none;
};
} // namespace adc

namespace usart
{
enum class handle : uint8_t
{
    none,
    uart5,
    uart7,
    usart1,
    usart10
};

struct config
{
    handle instance = handle::none;
    dma_config dma{};
};
} // namespace usart

} // namespace bsp
