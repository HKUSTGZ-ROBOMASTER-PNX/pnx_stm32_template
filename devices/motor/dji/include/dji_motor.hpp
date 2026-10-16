#pragma once
#include "motor.hpp"
#include "pid.hpp"
#include <cstdint>

namespace motor::dji
{

struct config_t
{
    float gear_ratio;
    int16_t max_output;
    float speed_pid_max_output;
    float position_pid_max_output;
    uint16_t encoder_resolution;
};

class motor : public ::motor::motor
{
public:
    bool supports(mode requested) const noexcept override;
    result update() override;
    virtual void receive(const uint8_t data[8]) noexcept;

    int16_t output() const noexcept
    {
        return output_;
    }

    const config_t& configuration() const noexcept
    {
        return config_;
    }

    control::pid speed_pid;
    control::pid position_pid;
    float position_offset = 0.0f;

protected:
    explicit motor(const config_t& config) noexcept;

private:
    config_t config_;
    int16_t output_ = 0;
    uint16_t last_encoder_ = 0;
    bool initialized_ = false;
};

} // namespace motor::dji
