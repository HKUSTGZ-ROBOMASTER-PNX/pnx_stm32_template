#include "led.hpp"

namespace led
{
using pnxtypes::status;

// Implemented by exactly one board-selected source file.
status set_rgb(bool red, bool green, bool blue);

status init()
{
    return all_on();
}

status all_on()
{
    return set_rgb(true, true, true);
}

status all_off()
{
    return set_rgb(false, false, false);
}

status on(color selected)
{
    switch (selected)
    {
        case color::red:
            return set_rgb(true, false, false);
        case color::green:
            return set_rgb(false, true, false);
        case color::blue:
            return set_rgb(false, false, true);
        case color::white:
            return all_on();
        default:
            return status::invalid_arg;
    }
}

status toggle(color selected)
{
    if (selected > color::white)
    {
        return status::invalid_arg;
    }
    static bool enabled = false;
    const auto result = enabled ? all_off() : on(selected);
    if (result == status::ok)
    {
        enabled = !enabled;
    }
    return result;
}

status blink(color selected)
{
    if (selected > color::white)
    {
        return status::invalid_arg;
    }
    static uint16_t count = 0;
    const auto result = count < blink_half_period_calls ? all_off() : on(selected);
    if (result == status::ok)
    {
        ++count;
        if (count == 2U * blink_half_period_calls)
        {
            count = 0;
        }
    }
    return result;
}
} // namespace led
