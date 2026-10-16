#pragma once
#include "motor_utils.hpp"
#include "motor_manager.hpp"
#include "dm_motor.hpp"
#include <cstdint>

namespace motor::dm
{
using send_fn = bool (*)(void*, uint8_t, const frame_t&);

class manager final : public manager_interface
{
public:
    explicit manager(send_fn sender, void* ctx = nullptr) : sender_(sender), ctx_(ctx)
    {
    }

    bool register_motor(motor& m, uint8_t bus, uint16_t control_id, uint16_t feedback_id) noexcept;
    bool receive(uint8_t bus, uint16_t feedback_id, const uint8_t data[8]) noexcept;
    bool special(motor& m, uint8_t opcode) noexcept;
    void update() override;

    void set_period(float seconds) noexcept
    {
        period_ = seconds;
    }

    bool set_tx_period(const motor& m, uint32_t us) noexcept
    {
        for (uint8_t i = 0; i < count_; ++i)
        {
            if (entries_[i].ptr == &m)
            {
                return tx_[i].set_period(us);
            }
        }
        return false;
    }

    void transmit(uint32_t now_us) override;

private:
    struct entry
    {
        motor* ptr = nullptr;
        uint16_t feedback_id = 0;
        uint8_t bus = 0;
    };

    entry entries_[24]{};
    tx_schedule tx_[24]{};
    uint8_t count_ = 0;
    float period_ = 0.001f;
    send_fn sender_;
    void* ctx_;
};
} // namespace motor::dm
