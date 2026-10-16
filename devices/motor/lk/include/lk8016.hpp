#pragma once
#include "lk_motor.hpp"

namespace motor::lk
{
class lk8016 final : public motor
{
public:
    lk8016() noexcept : motor(config_t{43.4028f, 0.0230399882f, 1.0f / 6.0f, 65536.0f, 2000, 2000.0f, 100.0f}) {}
};
}
