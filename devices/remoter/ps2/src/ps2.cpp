#include "ps2.hpp"
#include "bsp_dwt.hpp"
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
    const pnxtypes::status status = bsp::spi::wait_ready(spi_bus_, timeout_ms_);
    return status == pnxtypes::status::ok ? bsp::gpio::set_active(cs_, false) : status;
}

pnxtypes::status spi::transfer(const uint8_t* tx, uint8_t* rx, uint8_t size)
{
    if (tx == nullptr || rx == nullptr || size == 0U)
    {
        return pnxtypes::status::invalid_arg;
    }
    pnxtypes::status status = bsp::gpio::set_active(cs_, true);
    if (status == pnxtypes::status::ok)
    {
        status = bsp::spi::transmit_receive(spi_bus_, tx, rx, size, timeout_ms_);
    }
    const pnxtypes::status release_status = bsp::gpio::set_active(cs_, false);
    return status == pnxtypes::status::ok ? release_status : status;
}

pnxtypes::status gpio::init()
{
    pnxtypes::status status = bsp::dwt::init();
    if (status != pnxtypes::status::ok)
    {
        return status;
    }
    status = bsp::gpio::set_active(cs_, false);
    if (status == pnxtypes::status::ok)
    {
        status = bsp::gpio::write(clk_, true);
    }
    return status == pnxtypes::status::ok ? bsp::gpio::write(cmd_, true) : status;
}

pnxtypes::status gpio::transfer_byte(uint8_t tx, uint8_t& rx)
{
    rx = 0U;
    for (uint8_t bit = 0U; bit < 8U; ++bit)
    {
        pnxtypes::status status = bsp::gpio::write(cmd_, (tx & (1U << bit)) != 0U);
        if (status != pnxtypes::status::ok)
        {
            return status;
        }
        bsp::dwt::delay_us(timing_.half_period_us);
        status = bsp::gpio::write(clk_, false);
        if (status != pnxtypes::status::ok)
        {
            return status;
        }
        bsp::dwt::delay_us(timing_.half_period_us);
        bool data_high = false;
        status = bsp::gpio::read(data_, data_high);
        if (status != pnxtypes::status::ok)
        {
            return status;
        }
        if (data_high)
        {
            rx = static_cast<uint8_t>(rx | (1U << bit));
        }
        status = bsp::gpio::write(clk_, true);
        if (status != pnxtypes::status::ok)
        {
            return status;
        }
        bsp::dwt::delay_us(timing_.half_period_us);
    }
    return pnxtypes::status::ok;
}

pnxtypes::status gpio::transfer(const uint8_t* tx, uint8_t* rx, uint8_t size)
{
    if (tx == nullptr || rx == nullptr || size == 0U)
    {
        return pnxtypes::status::invalid_arg;
    }
    pnxtypes::status status = bsp::gpio::set_active(cs_, true);
    if (status == pnxtypes::status::ok)
    {
        bsp::dwt::delay_us(timing_.cs_setup_us);
        for (uint8_t index = 0U; index < size && status == pnxtypes::status::ok; ++index)
        {
            status = transfer_byte(tx[index], rx[index]);
        }
        bsp::dwt::delay_us(timing_.cs_hold_us);
    }
    const pnxtypes::status release_status = bsp::gpio::set_active(cs_, false);
    return status == pnxtypes::status::ok ? release_status : status;
}

} // namespace remotes::ps2
