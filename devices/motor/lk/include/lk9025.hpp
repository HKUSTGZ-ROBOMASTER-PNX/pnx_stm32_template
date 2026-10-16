#pragma once
#include "lk_motor.hpp"

namespace motor::lk
{
class lk9025 final : public motor
{
public:
    lk9025() noexcept : motor(config_t{195.3125f, 0.00512f, 1.0f, 65536.0f, 2000, 2000.0f, 100.0f}) {}
};
}
