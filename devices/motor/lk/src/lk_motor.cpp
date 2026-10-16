#include "lk_motor.hpp"
#include "constants.hpp"
#include "constrains.hpp"
#include <cmath>

namespace motor::lk
{

motor::motor(const config_t& config) noexcept
    : speed_pid(0.1f, 0.0f, 0.0f, config.speed_pid_max_output, 3.0f),
      position_pid(0.1f, 0.0f, 0.0f, config.position_pid_max_output, 3.0f), config_(config)
{
}

bool motor::supports(mode m) const noexcept
{
    switch (m)
    {
        case mode::relax:
        case mode::current:
        case mode::vel:
        case mode::pos_vel:
        case mode::torque:
        case mode::mit:
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
    if (!std::isfinite(cmd.position) || !std::isfinite(cmd.velocity) ||
        !std::isfinite(cmd.torque) || !std::isfinite(cmd.kp) || !std::isfinite(cmd.kd))
    {
        return result::invalid_command;
    }
    float raw = 0.0f;
    switch (motor_mode)
    {
        case mode::current:
            raw = static_cast<float>(cmd.current);
            break;
        case mode::torque:
            raw = cmd.torque * config_.torque_command_per_nm;
            break;
        case mode::mit:
        {
            const float tau =
                cmd.torque + cmd.kp * math::wrap(cmd.position - fdb.position, -math::pi, math::pi) +
                cmd.kd * (cmd.velocity - fdb.velocity);
            raw = tau * config_.torque_command_per_nm;
            break;
        }
        case mode::pos_vel:
        case mode::vel:
            speed_pid.ref = cmd.velocity;
            if (motor_mode == mode::pos_vel)
            {
                position_pid.ref = cmd.position;
                position_pid.fdb = fdb.position;
                position_pid.update();
                speed_pid.ref += position_pid.result;
            }
            speed_pid.fdb = fdb.velocity;
            speed_pid.update();
            raw = speed_pid.result;
            break;
        default:
            break;
    }
    output_ = static_cast<int16_t>(math::limit_abs(raw, static_cast<float>(config_.max_output)));
    return result::ok;
}

void motor::receive(const uint8_t data[8]) noexcept
{
    if (!data)
    {
        return;
    }
    const uint16_t encoder = (uint16_t(data[7]) << 8) | data[6];
    const int16_t speed_dps = static_cast<int16_t>((uint16_t(data[5]) << 8) | data[4]);
    const int16_t raw_torque = static_cast<int16_t>((uint16_t(data[3]) << 8) | data[2]);
    fdb.position =
        math::wrap((float(encoder) - encoder_offset) * (math::two_pi / config_.encoder_resolution),
            -math::pi, math::pi);
    fdb.velocity = math::degrees_to_radians(float(speed_dps)) * config_.feedback_velocity_scale;
    fdb.torque = float(raw_torque) * config_.torque_feedback_per_raw;
    fdb.temperature = float(data[1]);
    mark_feedback(fdb.torque);
}
} // namespace motor::lk
