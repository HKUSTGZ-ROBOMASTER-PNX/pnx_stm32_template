#include "led.hpp"
#include "config.hpp"
#include "bsp_gpio.hpp"

namespace led
{
pnxtypes::status set_rgb(bool red, bool green, bool blue)
{
    auto result = bsp::gpio::set_active(board::led::red, red);
    if (result != pnxtypes::status::ok)
    {
        return result;
    }
    result = bsp::gpio::set_active(board::led::green, green);
    if (result != pnxtypes::status::ok)
    {
        return result;
    }
    return bsp::gpio::set_active(board::led::blue, blue);
}
} // namespace led
