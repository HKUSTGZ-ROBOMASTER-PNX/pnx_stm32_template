#pragma once
#include "bsp_gpio.hpp"
#include "bsp_spi.hpp"
#include "status.hpp"
#include <stdint.h>

namespace remotes::ps2
{
inline constexpr uint8_t frame_size = 9U;

struct frame
{
    uint8_t bytes[frame_size]{};
};

struct sample
{
    uint16_t buttons = 0; // Active-high PS2 button bitmap.
    float lx = 0.0f;
    float ly = 0.0f;
    float rx = 0.0f;
    float ry = 0.0f;
};

// Synchronous device operations; scheduling, button edges and publication belong to remoter.
pnxtypes::status decode(const frame& raw, sample& out, float deadzone = 0.02f) noexcept;

template <typename Transport>
pnxtypes::status poll(Transport& transport, sample& out, frame& raw, float deadzone = 0.02f)
{
    constexpr uint8_t query[frame_size]{0x01U, 0x42U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
        0x00U};
    raw = {};
    const auto status = transport.transfer(query, raw.bytes, frame_size);
    return status == pnxtypes::status::ok ? decode(raw, out, deadzone) : status;
}

// CubeMX configures an idle-high, second-edge, LSB-first bus before init().
// Serialize transfers and all other users of the bus; CS must be active-low.
class spi
{
public:
    constexpr spi(bsp::spi::handle spi_bus, bsp::gpio::config cs, uint32_t timeout_ms = 20) noexcept
        : spi_bus_(spi_bus), cs_(cs), timeout_ms_(timeout_ms)
    {
    }

    pnxtypes::status init();
    pnxtypes::status transfer(const uint8_t* tx, uint8_t* rx, uint8_t size);

private:
    bsp::spi::handle spi_bus_;
    bsp::gpio::config cs_;
    uint32_t timeout_ms_;
};

struct gpio_timing
{
    uint32_t half_period_us = 5;
    uint32_t cs_setup_us = 20;
    uint32_t cs_hold_us = 20;
};

// CubeMX configures CMD/CLK/CS outputs and DATA input; serialize calls from one thread.
class gpio
{
public:
    constexpr gpio(bsp::gpio::config cmd, bsp::gpio::config data, bsp::gpio::config clk,
        bsp::gpio::config cs, gpio_timing timing = {}) noexcept
        : cmd_(cmd), data_(data), clk_(clk), cs_(cs), timing_(timing)
    {
    }

    pnxtypes::status init();
    pnxtypes::status transfer(const uint8_t* tx, uint8_t* rx, uint8_t size);

private:
    pnxtypes::status transfer_byte(uint8_t tx, uint8_t& rx);

    bsp::gpio::config cmd_;
    bsp::gpio::config data_;
    bsp::gpio::config clk_;
    bsp::gpio::config cs_;
    gpio_timing timing_;
};

} // namespace remotes::ps2
