#pragma once
#include "dm_motor.hpp"

namespace motor::dm
{
class dm8009p final : public motor
{
public:
    explicit dm8009p(firmware_mode configured = firmware_mode::mit) noexcept
        : motor(config_t{62.8319f, 45.0f, 54.0f, 1.0f / 9.0f, false}, configured)
    {
    }
};
} // namespace motor::dm
