#pragma once
#include "dji_motor.hpp"

namespace motor::dji
{
class m3508 final : public motor
{
public:
    m3508() noexcept : motor(config_t{3591.0f / 187.0f, 16384, 16384.0f, 100.0f, 8192})
    {
    }
};
} // namespace motor::dji
