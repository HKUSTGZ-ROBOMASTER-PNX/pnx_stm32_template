#include "ps2.hpp"
#include <cmath>

namespace remotes::ps2
{
namespace
{
constexpr bool analog_id(uint8_t id)
{
    return id == 0x53U || id == 0x73U;
}

constexpr float normalize_axis(uint8_t raw, uint8_t center)
{
    if (raw >= center)
    {
        return static_cast<float>(raw - center) / static_cast<float>(UINT8_MAX - center);
    }
    return -static_cast<float>(center - raw) / static_cast<float>(center);
}

constexpr float apply_axis_deadzone(float value, float deadzone)
{
    return value > -deadzone && value < deadzone ? 0.0f : value;
}

} // namespace

pnxtypes::status decode(const frame& raw, sample& out, float deadzone) noexcept
{
    if (!std::isfinite(deadzone) || deadzone < 0.0f || deadzone >= 1.0f)
    {
        return pnxtypes::status::invalid_arg;
    }
    if (raw.bytes[0] != 0xFFU || !analog_id(raw.bytes[1]) || raw.bytes[2] != 0x5AU)
    {
        return pnxtypes::status::error;
    }

    sample decoded{};
    decoded.buttons = static_cast<uint16_t>(
        ~(static_cast<uint16_t>(raw.bytes[3]) | (static_cast<uint16_t>(raw.bytes[4]) << 8U)));
    decoded.rx = apply_axis_deadzone(normalize_axis(raw.bytes[5], 127U), deadzone);
    decoded.ry = apply_axis_deadzone(-normalize_axis(raw.bytes[6], 128U), deadzone);
    decoded.lx = apply_axis_deadzone(normalize_axis(raw.bytes[7], 127U), deadzone);
    decoded.ly = apply_axis_deadzone(-normalize_axis(raw.bytes[8], 128U), deadzone);
    out = decoded;
    return pnxtypes::status::ok;
}

pnxtypes::status spi::init()
{
    if (pending_)
    {
        return pnxtypes::status::busy;
    }
    if (&tx_ == &rx_)
    {
        return pnxtypes::status::invalid_arg;
    }
    return bsp::gpio::set_active(cs_, false);
}

pnxtypes::status spi::start()
{
    using pnxtypes::status;
    if (pending_ || bsp::spi::state(spi_bus_).busy)
    {
        return status::busy;
    }
    constexpr uint8_t query[frame_size]{0x01U, 0x42U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
        0x00U, 0x00U};
    for (uint8_t i = 0; i < frame_size; ++i)
    {
        tx_[i] = query[i];
    }
    auto result = bsp::gpio::set_active(cs_, true);
    if (result != status::ok)
    {
        return result;
    }
    pending_ = true;
    result = bsp::spi::transmit_receive_it(spi_bus_, tx_, rx_, frame_size);
    if (result != status::ok)
    {
        const auto stopped = cancel();
        return stopped == status::ok ? result : stopped;
    }
    return status::ok;
}

pnxtypes::status spi::read(frame& raw)
{
    using pnxtypes::status;
    if (!pending_)
    {
        return status::not_initialized;
    }
    const auto& transfer = bsp::spi::state(spi_bus_);
    if (!transfer.complete)
    {
        return status::busy;
    }
    const auto result = transfer.last_status;
    if (result != status::ok)
    {
        const auto stopped = cancel();
        return stopped == status::ok ? result : stopped;
    }
    const auto released = bsp::gpio::set_active(cs_, false);
    if (released != status::ok)
    {
        return released;
    }
    pending_ = false;
    for (uint8_t i = 0; i < frame_size; ++i)
    {
        raw.bytes[i] = rx_[i];
    }
    return status::ok;
}

pnxtypes::status spi::cancel()
{
    using pnxtypes::status;
    if (!pending_)
    {
        return status::ok;
    }
    const auto stopped = bsp::spi::abort(spi_bus_);
    const auto released = bsp::gpio::set_active(cs_, false);
    if (stopped == status::ok && released == status::ok)
    {
        pending_ = false;
    }
    return stopped == status::ok ? released : stopped;
}

} // namespace remotes::ps2
