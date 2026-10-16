#include "dm_motor.hpp"
#include "constants.hpp"
#include "constrains.hpp"
#include "quantize.hpp"
#include <cmath>
#include <cstring>

namespace motor::dm
{
namespace
{

uint16_t quant(float v, float lo, float hi, unsigned bits)
{
    const float range = float((1u << bits) - 1u);
    return static_cast<uint16_t>((math::clamp(v, lo, hi) - lo) * range / (hi - lo));
}

void put_float(uint8_t* dst, float v)
{
    static_assert(sizeof(float) == 4);
    std::memcpy(dst, &v, 4);
}

} // namespace

motor::motor(const config_t& config, firmware_mode configured) noexcept
    : config_(config), configured_(configured)
{
}

bool motor::supports(mode m) const noexcept
{
    if (m == mode::relax)
    {
        return true;
    }
    if (configured_ == firmware_mode::mit)
    {
        return m == mode::mit || m == mode::torque;
    }
    if (configured_ == firmware_mode::pos_vel)
    {
        return m == mode::pos_vel;
    }
    return m == mode::vel;
}

result motor::update()
{
    output_ = motor::special(0xfd);
    if (!supports(motor_mode))
    {
        return result::unsupported_mode;
    }
    if (motor_mode == mode::relax)
    {
        return result::ok;
    }
    if (!std::isfinite(cmd.position) || !std::isfinite(cmd.velocity) ||
        !std::isfinite(cmd.torque) || !std::isfinite(cmd.kp) || !std::isfinite(cmd.kd))
    {
        return result::invalid_command;
    }

    const float pmax = config_.position_limit;
    const float vmax = config_.velocity_limit;
    const float tmax = config_.torque_limit;
    if (motor_mode == mode::mit || motor_mode == mode::torque)
    {
        float p = 0.0f;
        if (motor_mode != mode::torque)
        {
            p = config_.wrap_position ? math::wrap(cmd.position + offset_, -math::pi, math::pi)
                                      : cmd.position;
        }
        const float v = motor_mode == mode::torque ? 0.0f : cmd.velocity;
        const float kp = motor_mode == mode::torque ? 0.0f : cmd.kp;
        const float kd = motor_mode == mode::torque ? 0.0f : cmd.kd;
        const uint16_t q = quant(p, -pmax, pmax, 16);
        const uint16_t dq = quant(v, -vmax, vmax, 12);
        const uint16_t k = quant(kp, 0.0f, 500.0f, 12);
        const uint16_t d = quant(kd, 0.0f, 500.0f, 12);
        const uint16_t t = quant(cmd.torque, -tmax, tmax, 12);
        output_.data[0] = q >> 8;
        output_.data[1] = q;
        output_.data[2] = dq >> 4;
        output_.data[3] = ((dq & 15) << 4) | (k >> 8);
        output_.data[4] = k;
        output_.data[5] = d >> 4;
        output_.data[6] = ((d & 15) << 4) | (t >> 8);
        output_.data[7] = t;
    }
    else if (motor_mode == mode::pos_vel)
    {
        const float p = config_.wrap_position
                            ? math::wrap(cmd.position + offset_, -math::pi, math::pi)
                            : math::clamp(cmd.position, -pmax, pmax);
        put_float(output_.data, p);
        put_float(output_.data + 4, cmd.velocity);
    }
    else if (motor_mode == mode::vel)
    {
        put_float(output_.data, cmd.velocity);
    }
    return result::ok;
}

bool motor::receive(const uint8_t data[8]) noexcept
{
    if (!data)
    {
        return false;
    }
    error_ = data[0] >> 4;
    const float pmax = config_.position_limit;
    const float vmax = config_.velocity_limit;
    const float tmax = config_.torque_limit;
    const uint16_t p = (uint16_t(data[1]) << 8) | data[2];
    const uint16_t v = (uint16_t(data[3]) << 4) | (data[4] >> 4);
    const uint16_t t = (uint16_t(data[4] & 15) << 8) | data[5];
    fdb.position = math::dequantize(p, -pmax, pmax, 16);
    if (config_.wrap_position)
    {
        fdb.position = math::wrap(fdb.position, -math::pi, math::pi);
    }
    fdb.velocity = math::dequantize(v, -vmax, vmax, 12) * config_.feedback_velocity_scale;
    fdb.torque = math::dequantize(t, -tmax, tmax, 12);
    mos_temperature_ = data[6];
    rotor_temperature_ = data[7];
    fdb.temperature = mos_temperature_;
    mark_feedback(fdb.torque);
    return true;
}

frame_t motor::special(uint8_t opcode) const noexcept
{
    frame_t frame{};
    frame.id = id_;
    frame.fd = fd_;
    if (configured_ == firmware_mode::pos_vel)
    {
        frame.id += 0x100;
    }
    if (configured_ == firmware_mode::vel)
    {
        frame.id += 0x200;
    }
    for (auto& b : frame.data)
    {
        b = 0xff;
    }
    frame.data[7] = opcode;
    return frame;
}
} // namespace motor::dm
