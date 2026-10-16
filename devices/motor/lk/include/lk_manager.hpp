#pragma once
#include "motor_manager.hpp"
#include "motor_utils.hpp"
#include "lk_motor.hpp"
#include <cstdint>

namespace motor::lk
{
struct frame_t
{
    uint16_t id = 0;
    uint8_t data[8]{};
    uint8_t length = 8;
};

using send_fn = bool (*)(void*, uint8_t, const frame_t&);

class manager final : public manager_interface
{
public:
    explicit manager(send_fn sender, void* ctx = nullptr) : sender_(sender), ctx_(ctx)
    {
    }

    bool register_motor(motor& m, uint8_t bus, uint16_t feedback_id) noexcept;
    bool receive(uint8_t bus, uint16_t id, const uint8_t data[8]) noexcept;
    void update() override;

    void set_period(float seconds) noexcept
    {
        period_ = seconds;
    }

    bool set_tx_period(uint8_t bus, uint32_t us) noexcept
    {
        return bus < 3 && tx_[bus].set_period(us);
    }

    void transmit(uint32_t now_us) override;

private:
    motor* motors_[3][4]{};
    tx_schedule tx_[3]{};
    float period_ = 0.001f;
    send_fn sender_;
    void* ctx_;
};
} // namespace motor::lk
