#pragma once

#include <cstdint>
#include "motor_utils.hpp"
#include <cmath>

namespace motor
{

enum class state : uint8_t
{
    online = 0,
    offline,
    blocked
};
enum class mode : uint8_t
{
    relax = 0,
    current,
    torque,
    voltage,
    mit,
    pos_vel,
    vel
};
enum class result : uint8_t
{
    ok = 0,
    unsupported_mode,
    invalid_command,
    not_ready
};

struct command_t
{
    int16_t current = 0;
    int16_t voltage = 0;
    float torque = 0.0f;
    float position = 0.0f;
    float velocity = 0.0f;
    float kp = 0.0f;
    float kd = 0.0f;
};

struct feedback_t
{
    float position = 0.0f;
    float velocity = 0.0f;
    float torque = 0.0f;
    float temperature = 0.0f;
};

class motor
{
public:
    virtual ~motor() = default;
    state motor_state = state::offline;
    mode motor_mode = mode::relax;
    command_t cmd{};
    feedback_t fdb{};
    float torque_constant = 0.0f;

    void configure_stall(const stall_config_t& cfg) noexcept
    {
        stall_.configure(cfg);
    }

    bool is_blocked() const noexcept
    {
        return stall_.blocked();
    }

    float feedback_age() const noexcept
    {
        return feedback_age_;
    }

    // Call once per control cycle; this is diagnostic only, never an automatic shutdown.
    void health_tick(float dt) noexcept
    {
        if (!(dt > 0.0f) || !std::isfinite(dt))
        {
            return;
        }
        feedback_age_ += dt;
        const bool blocked = stall_.update(fdb.velocity, feedback_load_, feedback_age_, dt);
        if (feedback_age_ > stall_.config().feedback_timeout)
        {
            motor_state = state::offline;
        }
        else
        {
            motor_state = blocked ? state::blocked : state::online;
        }
    }

protected:
    void mark_feedback(float load) noexcept
    {
        feedback_age_ = 0.0f;
        feedback_load_ = load;
        motor_state = state::online;
    }

private:
    stall_detector stall_{};
    float feedback_age_ = 1.0e6f;
    float feedback_load_ = 0.0f;

    virtual bool supports(mode requested) const noexcept = 0;
    virtual result update() = 0;
};

} // namespace motor
