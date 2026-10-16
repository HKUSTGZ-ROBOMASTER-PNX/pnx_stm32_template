#pragma once

// Generated from configs/user_config.json. Do not edit.
#include "config.hpp"
#include "ui.hpp"
#include "ps2.hpp"

namespace user_config
{
#if defined(STM32H723xx)
inline constexpr auto ui_uart = bsp::peripherals::usart1;
inline constexpr auto dr16_uart = bsp::peripherals::uart5;
inline constexpr auto vt03_uart = bsp::peripherals::uart7;
inline constexpr auto ps2_cmd = bsp::peripherals::ps2_cmd;
inline constexpr auto ps2_data = bsp::peripherals::ps2_data;
inline constexpr auto ps2_clk = bsp::peripherals::ps2_clk;
inline constexpr bsp::gpio::config ps2_cs{bsp::peripherals::ps2_cs.instance,
    bsp::peripherals::ps2_cs.pin, bsp::active_level::low};
inline constexpr auto ps2_spi = bsp::spi::handle::none;
#elif defined(STM32F407xx)
inline constexpr auto ui_uart = bsp::peripherals::usart1;
inline constexpr auto dr16_uart = bsp::peripherals::usart3;
inline constexpr auto vt03_uart = bsp::peripherals::usart6;
inline constexpr bsp::gpio::config ps2_cmd{};
inline constexpr bsp::gpio::config ps2_data{};
inline constexpr bsp::gpio::config ps2_clk{};
inline constexpr bsp::gpio::config ps2_cs{bsp::gpio::port::none, 0, bsp::active_level::low};
inline constexpr auto ps2_spi = bsp::spi::handle::none;
#else
#error Unsupported board
#endif

inline constexpr ui::config ui{ui_uart.instance, 100};
inline constexpr bsp::usart::line_config dr16_line{100000, bsp::usart::word_length::bits_9,
    bsp::usart::stop_bits::one, bsp::usart::parity::even, false, true};
inline constexpr bsp::usart::line_config vt03_line{115200, bsp::usart::word_length::bits_8,
    bsp::usart::stop_bits::one, bsp::usart::parity::none, false, true};
inline constexpr remotes::ps2::gpio_timing ps2_timing{5, 20, 20};
inline constexpr float ps2_deadzone = 0.02f;
inline constexpr uint32_t ps2_spi_timeout_ms = 20;
} // namespace user_config
