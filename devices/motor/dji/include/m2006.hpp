#pragma once
#include "dji_motor.hpp"

namespace motor::dji
{
class m2006 final : public motor
{
public:
    m2006() noexcept : motor(config_t{36.0f, 10000, 10000.0f, 100.0f, 8192})
    {
    }
};
} // namespace motor::dji
