#include "dji_motor.hpp"
#include "constants.hpp"
#include "constrains.hpp"
#include <cmath>

namespace motor::dji
{

motor::motor(const config_t& config) noexcept
    : speed_pid(0.1f, 0.0f, 0.0f, config.speed_pid_max_output, 3.0f),
      position_pid(0.1f, 0.0f, 0.0f, config.position_pid_max_output, 3.0f), config_(config)
{
}

bool motor::supports(mode requested) const noexcept
{
    switch (requested)
    {
        case mode::relax:
        case mode::current:
        case mode::vel:
        case mode::pos_vel:
            return true;
        default:
            return false;
    }
}

result motor::update()
{
    output_ = 0;
    if (!supports(motor_mode))
    {
        return result::unsupported_mode;
    }
    if (motor_mode == mode::relax)
    {
        return result::ok;
    }
    if (!std::isfinite(cmd.position) || !std::isfinite(cmd.velocity))
    {
        return result::invalid_command;
    }
    if (motor_mode == mode::current)
    {
        output_ = static_cast<int16_t>(math::limit_abs(static_cast<float>(cmd.current),
            static_cast<float>(config_.max_output)));
        return result::ok;
    }
    float speed_ref = cmd.velocity;
    if (motor_mode == mode::pos_vel)
    {
        position_pid.ref = cmd.position;
        position_pid.fdb = fdb.position;
        position_pid.update();
        speed_ref = position_pid.result;
    }
    speed_pid.ref = speed_ref;
    speed_pid.fdb = fdb.velocity;
    speed_pid.update();
    output_ = static_cast<int16_t>(
        math::limit_abs(speed_pid.result, static_cast<float>(config_.max_output)));
    return result::ok;
}

void motor::receive(const uint8_t data[8]) noexcept
{
    if (!data)
    {
        return;
    }
    const uint16_t encoder = static_cast<uint16_t>((uint16_t(data[0]) << 8) | data[1]);
    const int16_t rpm = static_cast<int16_t>((uint16_t(data[2]) << 8) | data[3]);
    const float resolution = static_cast<float>(config_.encoder_resolution);
    if (!initialized_)
    {
        last_encoder_ = encoder;
        initialized_ = true;
        fdb.position =
            (float(encoder) * math::two_pi / resolution) / config_.gear_ratio - position_offset;
    }
    else
    {
        int32_t diff = int32_t(encoder) - int32_t(last_encoder_);
        const int32_t span = config_.encoder_resolution;
        if (diff > span / 2)
        {
            diff -= span;
        }
        if (diff < -span / 2)
        {
            diff += span;
        }
        fdb.position += float(diff) * math::two_pi / (resolution * config_.gear_ratio);
        last_encoder_ = encoder;
    }
    fdb.velocity = float(rpm) * math::two_pi / (60.0f * config_.gear_ratio);
    fdb.temperature = float(data[6]);
    const int16_t raw_current = static_cast<int16_t>((uint16_t(data[4]) << 8) | data[5]);
    mark_feedback(static_cast<float>(raw_current));
    // Feedback raw current is not calibrated to Nm; retain unknown torque as zero.
    fdb.torque = 0.0f;
}
} // namespace motor::dji
