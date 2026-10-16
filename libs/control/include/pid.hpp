#pragma once

#include "math.h"
#include "stdint.h"

#include "constrains.hpp"

namespace control
{

enum class pid_mode : uint8_t
{
    position = 0x01,
    delta = 0x02, // Uses kd * velocity; not an incremental PID.
    trapezoid_integral = 0x04,
    changing_integral_rate = 0x08,
    integral_separation = 0x10,
    derivative_on_measurement = 0x20,
    integral_limit = 0x40,
};

// Select one base mode (position/delta), then combine the desired options.
constexpr pid_mode operator|(pid_mode lhs, pid_mode rhs) noexcept
{
    return static_cast<pid_mode>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

class pid
{
public:
    pid(float kp, float ki, float kd, float max_out, float max_iout,
        pid_mode mode = pid_mode::position)
        : mode(mode), kp(kp), ki(ki), kd(kd), max_out(max_out), max_iout(max_iout)
    {
    }

    void tune(float kp, float ki, float kd)
    {
        this->kp = kp;
        this->ki = ki;
        this->kd = kd;
    }

    // Call at a fixed period: ki and kd already include the sample-period scaling.
    // velocity is the signed derivative contribution before multiplication by kd.
    void update(float velocity = 0.0f)
    {
        err[1] = err[0];
        err[0] = ref - fdb;
        p_out = kp * err[0];
        i_term = ki * err[0];
        d_out = 0.0f;

        if (enabled(pid_mode::position))
        {
            d_out = kd * (err[0] - err[1]);
        }
        else if (enabled(pid_mode::delta))
        {
            d_out = kd * velocity;
        }

        if (enabled(pid_mode::trapezoid_integral))
        {
            i_term = ki * (err[0] + err[1]) * 0.5f;
        }
        if (enabled(pid_mode::derivative_on_measurement))
        {
            d_out = kd * (last_fdb - fdb);
        }
        if (enabled(pid_mode::integral_separation))
        {
            apply_integral_separation();
        }
        if (enabled(pid_mode::changing_integral_rate))
        {
            apply_changing_integral_rate();
        }
        if (enabled(pid_mode::integral_limit))
        {
            apply_integral_limit();
        }

        i_out = math::limit_abs(i_out + i_term, max_iout);
        last_fdb = fdb;
        result = math::limit_abs(p_out + i_out + d_out, max_out);
    }

    void clear()
    {
        reset_state(0.0f, 0.0f);
    }

    // Seed history from the current input to avoid a derivative kick on the next update.
    void reset_state(float reference, float feedback)
    {
        ref = reference;
        fdb = feedback;
        last_fdb = feedback;
        err[0] = reference - feedback;
        err[1] = err[0];
        p_out = 0.0f;
        i_out = 0.0f;
        d_out = 0.0f;
        i_term = 0.0f;
        result = 0.0f;
    }

    pid_mode mode = pid_mode::position;

    float kp = 0.0f;
    float ki = 0.0f;
    float kd = 0.0f;
    float ref = 0.0f;
    float fdb = 0.0f;
    float result = 0.0f;

    float max_out = 0.0f;
    float max_iout = 0.0f;

    float scalar_a = 0.0f;
    float scalar_b = 0.0f;

private:
    bool enabled(pid_mode option) const
    {
        return (static_cast<uint8_t>(mode) & static_cast<uint8_t>(option)) != 0;
    }

    void apply_integral_separation()
    {
        // Preserve the original separation threshold and allow integral unwinding.
        if (err[0] * i_out > 0.0f && std::fabs(err[0]) > max_out)
        {
            i_term = 0.0f;
        }
    }

    void apply_changing_integral_rate()
    {
        if (err[0] * i_out <= 0.0f)
        {
            return;
        }

        const float error = std::fabs(err[0]);
        if (error <= scalar_b)
        {
            return;
        }
        if (scalar_a > 0.0f && error <= scalar_a + scalar_b)
        {
            i_term *= (scalar_a + scalar_b - error) / scalar_a;
        }
        else
        {
            i_term = 0.0f;
        }
    }

    void apply_integral_limit()
    {
        const float next_iout = i_out + i_term;
        const float output = p_out + i_out + d_out;
        if (std::fabs(output) > max_out && err[0] * i_out > 0.0f)
        {
            i_term = 0.0f;
        }

        if (next_iout > max_iout)
        {
            i_term = 0.0f;
            i_out = max_iout;
        }
        else if (next_iout < -max_iout)
        {
            i_term = 0.0f;
            i_out = -max_iout;
        }
    }

    float last_fdb = 0.0f;
    float err[2]{};
    float p_out = 0.0f;
    float i_out = 0.0f;
    float d_out = 0.0f;
    float i_term = 0.0f;
};

} // namespace control
