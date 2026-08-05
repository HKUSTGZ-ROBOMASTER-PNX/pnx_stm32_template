if(NOT DEFINED PNX_SOURCE_DIR OR NOT DEFINED PNX_BINARY_DIR)
    message(FATAL_ERROR
        "config_generation_contract requires PNX_SOURCE_DIR and PNX_BINARY_DIR")
endif()

set(contract_out "${PNX_BINARY_DIR}/f407-config-contract")
file(MAKE_DIRECTORY "${contract_out}")

execute_process(
    COMMAND "${CMAKE_COMMAND}"
        "-DIOC=${PNX_SOURCE_DIR}/boards/dji_c_board_f407/dji_c_board_f407.ioc"
        "-DPARAMS=${PNX_SOURCE_DIR}/configs/boards/dji_c_board_f407/params.json"
        "-DROBOT_CONFIG=${PNX_SOURCE_DIR}/configs/boards/dji_c_board_f407/robot.json"
        "-DOUT_DIR=${contract_out}"
        -DPNX_ENABLE_USB_CDC=OFF
        -P "${PNX_SOURCE_DIR}/configs/cmake/generate_config.cmake"
    RESULT_VARIABLE generation_result
    OUTPUT_VARIABLE generation_output
    ERROR_VARIABLE generation_error
)
if(NOT generation_result EQUAL 0)
    message(FATAL_ERROR
        "F407 config generation failed:\n${generation_output}\n${generation_error}")
endif()

file(READ "${contract_out}/config.hpp" generated_config)
file(READ "${contract_out}/robot_config.hpp" generated_robot_config)

foreach(required_text IN ITEMS
        "inline constexpr bool enable_ps2 = 0"
        "inline constexpr bsp::usart::port ps2 = bsp::usart::none"
        "inline constexpr std::uint32_t ps2_offline_timeout_ticks = 600"
        "inline constexpr std::uint32_t ps2_frame_timeout_ticks = 20"
        "inline constexpr float ps2_deadzone = 0.08f")
    string(FIND "${generated_config}" "${required_text}" required_index)
    if(required_index EQUAL -1)
        message(FATAL_ERROR
            "Generated F407 config is missing: ${required_text}")
    endif()
endforeach()

foreach(required_text IN ITEMS "lk_lk8016" "lk_lk9025")
    string(FIND "${generated_robot_config}" "${required_text}" required_index)
    if(required_index EQUAL -1)
        message(FATAL_ERROR
            "Generated F407 robot config is missing: ${required_text}")
    endif()
endforeach()

set(ps2_contract_out "${PNX_BINARY_DIR}/f407-ps2-config-contract")
file(MAKE_DIRECTORY "${ps2_contract_out}")

execute_process(
    COMMAND "${CMAKE_COMMAND}"
        "-DIOC=${PNX_SOURCE_DIR}/boards/dji_c_board_f407/dji_c_board_f407.ioc"
        "-DPARAMS=${PNX_SOURCE_DIR}/configs/boards/dji_c_board_f407/params.ps2.json"
        "-DROBOT_CONFIG=${PNX_SOURCE_DIR}/configs/boards/dji_c_board_f407/robot.json"
        "-DOUT_DIR=${ps2_contract_out}"
        -DPNX_ENABLE_USB_CDC=OFF
        -DPNX_ENABLE_PWM_A2=OFF
        -DPNX_ENABLE_PS2_VALIDATION=ON
        -P "${PNX_SOURCE_DIR}/configs/cmake/generate_config.cmake"
    RESULT_VARIABLE ps2_generation_result
    OUTPUT_VARIABLE ps2_generation_output
    ERROR_VARIABLE ps2_generation_error
)
if(NOT ps2_generation_result EQUAL 0)
    message(FATAL_ERROR
        "F407 PS2 config generation failed:\n"
        "${ps2_generation_output}\n${ps2_generation_error}")
endif()

file(READ "${ps2_contract_out}/config.hpp" generated_ps2_config)

foreach(required_text IN ITEMS
        "inline constexpr bool enable_ps2 = 1"
        "inline constexpr bsp::usart::port ps2 = usart1"
        "inline constexpr std::uint32_t ps2_offline_timeout_ticks = 600"
        "inline constexpr std::uint32_t ps2_frame_timeout_ticks = 20")
    string(FIND "${generated_ps2_config}" "${required_text}" required_index)
    if(required_index EQUAL -1)
        message(FATAL_ERROR
            "Generated F407 PS2 config is missing: ${required_text}")
    endif()
endforeach()

if(NOT generated_ps2_config MATCHES
   "inline constexpr float ps2_deadzone = 0\\.0*8[0-9]*f")
    message(FATAL_ERROR
        "Generated F407 PS2 config has the wrong deadzone")
endif()

file(READ
    "${PNX_SOURCE_DIR}/configs/boards/dji_c_board_f407/params.ps2.json"
    ps2_params)

string(REPLACE
    "\"report_uart\": \"usart6\""
    "\"report_uart\": \"usart1\""
    conflicting_ps2_params
    "${ps2_params}")
if(conflicting_ps2_params STREQUAL ps2_params)
    message(FATAL_ERROR "PS2 conflict fixture no longer matches params.ps2.json")
endif()
set(conflicting_ps2_params_path
    "${ps2_contract_out}/params.ps2-uart-conflict.json")
file(WRITE "${conflicting_ps2_params_path}" "${conflicting_ps2_params}")

execute_process(
    COMMAND "${CMAKE_COMMAND}"
        "-DIOC=${PNX_SOURCE_DIR}/boards/dji_c_board_f407/dji_c_board_f407.ioc"
        "-DPARAMS=${conflicting_ps2_params_path}"
        "-DROBOT_CONFIG=${PNX_SOURCE_DIR}/configs/boards/dji_c_board_f407/robot.json"
        "-DOUT_DIR=${ps2_contract_out}/uart-conflict"
        -DPNX_ENABLE_PS2_VALIDATION=ON
        -P "${PNX_SOURCE_DIR}/configs/cmake/generate_config.cmake"
    RESULT_VARIABLE conflict_result
    OUTPUT_VARIABLE conflict_output
    ERROR_VARIABLE conflict_error
)
if(conflict_result EQUAL 0)
    message(FATAL_ERROR
        "F407 PS2 config accepted a report/remoter UART conflict")
endif()
set(conflict_log "${conflict_output}\n${conflict_error}")
if(NOT conflict_log MATCHES "conflicts with the active remoter UART")
    message(FATAL_ERROR
        "F407 PS2 UART conflict failed for the wrong reason:\n${conflict_log}")
endif()

file(READ
    "${PNX_SOURCE_DIR}/boards/dji_c_board_f407/dji_c_board_f407.ioc"
    missing_dma_ps2_ioc)
string(REPLACE "\r\n" "\n" missing_dma_ps2_ioc "${missing_dma_ps2_ioc}")
string(REPLACE
    "Dma.Request0=USART1_RX\n"
    ""
    missing_dma_ps2_ioc
    "${missing_dma_ps2_ioc}")
if(missing_dma_ps2_ioc MATCHES "Dma\\.Request[0-9]+=USART1_RX")
    message(FATAL_ERROR "PS2 RX-DMA fixture still contains USART1_RX DMA")
endif()
set(missing_dma_ps2_ioc_path
    "${ps2_contract_out}/dji_c_board_f407-no-usart1-rx-dma.ioc")
file(WRITE "${missing_dma_ps2_ioc_path}" "${missing_dma_ps2_ioc}")

execute_process(
    COMMAND "${CMAKE_COMMAND}"
        "-DIOC=${missing_dma_ps2_ioc_path}"
        "-DPARAMS=${PNX_SOURCE_DIR}/configs/boards/dji_c_board_f407/params.ps2.json"
        "-DROBOT_CONFIG=${PNX_SOURCE_DIR}/configs/boards/dji_c_board_f407/robot.json"
        "-DOUT_DIR=${ps2_contract_out}/no-rx-dma"
        -DPNX_ENABLE_PS2_VALIDATION=ON
        -P "${PNX_SOURCE_DIR}/configs/cmake/generate_config.cmake"
    RESULT_VARIABLE missing_dma_result
    OUTPUT_VARIABLE missing_dma_output
    ERROR_VARIABLE missing_dma_error
)
if(missing_dma_result EQUAL 0)
    message(FATAL_ERROR
        "F407 PS2 config accepted a remoter UART without RX DMA")
endif()
set(missing_dma_log "${missing_dma_output}\n${missing_dma_error}")
if(NOT missing_dma_log MATCHES "remoter|RX DMA")
    message(FATAL_ERROR
        "F407 PS2 RX-DMA rejection failed for the wrong reason:\n${missing_dma_log}")
endif()
