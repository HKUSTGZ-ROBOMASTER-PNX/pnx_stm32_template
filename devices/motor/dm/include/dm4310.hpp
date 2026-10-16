#pragma once
#include "dm_motor.hpp"
#include "constants.hpp"

namespace motor::dm
{
class dm4310 final : public motor
{
public:
    explicit dm4310(firmware_mode configured = firmware_mode::mit) noexcept
        : motor(config_t{2.0f * math::two_pi, 15.0f, 10.0f, 1.0f, true}, configured)
    {
    }
};
} // namespace motor::dm
