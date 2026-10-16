#pragma once

#include <cstdint>
#include "status.hpp"

// One board LED group. Call after board initialization and serialize all calls,
// including other users of the WS2812 SPI bus. SPI operations are blocking.
namespace led
{
enum class color : uint8_t
{
    red,
    green,
    blue,
    white,
};

// GPIO LEDs are on/off only; brightness and timeout apply only to WS2812.
inline constexpr uint8_t brightness = 7;
inline constexpr uint32_t spi_timeout_ms = 1000;
inline constexpr uint16_t blink_half_period_calls = 250;
static_assert(blink_half_period_calls > 0 && blink_half_period_calls <= 32767);

// init lights all channels; CubeMX already initializes the peripherals.
pnxtypes::status init();
pnxtypes::status on(color selected);
pnxtypes::status all_off();
pnxtypes::status all_on();
// Independent toggle/blink sequences, advanced only after successful writes.
// blink starts with an off half-period, measured in calls, not RTOS ticks.
pnxtypes::status toggle(color selected);
pnxtypes::status blink(color selected);
} // namespace led
