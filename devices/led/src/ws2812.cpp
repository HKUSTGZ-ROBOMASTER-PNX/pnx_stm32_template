#include "led.hpp"
#include "config.hpp"
#include "bsp_spi.hpp"

namespace led
{
pnxtypes::status set_rgb(bool red, bool green, bool blue)
{
    // Preserve penix's SPI symbols and GRB/MSB-first encoding. Their timing depends
    // on the board SPI6 clock/prescaler; these are not portable to arbitrary clocks.
    constexpr uint8_t low_symbol = 0x60;
    constexpr uint8_t high_symbol = 0x78;
    constexpr uint8_t reset_bytes[100]{};
    const uint8_t channels[]{green ? brightness : uint8_t{0}, red ? brightness : uint8_t{0},
                             blue ? brightness : uint8_t{0}};
    uint8_t payload[24]{};
    for (uint8_t channel = 0; channel < 3; ++channel)
    {
        for (uint8_t bit = 0; bit < 8; ++bit)
        {
            payload[channel * 8 + bit] = (channels[channel] & (0x80U >> bit)) != 0
                                            ? high_symbol
                                            : low_symbol;
        }
    }

    auto result = bsp::spi::wait_ready(board::led::spi, spi_timeout_ms);
    if (result != pnxtypes::status::ok)
    {
        return result;
    }
    result = bsp::spi::transmit(board::led::spi, payload, sizeof(payload), spi_timeout_ms);
    if (result != pnxtypes::status::ok)
    {
        return result;
    }
    return bsp::spi::transmit(board::led::spi, reset_bytes, sizeof(reset_bytes), spi_timeout_ms);
}
} // namespace led
