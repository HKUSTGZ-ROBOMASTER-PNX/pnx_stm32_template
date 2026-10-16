#include "bmi088.hpp"
#include "bsp_gpio.hpp"
#include "bsp_pwm.hpp"
#include "bsp_spi.hpp"
#include "tx_api.h"
#include <cmath>

namespace imu
{

static void sleep_ms(uint32_t ms)
{
    // Round up and add one tick so a call just before a tick still waits long enough.
    tx_thread_sleep((static_cast<uint64_t>(ms) * TX_TIMER_TICKS_PER_SECOND + 999U) / 1000U + 1U);
}

pnxtypes::status bmi088::write_reg(cs target, uint8_t addr, const uint8_t* data, uint8_t len)
{
    if (!transport_ready)
    {
        return pnxtypes::status::not_initialized;
    }
    const auto& pin = target == cs::acc ? wiring.acc_cs : wiring.gyro_cs;
    auto result = bsp::gpio::set_active(pin, true);
    const uint8_t command = addr & spi_write;
    if (result == pnxtypes::status::ok)
    {
        result = bsp::spi::transmit(wiring.spi, &command, 1, spi_timeout_ms);
    }
    if (result == pnxtypes::status::ok)
    {
        result = bsp::spi::transmit(wiring.spi, data, len, spi_timeout_ms);
    }
    const auto released = bsp::gpio::set_active(pin, false);
    return result == pnxtypes::status::ok ? released : result;
}

pnxtypes::status bmi088::read_reg(cs target, uint8_t addr, uint8_t* data, uint8_t len)
{
    if (!transport_ready)
    {
        return pnxtypes::status::not_initialized;
    }
    const auto& pin = target == cs::acc ? wiring.acc_cs : wiring.gyro_cs;
    auto result = bsp::gpio::set_active(pin, true);
    const uint8_t command = addr | spi_read;
    if (result == pnxtypes::status::ok)
    {
        result = bsp::spi::transmit(wiring.spi, &command, 1, spi_timeout_ms);
    }
    if (result == pnxtypes::status::ok)
    {
        // Accelerometer callers include its dummy byte in len; gyro has no dummy byte.
        result = bsp::spi::receive(wiring.spi, data, len, spi_timeout_ms);
    }
    const auto released = bsp::gpio::set_active(pin, false);
    return result == pnxtypes::status::ok ? released : result;
}

pnxtypes::status bmi088::verify_acc_chip_id()
{
    uint8_t data[2]{};
    const auto result = read_reg(cs::acc, acc_chip_id_addr, data, sizeof(data));
    if (result != pnxtypes::status::ok)
    {
        return result;
    }
    return data[1] != acc_chip_id_val ? pnxtypes::status::error : pnxtypes::status::ok;
}

pnxtypes::status bmi088::verify_gyro_chip_id()
{
    uint8_t data{};
    const auto result = read_reg(cs::gyro, gyro_chip_id_addr, &data, 1);
    if (result != pnxtypes::status::ok)
    {
        return result;
    }
    return data != gyro_chip_id_val ? pnxtypes::status::error : pnxtypes::status::ok;
}

pnxtypes::status bmi088::verify_acc_data()
{
    math::vector3 sample{};
    return read_acc(&sample);
}

pnxtypes::status bmi088::verify_gyro_data()
{
    math::vector3 sample{};
    return read_gyro(&sample);
}

pnxtypes::status bmi088::configure()
{
    initialized = false;
    transport_ready = false;
    auto result = stop_heater();
    if (result != pnxtypes::status::ok)
    {
        return result;
    }
    if (wiring.spi == bsp::spi::handle::none || wiring.acc_cs.instance == bsp::gpio::port::none ||
        wiring.gyro_cs.instance == bsp::gpio::port::none ||
        wiring.acc_cs.active != bsp::active_level::low ||
        wiring.gyro_cs.active != bsp::active_level::low ||
        (wiring.acc_cs.instance == wiring.gyro_cs.instance &&
            wiring.acc_cs.pin == wiring.gyro_cs.pin))
    {
        return pnxtypes::status::invalid_arg;
    }
    result = bsp::gpio::set_active(wiring.acc_cs, false);
    if (result != pnxtypes::status::ok)
    {
        return result;
    }
    result = bsp::gpio::set_active(wiring.gyro_cs, false);
    if (result != pnxtypes::status::ok)
    {
        return result;
    }
    result = bsp::spi::wait_ready(wiring.spi, spi_timeout_ms);
    if (result != pnxtypes::status::ok)
    {
        return result;
    }
    transport_ready = true;
    sleep_ms(10);

    // The accelerometer powers up in I2C mode; a dummy read selects SPI.
    uint8_t dummy[2]{};
    result = read_reg(cs::acc, acc_chip_id_addr, dummy, sizeof(dummy));
    if (result != pnxtypes::status::ok)
    {
        return result;
    }
    uint8_t reset = acc_soft_reset_val;
    result = write_reg(cs::acc, acc_soft_reset_addr, &reset, 1);
    if (result != pnxtypes::status::ok)
    {
        return result;
    }
    sleep_ms(100);
    result = read_reg(cs::acc, acc_chip_id_addr, dummy, sizeof(dummy));
    if (result != pnxtypes::status::ok)
    {
        return result;
    }
    sleep_ms(1);
    result = verify_acc_chip_id();
    if (result != pnxtypes::status::ok)
    {
        return result;
    }
    reset = gyro_soft_reset_val;
    result = write_reg(cs::gyro, gyro_soft_reset_addr, &reset, 1);
    if (result != pnxtypes::status::ok)
    {
        return result;
    }
    sleep_ms(100);
    result = verify_gyro_chip_id();
    if (result != pnxtypes::status::ok)
    {
        return result;
    }

    struct setting
    {
        cs target;
        uint8_t address;
        uint8_t value;
    };

    // Polling acquisition: do not enable DRDY interrupts or depend on board EXTI polarity.
    const setting settings[]{
        {cs::acc, acc_pwr_conf_addr, acc_pwr_conf_act},
        {cs::acc, acc_pwr_ctrl_addr, acc_pwr_ctrl_on},
        {cs::acc, acc_range_addr, acc_range_6g},
        {cs::acc, acc_conf_addr, 0xAB},
        {cs::gyro, gyro_range_addr, gyro_range_1000},
        {cs::gyro, gyro_bandwidth_addr, 0x81},
        {cs::gyro, gyro_lpm1_addr, gyro_lpm1_normal},
    };
    for (const auto& item : settings)
    {
        result = write_reg(item.target, item.address, &item.value, 1);
        if (result != pnxtypes::status::ok)
        {
            return result;
        }
        sleep_ms(10);
        uint8_t actual[2]{};
        const uint8_t index = item.target == cs::acc ? 1 : 0;
        result = read_reg(item.target, item.address, actual, index + 1);
        if (result != pnxtypes::status::ok)
        {
            return result;
        }
        if (actual[index] != item.value)
        {
            return pnxtypes::status::error;
        }
    }
    initialized = true;
    result = verify_acc_data();
    if (result == pnxtypes::status::ok)
    {
        result = verify_gyro_data();
    }
    initialized = result == pnxtypes::status::ok;
    return result;
}

pnxtypes::status bmi088::stop_heater()
{
    heat_state = heat_phase::boost;
    heater_duty = 0.0f;
    temperature_pid.clear();
    if (wiring.heater_timer == bsp::pwm::handle::none &&
        wiring.heater_channel == bsp::pwm::channel::none)
    {
        return pnxtypes::status::ok;
    }
    const auto zero = bsp::pwm::set_duty(wiring.heater_timer, wiring.heater_channel, 0.0f);
    const auto stopped = bsp::pwm::stop(wiring.heater_timer, wiring.heater_channel);
    if (stopped == pnxtypes::status::ok)
    {
        heater_started = false;
    }
    return zero != pnxtypes::status::ok ? zero : stopped;
}

pnxtypes::status bmi088::shutdown()
{
    const auto result = stop_heater();
    const auto acc_result = bsp::gpio::set_active(wiring.acc_cs, false);
    const auto gyro_result = bsp::gpio::set_active(wiring.gyro_cs, false);
    transport_ready = false;
    initialized = false;
    if (result != pnxtypes::status::ok)
    {
        return result;
    }
    return acc_result != pnxtypes::status::ok ? acc_result : gyro_result;
}

pnxtypes::status bmi088::temperature_control(float temp)
{
    if (!initialized)
    {
        return pnxtypes::status::not_initialized;
    }
    if (wiring.heater_timer == bsp::pwm::handle::none)
    {
        return pnxtypes::status::not_configured;
    }
    if (!std::isfinite(temp) || temp < -20.0f || temp > 85.0f || !std::isfinite(thermal.target) ||
        thermal.target < 20.0f || thermal.target > 60.0f)
    {
        const auto stopped = stop_heater();
        return stopped == pnxtypes::status::ok ? pnxtypes::status::invalid_arg : stopped;
    }
    const float limits[]{thermal.boost, thermal.reboost, thermal.approach_start,
        thermal.approach_end, thermal.hold_max};
    for (float duty : limits)
    {
        if (!std::isfinite(duty) || duty < 0.0f || duty > 1.0f)
        {
            const auto stopped = stop_heater();
            return stopped == pnxtypes::status::ok ? pnxtypes::status::invalid_arg : stopped;
        }
    }
    const auto previous = heat_state;
    const float approach =
        thermal.approach_end + (thermal.approach_start - thermal.approach_end) *
                                   math::clamp((thermal.target - 0.1f - temp) / 3.9f, 0.0f, 1.0f);
    switch (heat_state)
    {
        case heat_phase::boost:
            if (temp < thermal.target - 4.0f)
            {
                heater_duty = thermal.boost;
            }
            else
            {
                heat_state = heat_phase::approach;
                heater_duty = approach;
            }
            break;
        case heat_phase::approach:
        case heat_phase::hold:
            if (temp >= thermal.target + 0.2f)
            {
                heat_state = heat_phase::overtemperature;
                heater_duty = 0.0f;
            }
            else if (temp < thermal.target - 4.0f)
            {
                heat_state = heat_phase::boost;
                heater_duty = thermal.reboost;
            }
            else if (heat_state == heat_phase::hold)
            {
                temperature_pid.ref = thermal.target;
                temperature_pid.fdb = temp;
                temperature_pid.update();
                heater_duty = math::clamp(temperature_pid.result / 999.0f, 0.0f, thermal.hold_max);
            }
            else if (temp >= thermal.target - 0.1f)
            {
                heat_state = heat_phase::hold;
            }
            else
            {
                heater_duty = approach;
            }
            break;
        case heat_phase::overtemperature:
            heater_duty = 0.0f;
            if (temp <= thermal.target - 0.2f)
            {
                heat_state = heat_phase::approach;
            }
            break;
    }
    // Also stop immediately when a first call arrives with an already-hot sensor.
    if (temp >= thermal.target + 0.2f)
    {
        heat_state = heat_phase::overtemperature;
        heater_duty = 0.0f;
    }
    if (previous != heat_state &&
        (previous == heat_phase::hold || heat_state == heat_phase::overtemperature))
    {
        temperature_pid.reset_state(thermal.target, temp);
    }
    auto result = bsp::pwm::set_duty(wiring.heater_timer, wiring.heater_channel, heater_duty);
    if (result == pnxtypes::status::ok && !heater_started)
    {
        result = bsp::pwm::start(wiring.heater_timer, wiring.heater_channel);
        heater_started = result == pnxtypes::status::ok;
    }
    if (result != pnxtypes::status::ok)
    {
        const auto stopped = stop_heater();
        return stopped == pnxtypes::status::ok ? result : stopped;
    }
    return result;
}

pnxtypes::status bmi088::calibrate()
{
    if (!initialized)
    {
        return pnxtypes::status::not_initialized;
    }
    const auto stopped = stop_heater();
    if (stopped != pnxtypes::status::ok)
    {
        return stopped;
    }
    const auto saved_offset = gyro_offset;
    constexpr int calib_samples = calibration_samples;
    constexpr int calib_max_attempts = calibration_samples;
    constexpr float gravity = 9.80665f;
    constexpr float max_gyro_norm = 0.15f;
    constexpr float max_accel_norm_error = 0.6f;
    constexpr float max_gyro_mean_norm = 0.03f;
    constexpr float max_gyro_stddev = 0.03f;
    constexpr float max_accel_norm_stddev = 0.15f;
    float gyro_sum[3] = {0.0f, 0.0f, 0.0f};
    float gyro_sq_sum[3] = {0.0f, 0.0f, 0.0f};
    float accel_norm_sum = 0.0f;
    float accel_norm_sq_sum = 0.0f;
    int sample_count = 0;
    math::vector3 gyro_sample{};
    math::vector3 accel_sample{};

    gyro_offset.x = 0.0f;
    gyro_offset.y = 0.0f;
    gyro_offset.z = 0.0f;

    for (int attempt = 0; attempt < calib_max_attempts && sample_count < calib_samples; ++attempt)
    {
        const auto gyro_result = read_gyro(&gyro_sample);
        const auto acc_result =
            gyro_result == pnxtypes::status::ok ? read_acc(&accel_sample) : gyro_result;
        if (acc_result != pnxtypes::status::ok)
        {
            gyro_offset = saved_offset;
            return acc_result;
        }
        const float gyro_norm =
            std::sqrt(gyro_sample.x * gyro_sample.x + gyro_sample.y * gyro_sample.y +
                      gyro_sample.z * gyro_sample.z);
        const float accel_norm =
            std::sqrt(accel_sample.x * accel_sample.x + accel_sample.y * accel_sample.y +
                      accel_sample.z * accel_sample.z);
        if (!std::isfinite(gyro_norm) || !std::isfinite(accel_norm) || gyro_norm > max_gyro_norm ||
            std::fabs(accel_norm - gravity) > max_accel_norm_error)
        {
            break;
        }
        gyro_sum[0] += gyro_sample.x;
        gyro_sum[1] += gyro_sample.y;
        gyro_sum[2] += gyro_sample.z;
        gyro_sq_sum[0] += gyro_sample.x * gyro_sample.x;
        gyro_sq_sum[1] += gyro_sample.y * gyro_sample.y;
        gyro_sq_sum[2] += gyro_sample.z * gyro_sample.z;
        accel_norm_sum += accel_norm;
        accel_norm_sq_sum += accel_norm * accel_norm;
        ++sample_count;
        sleep_ms(1);
    }

    bool stationary = sample_count == calib_samples;
    if (stationary)
    {
        const float inv_count = 1.0f / static_cast<float>(sample_count);
        float gyro_mean[3]{};
        for (int axis = 0; axis < 3; ++axis)
        {
            gyro_mean[axis] = gyro_sum[axis] * inv_count;
            const float variance =
                std::fmax(0.0f, gyro_sq_sum[axis] * inv_count - gyro_mean[axis] * gyro_mean[axis]);
            stationary = stationary && std::sqrt(variance) <= max_gyro_stddev;
        }
        const float gyro_mean_norm =
            std::sqrt(gyro_mean[0] * gyro_mean[0] + gyro_mean[1] * gyro_mean[1] +
                      gyro_mean[2] * gyro_mean[2]);
        const float accel_mean = accel_norm_sum * inv_count;
        const float accel_variance =
            std::fmax(0.0f, accel_norm_sq_sum * inv_count - accel_mean * accel_mean);
        stationary = stationary && gyro_mean_norm <= max_gyro_mean_norm &&
                     std::sqrt(accel_variance) <= max_accel_norm_stddev;
        if (stationary)
        {
            gyro_offset.x = gyro_mean[0];
            gyro_offset.y = gyro_mean[1];
            gyro_offset.z = gyro_mean[2];
        }
    }

    if (!stationary || std::fabs(gyro_offset.x) > 0.1f || std::fabs(gyro_offset.y) > 0.1f ||
        std::fabs(gyro_offset.z) > 0.1f)
    {
        gyro_offset = saved_offset;
        return pnxtypes::status::error;
    }
    return pnxtypes::status::ok;
}

pnxtypes::status bmi088::read_acc(math::vector3* data)
{
    if (data == nullptr)
    {
        return pnxtypes::status::invalid_arg;
    }

    if (!initialized)
    {
        return pnxtypes::status::not_initialized;
    }

    uint8_t buf[acc_xyz_len + 1] = {0};
    int16_t raw[3] = {0};
    const auto status = read_reg(cs::acc, acc_xyz_addr, buf, acc_xyz_len + 1);
    if (status != pnxtypes::status::ok)
    {
        return status;
    }

    raw[0] = static_cast<int16_t>((buf[2] << 8) | buf[1]);
    raw[1] = static_cast<int16_t>((buf[4] << 8) | buf[3]);
    raw[2] = static_cast<int16_t>((buf[6] << 8) | buf[5]);

    data->x = static_cast<float>(raw[0]) * accel_6g_sensitivity + accel_offset.x;
    data->y = static_cast<float>(raw[1]) * accel_6g_sensitivity + accel_offset.y;
    data->z = static_cast<float>(raw[2]) * accel_6g_sensitivity + accel_offset.z;
    return pnxtypes::status::ok;
}

pnxtypes::status bmi088::read_gyro(math::vector3* data)
{
    if (data == nullptr)
    {
        return pnxtypes::status::invalid_arg;
    }

    if (!initialized)
    {
        return pnxtypes::status::not_initialized;
    }

    uint8_t buf[gyro_xyz_len] = {0};
    int16_t raw[3] = {0};
    const auto status = read_reg(cs::gyro, gyro_xyz_addr, buf, gyro_xyz_len);
    if (status != pnxtypes::status::ok)
    {
        return status;
    }

    raw[0] = static_cast<int16_t>((buf[1] << 8) | buf[0]);
    raw[1] = static_cast<int16_t>((buf[3] << 8) | buf[2]);
    raw[2] = static_cast<int16_t>((buf[5] << 8) | buf[4]);

    data->x = static_cast<float>(raw[0]) * gyro_1000_sensitivity - gyro_offset.x;
    data->y = static_cast<float>(raw[1]) * gyro_1000_sensitivity - gyro_offset.y;
    data->z = static_cast<float>(raw[2]) * gyro_1000_sensitivity - gyro_offset.z;
    return pnxtypes::status::ok;
}

pnxtypes::status bmi088::read_temperature(float* temp)
{
    if (temp == nullptr)
    {
        return pnxtypes::status::invalid_arg;
    }

    if (!initialized)
    {
        return pnxtypes::status::not_initialized;
    }

    uint8_t buf[temp_len + 1] = {0};
    const auto status = read_reg(cs::acc, temp_addr, buf, temp_len + 1);
    if (status != pnxtypes::status::ok)
    {
        const auto stopped = stop_heater();
        return stopped == pnxtypes::status::ok ? status : stopped;
    }
    const uint16_t temp_uint11 = static_cast<uint16_t>((buf[1] << 3) | (buf[2] >> 5));
    int16_t temp_int11 = static_cast<int16_t>(temp_uint11);
    if (temp_uint11 > 1023)
    {
        temp_int11 = static_cast<int16_t>(temp_uint11 - 2048);
    }
    *temp = static_cast<float>(temp_int11) * temp_unit + temp_bias;

    return pnxtypes::status::ok;
}

} // namespace imu
