#pragma once
#include "dji_motor.hpp"

namespace motor::dji
{
class gm6020 final : public motor
{
public:
    gm6020() noexcept : motor(config_t{1.0f, 15000, 15000.0f, 100.0f, 8192})
    {
    }
};
} // namespace motor::dji
