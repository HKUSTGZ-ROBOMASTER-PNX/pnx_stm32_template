#pragma once
#include <cstdint>

namespace math
{
// Caller supplies 1..31 bits, finite min < max and an in-range raw value.
inline float dequantize(uint32_t value, float min, float max, uint8_t bits)
{
    return static_cast<float>(value) * (max - min) /
               static_cast<float>((UINT32_C(1) << bits) - 1U) +
           min;
}
} // namespace math
