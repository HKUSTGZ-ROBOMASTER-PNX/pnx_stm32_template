#include "bmi088.hpp"
#include "lkmotors.hpp"
#include "pid.hpp"
#include "remoter.hpp"

#include <type_traits>

using bmi088_read_acc =
    bool (imu::bmi088::*)(imu::accdata*);
static_assert(std::is_same_v<
              decltype(&imu::bmi088::read_acc),
              bmi088_read_acc>);

using pid_reset_state =
    void (control::pid::*)(float, float);
static_assert(std::is_same_v<
              decltype(&control::pid::reset_state),
              pid_reset_state>);

static_assert(std::is_base_of_v<
              motors::lkmotor, motors::lk9025>);
static_assert(
    remoter::ps2_config{}.uart_port == app::uart::ps2);
static_assert(std::is_same_v<
              decltype(remoter::state{}.ps2_buttons),
              std::uint16_t>);
static_assert(std::is_same_v<
              decltype(remoter::ps2_state{}.data),
              remoter::state>);
static_assert(std::is_same_v<
              decltype(remoter::ps2_state{}.frame_count),
              std::uint32_t>);
static_assert(
    static_cast<std::uint16_t>(remoter::ps2_button::square) == 0x8000U);
static_assert(
    remoter::is_held(0x4000U, remoter::ps2_button::cross));

int main()
{
    return 0;
}
