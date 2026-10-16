#pragma once

#include <cmath>
#include "stdint.h"

namespace motor
{

struct stall_config_t
{
    bool enabled = false;
    float velocity_threshold = 0.1f; // rad/s
    float load_threshold = 0.0f;     // model-specific: Nm for DM/LK, raw feedback units for DJI
    float trigger_time = 0.2f;       // seconds
    float release_time = 0.05f;      // seconds
    float feedback_timeout = 0.1f;   // seconds
};

class stall_detector
{
public:
    void configure(const stall_config_t& config) noexcept
    {
        config_ = config;
        reset();
    }

    const stall_config_t& config() const noexcept
    {
        return config_;
    }

    void reset() noexcept
    {
        active_time_ = 0.0f;
        release_time_ = 0.0f;
        blocked_ = false;
    }

    bool blocked() const noexcept
    {
        return blocked_;
    }

    bool update(float velocity, float load, float feedback_age, float dt) noexcept
    {
        if (!config_.enabled || !std::isfinite(dt) || dt <= 0.0f || !std::isfinite(velocity) ||
            !std::isfinite(load) || !std::isfinite(feedback_age) ||
            feedback_age > config_.feedback_timeout || config_.velocity_threshold <= 0.0f ||
            config_.load_threshold <= 0.0f || config_.trigger_time <= 0.0f ||
            config_.release_time <= 0.0f)
        {
            reset();
            return false;
        }
        const bool candidate = std::fabs(velocity) <= config_.velocity_threshold &&
                               std::fabs(load) >= config_.load_threshold;
        if (candidate)
        {
            release_time_ = 0.0f;
            if (!blocked_)
            {
                active_time_ += dt;
                if (active_time_ >= config_.trigger_time)
                {
                    blocked_ = true;
                }
            }
        }
        else
        {
            active_time_ = 0.0f;
            if (blocked_)
            {
                release_time_ += dt;
                if (release_time_ >= config_.release_time)
                {
                    reset();
                }
            }
        }
        return blocked_;
    }

private:
    stall_config_t config_{};
    float active_time_ = 0.0f;
    float release_time_ = 0.0f;
    bool blocked_ = false;
};

class tx_schedule
{
public:
    bool set_period(uint32_t us) noexcept
    {
        if (!us)
        {
            return false;
        }
        period_ = us;
        return true;
    }

    bool due(uint32_t now) const noexcept
    {
        return !sent_ || uint32_t(now - last_) >= period_;
    }

    void mark_sent(uint32_t now) noexcept
    {
        last_ = now;
        sent_ = true;
    }

private:
    uint32_t period_ = 1000, last_ = 0;
    bool sent_ = false;
};

} // namespace motor
