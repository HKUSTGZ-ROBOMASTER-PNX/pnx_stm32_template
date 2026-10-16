#pragma once
#include "motor.hpp"
#include <cstdint>

namespace motor::dm
{
enum class firmware_mode : uint8_t
{
    mit,
    pos_vel,
    vel
};

struct config_t
{
    float position_limit;
    float velocity_limit;
    float torque_limit;
    float feedback_velocity_scale;
    bool wrap_position;
};

struct frame_t
{
    uint16_t id = 0;
    uint8_t data[8]{};
    uint8_t length = 8;
    bool fd = false;
};

class motor : public ::motor::motor
{
public:
    bool supports(mode requested) const noexcept override;
    result update() override;
    virtual bool receive(const uint8_t data[8]) noexcept;

    const frame_t& output() const noexcept
    {
        return output_;
    }

    virtual frame_t special(uint8_t opcode) const noexcept;

    uint8_t error_code() const noexcept
    {
        return error_;
    }

    float mos_temperature() const noexcept
    {
        return mos_temperature_;
    }

    float rotor_temperature() const noexcept
    {
        return rotor_temperature_;
    }

    const config_t& configuration() const noexcept
    {
        return config_;
    }

    void set_id(uint16_t id) noexcept
    {
        id_ = id;
    }

    void set_fd(bool fd) noexcept
    {
        fd_ = fd;
    }

    void set_offset(float offset) noexcept
    {
        offset_ = offset;
    }

protected:
    explicit motor(const config_t& config, firmware_mode configured = firmware_mode::mit) noexcept;

private:
    config_t config_;
    firmware_mode configured_;
    frame_t output_{};
    uint16_t id_ = 1;
    bool fd_ = false;
    float offset_ = 0.0f;
    uint8_t error_ = 0;
    float mos_temperature_ = 0;
    float rotor_temperature_ = 0;
};
} // namespace motor::dm
