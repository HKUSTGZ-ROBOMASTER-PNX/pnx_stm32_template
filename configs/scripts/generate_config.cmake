cmake_minimum_required(VERSION 3.22)

# Also usable without configuring or building firmware:
# cmake -P configs/scripts/generate_config.cmake
get_filename_component(_config_dir "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
file(READ "${_config_dir}/user_config.json" _json)

function(config_number out min max integer)
    string(JSON value GET "${_json}" ${ARGN})
    string(JSON type TYPE "${_json}" ${ARGN})
    if(NOT type STREQUAL "NUMBER" OR value LESS min OR value GREATER max)
        message(FATAL_ERROR "${ARGN}: expected a number in [${min}, ${max}]")
    endif()
    if(integer AND NOT value MATCHES "^[0-9]+$")
        message(FATAL_ERROR "${ARGN}: expected an integer")
    endif()
    set(${out} "${value}" PARENT_SCOPE)
endfunction()

config_number(_ui_timeout 0 2147483647 TRUE ui timeout_ms)
config_number(_half_period 1 1000000 TRUE ps2 half_period_us)
config_number(_cs_setup 0 1000000 TRUE ps2 cs_setup_us)
config_number(_cs_hold 0 1000000 TRUE ps2 cs_hold_us)
config_number(_deadzone 0 1 FALSE ps2 deadzone)
if(_deadzone GREATER_EQUAL 1)
    message(FATAL_ERROR "ps2.deadzone must be less than 1")
endif()
if(NOT _deadzone MATCHES "[.eE]")
    string(APPEND _deadzone ".0")
endif()
config_number(_spi_timeout 1 2147483647 TRUE ps2 spi_timeout_ms)

set(_header "#pragma once\n\n// Generated from configs/user_config.json. Do not edit.\n#include \"config.hpp\"\n#include \"ui.hpp\"\n#include \"ps2.hpp\"\n\nnamespace user_config\n{\n")
foreach(_board IN ITEMS h723_mc02 f407_cboard)
    if(_board STREQUAL "h723_mc02")
        string(APPEND _header "#if defined(STM32H723xx)\n")
    else()
        string(APPEND _header "#elif defined(STM32F407xx)\n")
    endif()
    set(_board_header "${_config_dir}/../boards/wrapper/${_board}/include/config.hpp")
    file(READ "${_board_header}" _resources)
    foreach(_key IN ITEMS ui_uart dr16_uart vt03_uart ps2_cmd ps2_data ps2_clk ps2_cs ps2_spi)
        string(JSON _value GET "${_json}" boards ${_board} ${_key})
        string(JSON _type TYPE "${_json}" boards ${_board} ${_key})
        if(NOT _type STREQUAL "STRING" OR NOT _value MATCHES "^[a-z][a-z0-9_]*$")
            message(FATAL_ERROR "boards.${_board}.${_key}: expected a resource name")
        endif()
        if(_key MATCHES "_uart$")
            set(_kind usart)
        elseif(_key STREQUAL "ps2_spi")
            set(_kind spi)
        else()
            set(_kind gpio)
        endif()
        if(NOT _value STREQUAL "none" AND
           NOT _resources MATCHES "inline constexpr ${_kind}::config ${_value}\\{")
            message(FATAL_ERROR "boards.${_board}.${_key}: unknown ${_kind} resource '${_value}'")
        endif()
        if(_kind STREQUAL "usart")
            if(_value STREQUAL "none")
                string(APPEND _header "inline constexpr bsp::usart::config ${_key}{};\n")
            else()
                string(APPEND _header "inline constexpr auto ${_key} = bsp::peripherals::${_value};\n")
            endif()
        elseif(_kind STREQUAL "spi")
            string(APPEND _header "inline constexpr auto ${_key} = bsp::spi::handle::${_value};\n")
        elseif(_value STREQUAL "none")
            if(_key STREQUAL "ps2_cs")
                string(APPEND _header "inline constexpr bsp::gpio::config ps2_cs{bsp::gpio::port::none, 0, bsp::active_level::low};\n")
            else()
                string(APPEND _header "inline constexpr bsp::gpio::config ${_key}{};\n")
            endif()
        elseif(_key STREQUAL "ps2_cs")
            string(APPEND _header "inline constexpr bsp::gpio::config ps2_cs{bsp::peripherals::${_value}.instance,\n    bsp::peripherals::${_value}.pin, bsp::active_level::low};\n")
        else()
            string(APPEND _header "inline constexpr auto ${_key} = bsp::peripherals::${_value};\n")
        endif()
    endforeach()
endforeach()
string(APPEND _header "#else\n#error Unsupported board\n#endif\n\n")
string(APPEND _header "inline constexpr ui::config ui{ui_uart.instance, ${_ui_timeout}};\n")
# Wire formats belong to the protocols. HAL counts the DR16 parity bit in its word length.
string(APPEND _header [=[
inline constexpr bsp::usart::line_config dr16_line{100000, bsp::usart::word_length::bits_9,
    bsp::usart::stop_bits::one, bsp::usart::parity::even, false, true};
inline constexpr bsp::usart::line_config vt03_line{115200, bsp::usart::word_length::bits_8,
    bsp::usart::stop_bits::one, bsp::usart::parity::none, false, true};
]=])
string(APPEND _header "inline constexpr remotes::ps2::gpio_timing ps2_timing{${_half_period}, ${_cs_setup}, ${_cs_hold}};\ninline constexpr float ps2_deadzone = ${_deadzone}f;\ninline constexpr uint32_t ps2_spi_timeout_ms = ${_spi_timeout};\n} // namespace user_config\n")
# All board branches share one checked-in header; switching build directories cannot overwrite
# another board's configuration. file(CONFIGURE) preserves timestamps when content is unchanged.
file(CONFIGURE OUTPUT "${_config_dir}/include/user_config.hpp" CONTENT "${_header}" @ONLY)

if(NOT CMAKE_SCRIPT_MODE_FILE)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${_config_dir}/user_config.json"
        "${_config_dir}/../boards/wrapper/h723_mc02/include/config.hpp"
        "${_config_dir}/../boards/wrapper/f407_cboard/include/config.hpp"
    )
    add_library(app_config INTERFACE)
    target_include_directories(app_config INTERFACE "${_config_dir}/include")
    target_link_libraries(app_config INTERFACE devices)
endif()
