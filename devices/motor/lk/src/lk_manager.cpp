#include "lk_manager.hpp"

namespace motor::lk
{
bool manager::register_motor(motor& m, uint8_t bus, uint16_t id) noexcept
{
    if (bus >= 3 || id < 0x141 || id > 0x144)
    {
        return false;
    }
    auto& slot = motors_[bus][id - 0x141];
    if (slot)
    {
        return false;
    }
    slot = &m;
    return true;
}

bool manager::receive(uint8_t bus, uint16_t id, const uint8_t data[8]) noexcept
{
    if (!data || bus >= 3 || id < 0x141 || id > 0x144)
    {
        return false;
    }
    auto* m = motors_[bus][id - 0x141];
    if (!m)
    {
        return false;
    }
    m->receive(data);
    return true;
}

void manager::update()
{
    for (auto& bus : motors_)
    {
        for (auto* m : bus)
        {
            if (m)
            {
                m->health_tick(period_);
                m->update();
            }
        }
    }
}

void manager::transmit(uint32_t now_us)
{
    if (!sender_)
    {
        return;
    }
    for (uint8_t bus = 0; bus < 3; ++bus)
    {
        if (!tx_[bus].due(now_us))
        {
            continue;
        }
        frame_t frame{};
        frame.id = 0x280;
        bool populated = false;
        for (uint8_t i = 0; i < 4; ++i)
        {
            auto* m = motors_[bus][i];
            if (!m)
            {
                continue;
            }
            populated = true;
            uint16_t raw = static_cast<uint16_t>(m->output());
            frame.data[2 * i] = static_cast<uint8_t>(raw);
            frame.data[2 * i + 1] = static_cast<uint8_t>(raw >> 8);
        }
        if (populated && sender_(ctx_, bus, frame))
        {
            tx_[bus].mark_sent(now_us);
        }
    }
}
} // namespace motor::lk
