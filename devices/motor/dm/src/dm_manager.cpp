#include "dm_manager.hpp"

namespace motor::dm
{
bool manager::register_motor(motor& m, uint8_t bus, uint16_t control_id,
    uint16_t feedback_id) noexcept
{
    if (bus >= 3 || control_id == 0 || control_id > 0x7ff || feedback_id > 0x7ff || count_ >= 24)
    {
        return false;
    }
    for (uint8_t i = 0; i < count_; ++i)
    {
        if (entries_[i].ptr == &m ||
            (entries_[i].bus == bus && entries_[i].feedback_id == feedback_id))
        {
            return false;
        }
    }
    m.set_id(control_id);
    entries_[count_++] = {&m, feedback_id, bus};
    return true;
}

bool manager::receive(uint8_t bus, uint16_t id, const uint8_t data[8]) noexcept
{
    if (!data)
    {
        return false;
    }
    for (uint8_t i = 0; i < count_; ++i)
    {
        if (entries_[i].bus == bus && entries_[i].feedback_id == id)
        {
            return entries_[i].ptr->receive(data);
        }
    }
    return false;
}

bool manager::special(motor& m, uint8_t opcode) noexcept
{
    if (!sender_ || (opcode != 0xfc && opcode != 0xfd && opcode != 0xfe && opcode != 0xfb))
    {
        return false;
    }
    for (uint8_t i = 0; i < count_; ++i)
    {
        if (entries_[i].ptr == &m)
        {
            return sender_(ctx_, entries_[i].bus, m.special(opcode));
        }
    }
    return false;
}

void manager::update()
{
    for (uint8_t i = 0; i < count_; ++i)
    {
        entries_[i].ptr->health_tick(period_);
        entries_[i].ptr->update();
    }
}

void manager::transmit(uint32_t now_us)
{
    if (!sender_)
    {
        return;
    }
    for (uint8_t i = 0; i < count_; ++i)
    {
        if (tx_[i].due(now_us) && sender_(ctx_, entries_[i].bus, entries_[i].ptr->output()))
        {
            tx_[i].mark_sent(now_us);
        }
    }
}
} // namespace motor::dm
