#pragma once

#include <cstdint>

namespace motor
{

class manager_interface
{
public:
    virtual ~manager_interface() = default;
    virtual void update() = 0;
    virtual void transmit(uint32_t now_us) = 0;
};

class motor_manager
{
public:
    static constexpr uint8_t max_managers = 8;

    bool register_manager(manager_interface& manager) noexcept
    {
        for (uint8_t i = 0; i < count_; ++i)
        {
            if (managers_[i] == &manager)
            {
                return false;
            }
        }
        if (count_ == max_managers)
        {
            return false;
        }
        managers_[count_++] = &manager;
        return true;
    }

    void update()
    {
        for (uint8_t i = 0; i < count_; ++i)
        {
            managers_[i]->update();
        }
    }

    void transmit(uint32_t now_us)
    {
        for (uint8_t i = 0; i < count_; ++i)
        {
            managers_[i]->transmit(now_us);
        }
    }

private:
    manager_interface* managers_[max_managers]{};
    uint8_t count_ = 0;
};

} // namespace motor
