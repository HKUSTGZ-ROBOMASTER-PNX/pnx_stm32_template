#include "ps2_demo.hpp"

#include "config.hpp"
#include "demo_debug.hpp"
#include "msg.hpp"
#include "remoter.hpp"
#include "tx_api.h"

#include <cstdint>

namespace demo::ps2
{
namespace
{

enum stage : std::uint32_t
{
    source_enabled = 1U << 0U,
    service_initialized = 1U << 1U,
    raw_subscriber_created = 1U << 2U,
    generic_subscriber_created = 1U << 3U,
    monitor_thread_started = 1U << 4U,
    raw_data_received = 1U << 5U,
    connected_frame_received = 1U << 6U,
    generic_data_received = 1U << 7U,
    mapping_verified = 1U << 8U,
};

enum failure : std::uint32_t
{
    source_not_enabled = 1U << 0U,
    service_init_failed = 1U << 1U,
    raw_subscribe_failed = 1U << 2U,
    generic_subscribe_failed = 1U << 3U,
    thread_create_failed = 1U << 4U,
    receiver_data_timeout = 1U << 5U,
    receiver_offline = 1U << 6U,
    mapping_mismatch = 1U << 7U,
};

constexpr ULONG monitor_period_ticks = 5U;
constexpr ULONG receiver_startup_timeout_ticks = 3000U;

TX_THREAD monitor_thread{};
alignas(8) std::uint8_t monitor_stack[1536]{};
bool monitor_started = false;
msg::subscriber raw_sub{};
msg::subscriber generic_sub{};
ULONG started_at = 0;

::remoter::ps2_state latest_raw{};
::remoter::state latest_generic{};
std::uint32_t stages = 0;
std::uint32_t raw_update_count = 0;
std::uint32_t connected_frame_count = 0;
std::uint32_t remote_disconnected_count = 0;
std::uint32_t receiver_offline_count = 0;
std::uint32_t previous_frame_tick = 0;
std::uint32_t previous_frame_count = 0;
std::uint32_t last_frame_period_ticks = 0;
std::uint32_t min_frame_period_ticks = 0;
std::uint32_t max_frame_period_ticks = 0;
std::uint16_t last_pressed = 0;
std::uint16_t last_released = 0;
std::uint16_t pressed_seen_mask = 0;
std::uint16_t released_seen_mask = 0;
std::uint32_t press_event_count = 0;
std::uint32_t release_event_count = 0;
std::uint32_t previous_event_count = 0;
bool event_seen = false;
::remoter::ps2_link_state previous_link = ::remoter::ps2_link_state::receiver_offline;
bool link_seen = false;

void record_raw_update(const ::remoter::ps2_state& data) noexcept
{
    latest_raw = data;
    ++raw_update_count;
    stages |= raw_data_received;
    const auto& raw = data.data;

    if (!link_seen || raw.ps2_link != previous_link)
    {
        if (raw.ps2_link == ::remoter::ps2_link_state::remote_disconnected)
        {
            ++remote_disconnected_count;
        }
        else if (raw.ps2_link == ::remoter::ps2_link_state::receiver_offline)
        {
            ++receiver_offline_count;
        }
        previous_link = raw.ps2_link;
        link_seen = true;
    }

    if (!event_seen || raw.ps2_event_count != previous_event_count)
    {
        if (raw.ps2_pressed != 0U)
        {
            last_pressed = raw.ps2_pressed;
            pressed_seen_mask |= raw.ps2_pressed;
            ++press_event_count;
        }
        if (raw.ps2_released != 0U)
        {
            last_released = raw.ps2_released;
            released_seen_mask |= raw.ps2_released;
            ++release_event_count;
        }
        previous_event_count = raw.ps2_event_count;
        event_seen = true;
    }

    if (raw.ps2_link != ::remoter::ps2_link_state::connected)
    {
        return;
    }

    stages |= connected_frame_received;
    connected_frame_count = data.frame_count;

    if (data.frame_count != previous_frame_count)
    {
        const std::uint32_t frame_delta = data.frame_count - previous_frame_count;
        if (previous_frame_tick != 0U && frame_delta != 0U)
        {
            last_frame_period_ticks =
                (data.last_signal_tick - previous_frame_tick) / frame_delta;
            if (min_frame_period_ticks == 0U ||
                last_frame_period_ticks < min_frame_period_ticks)
            {
                min_frame_period_ticks = last_frame_period_ticks;
            }
            if (last_frame_period_ticks > max_frame_period_ticks)
            {
                max_frame_period_ticks = last_frame_period_ticks;
            }
        }
        previous_frame_count = data.frame_count;
        previous_frame_tick = data.last_signal_tick;
    }
}

void sync_debug(bool startup_timed_out) noexcept
{
    auto& state = demo::debug::debug_instance.ps2_unit;
    const auto& raw = latest_raw.data;
    const bool mapping_match =
        latest_generic.ps2_link == raw.ps2_link &&
        latest_generic.ps2_buttons == raw.ps2_buttons &&
        latest_generic.ps2_pressed == raw.ps2_pressed &&
        latest_generic.ps2_released == raw.ps2_released &&
        latest_generic.ps2_event_count == raw.ps2_event_count;
    if ((stages & generic_data_received) != 0U && mapping_match)
    {
        stages |= mapping_verified;
    }

    state.stage_mask = stages;
    state.last_step = stages;
    state.observed_count = raw_update_count;
    state.generic_offline = latest_generic.offline;
    state.mapping_match = mapping_match;
    state.link = static_cast<std::uint32_t>(raw.ps2_link);
    state.generic_source = static_cast<std::uint32_t>(latest_generic.active_source);
    state.generic_update_count = latest_generic.update_count;
    state.raw_update_count = raw_update_count;
    state.event_count = raw.ps2_event_count;
    state.frame_count = latest_raw.frame_count;
    state.signal_count = latest_raw.signal_count;
    state.connected_frame_count = connected_frame_count;
    state.remote_disconnected_count = remote_disconnected_count;
    state.receiver_offline_count = receiver_offline_count;
    state.last_update_tick = latest_raw.last_signal_tick;
    state.last_frame_period_ticks = last_frame_period_ticks;
    state.min_frame_period_ticks = min_frame_period_ticks;
    state.max_frame_period_ticks = max_frame_period_ticks;

    state.buttons = raw.ps2_buttons;
    state.pressed = raw.ps2_pressed;
    state.released = raw.ps2_released;
    state.last_pressed = last_pressed;
    state.last_released = last_released;
    state.pressed_seen_mask = pressed_seen_mask;
    state.released_seen_mask = released_seen_mask;
    state.press_event_count = press_event_count;
    state.release_event_count = release_event_count;
    state.square = ::remoter::is_held(raw.ps2_buttons, ::remoter::ps2_button::square);
    state.cross = ::remoter::is_held(raw.ps2_buttons, ::remoter::ps2_button::cross);
    state.circle = ::remoter::is_held(raw.ps2_buttons, ::remoter::ps2_button::circle);
    state.triangle = ::remoter::is_held(raw.ps2_buttons, ::remoter::ps2_button::triangle);
    state.r1 = ::remoter::is_held(raw.ps2_buttons, ::remoter::ps2_button::r1);
    state.l1 = ::remoter::is_held(raw.ps2_buttons, ::remoter::ps2_button::l1);
    state.r2 = ::remoter::is_held(raw.ps2_buttons, ::remoter::ps2_button::r2);
    state.l2 = ::remoter::is_held(raw.ps2_buttons, ::remoter::ps2_button::l2);
    state.left = ::remoter::is_held(raw.ps2_buttons, ::remoter::ps2_button::left);
    state.down = ::remoter::is_held(raw.ps2_buttons, ::remoter::ps2_button::down);
    state.right = ::remoter::is_held(raw.ps2_buttons, ::remoter::ps2_button::right);
    state.up = ::remoter::is_held(raw.ps2_buttons, ::remoter::ps2_button::up);
    state.start = ::remoter::is_held(raw.ps2_buttons, ::remoter::ps2_button::start);
    state.r3 = ::remoter::is_held(raw.ps2_buttons, ::remoter::ps2_button::r3);
    state.l3 = ::remoter::is_held(raw.ps2_buttons, ::remoter::ps2_button::l3);
    state.select = ::remoter::is_held(raw.ps2_buttons, ::remoter::ps2_button::select);
    state.raw_left_x = latest_raw.raw_left_x;
    state.raw_left_y = latest_raw.raw_left_y;
    state.raw_right_x = latest_raw.raw_right_x;
    state.raw_right_y = latest_raw.raw_right_y;
    state.left_x = latest_generic.left_x;
    state.left_y = latest_generic.left_y;
    state.right_x = latest_generic.right_x;
    state.right_y = latest_generic.right_y;

    state.failure_mask = 0;
    if ((stages & source_enabled) == 0U)
    {
        state.failure_mask |= source_not_enabled;
    }
    if ((stages & service_initialized) == 0U)
    {
        state.failure_mask |= service_init_failed;
    }
    if ((stages & raw_subscriber_created) == 0U)
    {
        state.failure_mask |= raw_subscribe_failed;
    }
    if ((stages & generic_subscriber_created) == 0U)
    {
        state.failure_mask |= generic_subscribe_failed;
    }
    if ((stages & monitor_thread_started) == 0U)
    {
        state.failure_mask |= thread_create_failed;
    }
    if (startup_timed_out && (stages & raw_data_received) == 0U)
    {
        state.failure_mask |= receiver_data_timeout;
    }
    if (startup_timed_out && raw.ps2_link == ::remoter::ps2_link_state::receiver_offline)
    {
        state.failure_mask |= receiver_offline;
    }
    if ((stages & connected_frame_received) != 0U &&
        (stages & generic_data_received) != 0U && !mapping_match)
    {
        state.failure_mask |= mapping_mismatch;
    }

    state.failed_count = state.failure_mask == 0U ? 0U : 1U;
    state.passed =
        state.failure_mask == 0U &&
        latest_raw.frame_count != 0U &&
        raw.ps2_link == ::remoter::ps2_link_state::connected &&
        !latest_generic.offline &&
        latest_generic.active_source == ::remoter::source::ps2 &&
        mapping_match;
    state.passed_count = state.passed ? state.total_count : 0U;
}

void monitor_entry(ULONG /*arg*/)
{
    ::remoter::config cfg{};
    cfg.ps2.thread_priority = params::remoter::thread_priority;
    cfg.ps2.receiver_offline_timeout_ticks = params::remoter::ps2_offline_timeout_ticks;
    cfg.ps2.frame_timeout_ticks = params::remoter::ps2_frame_timeout_ticks;
    cfg.ps2.deadzone = params::remoter::ps2_deadzone;
    cfg.thread_priority = params::remoter::thread_priority + 1U;

    if (!::remoter::service::instance().init(cfg))
    {
        sync_debug(false);
        return;
    }
    stages |= service_initialized;

    raw_sub = msg::subscribe<::remoter::ps2_state>();
    if (!raw_sub.valid())
    {
        sync_debug(false);
        return;
    }
    stages |= raw_subscriber_created;

    generic_sub = msg::subscribe<::remoter::state>();
    if (!generic_sub.valid())
    {
        sync_debug(false);
        return;
    }
    stages |= generic_subscriber_created;

    for (;;)
    {
        ::remoter::ps2_state raw{};
        if (msg::read(raw_sub, raw) == types::status::ok)
        {
            record_raw_update(raw);
        }

        ::remoter::state generic{};
        if (msg::read(generic_sub, generic) == types::status::ok)
        {
            latest_generic = generic;
            stages |= generic_data_received;
        }

        sync_debug((tx_time_get() - started_at) > receiver_startup_timeout_ticks);
        tx_thread_sleep(monitor_period_ticks);
    }
}

} // namespace

void run() noexcept
{
    auto& state = demo::debug::debug_instance.ps2_unit;
    if (monitor_started)
    {
        state.started = true;
        return;
    }

    state = {};
    state.started = true;
    state.total_count = 9U;
    started_at = tx_time_get();
    latest_raw = {};
    latest_generic = {};
    stages = 0;
    raw_update_count = 0;
    connected_frame_count = 0;
    remote_disconnected_count = 0;
    receiver_offline_count = 0;
    previous_frame_tick = 0;
    previous_frame_count = 0;
    last_frame_period_ticks = 0;
    min_frame_period_ticks = 0;
    max_frame_period_ticks = 0;
    last_pressed = 0;
    last_released = 0;
    pressed_seen_mask = 0;
    released_seen_mask = 0;
    press_event_count = 0;
    release_event_count = 0;
    previous_event_count = 0;
    event_seen = false;
    previous_link = ::remoter::ps2_link_state::receiver_offline;
    link_seen = false;

    if (!::config::feature::enable_ps2)
    {
        sync_debug(false);
        return;
    }
    stages = source_enabled;

    const UINT status = tx_thread_create(&monitor_thread, const_cast<CHAR*>("ps2_demo"),
                                         monitor_entry, 0, monitor_stack,
                                         sizeof(monitor_stack),
                                         params::remoter::thread_priority + 2U,
                                         params::remoter::thread_priority + 2U,
                                         TX_NO_TIME_SLICE,
                                         TX_AUTO_START);
    if (status != TX_SUCCESS)
    {
        sync_debug(false);
        return;
    }

    monitor_started = true;
    stages |= monitor_thread_started;
    sync_debug(false);
}

} // namespace demo::ps2
