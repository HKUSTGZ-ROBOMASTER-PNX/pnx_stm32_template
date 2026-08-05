#pragma once

#include "demo_protocol.hpp"

#include <cstdint>

namespace demo::debug
{

struct link_state
{
    bool started = false;
    bool ready = false;
    bool connected = false;
    std::uint8_t last_status = protocol::status_code(types::status::not_configured);
    std::uint32_t rx_count = 0;
    std::uint32_t tx_count = 0;
    std::uint32_t error_count = 0;
    std::uint32_t last_seq = 0;
    std::uint32_t last_counter = 0;
    float last_value = 0.0f;
    bool last_flag = false;
    bool bsp_read_busy = false;
    bool bsp_write_busy = false;
    std::uint32_t bsp_last_read_status = 0;
    std::uint32_t bsp_last_write_status = 0;
    std::uint16_t bsp_last_read_len = 0;
    std::uint16_t bsp_last_write_requested = 0;
    std::uint16_t bsp_last_write_actual = 0;
    std::uint16_t bsp_pending_write_len = 0;
    std::uint32_t bsp_read_count = 0;
    std::uint32_t bsp_write_count = 0;
    std::uint32_t bsp_error_count = 0;
    std::uint32_t bsp_tx_wake_count = 0;
    std::uint32_t bsp_fill_count = 0;
    bool tx_pending = false;
    std::uint32_t tx_pending_seq = 0;
    std::uint32_t tx_fill_hit_count = 0;
    std::uint32_t tx_fill_miss_count = 0;
    protocol::host_packet last_rx{};
    protocol::device_packet last_tx{};
};

struct unit_test_state
{
    bool started = false;
    bool passed = false;
    std::uint32_t total_count = 0;
    std::uint32_t passed_count = 0;
    std::uint32_t failed_count = 0;
    std::uint32_t failure_mask = 0;
    std::uint32_t last_step = 0;
    std::uint32_t stage_mask = 0;
    std::uint32_t observed_count = 0;
    float value_a = 0.0f;
    float value_b = 0.0f;
    float value_c = 0.0f;
};

struct ps2_unit_state : unit_test_state
{
    bool generic_offline = true;
    bool mapping_match = false;
    std::uint32_t link = 0;
    std::uint32_t generic_source = 0;
    std::uint32_t generic_update_count = 0;
    std::uint32_t raw_update_count = 0;
    std::uint32_t event_count = 0;
    std::uint32_t frame_count = 0;
    std::uint32_t signal_count = 0;
    std::uint32_t connected_frame_count = 0;
    std::uint32_t remote_disconnected_count = 0;
    std::uint32_t receiver_offline_count = 0;
    std::uint32_t last_update_tick = 0;
    std::uint32_t last_frame_period_ticks = 0;
    std::uint32_t min_frame_period_ticks = 0;
    std::uint32_t max_frame_period_ticks = 0;

    std::uint16_t buttons = 0;
    std::uint16_t pressed = 0;
    std::uint16_t released = 0;
    std::uint16_t last_pressed = 0;
    std::uint16_t last_released = 0;
    std::uint16_t pressed_seen_mask = 0;
    std::uint16_t released_seen_mask = 0;
    std::uint32_t press_event_count = 0;
    std::uint32_t release_event_count = 0;

    bool square = false;
    bool cross = false;
    bool circle = false;
    bool triangle = false;
    bool r1 = false;
    bool l1 = false;
    bool r2 = false;
    bool l2 = false;
    bool left = false;
    bool down = false;
    bool right = false;
    bool up = false;
    bool start = false;
    bool r3 = false;
    bool l3 = false;
    bool select = false;

    std::uint8_t raw_left_x = 127;
    std::uint8_t raw_left_y = 128;
    std::uint8_t raw_right_x = 127;
    std::uint8_t raw_right_y = 128;

    float left_x = 0.0f;
    float left_y = 0.0f;
    float right_x = 0.0f;
    float right_y = 0.0f;
};

struct debug_instance_type
{
    link_state usart{};
    link_state usb{};
    ps2_unit_state ps2_unit{};
};

extern debug_instance_type& debug_instance;

void reset(link_state& state) noexcept;

} // namespace demo::debug

extern "C" demo::debug::debug_instance_type demo_debug_instance;
