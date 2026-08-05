cmake_minimum_required(VERSION 3.22)

option(PNX_HOST_TESTS "Build host-only shared-logic tests" OFF)
if(PNX_HOST_TESTS)
    project(pnx_host_tests LANGUAGES CXX)
    if(CMAKE_CROSSCOMPILING)
        message(FATAL_ERROR
            "PNX_HOST_TESTS requires a native compiler")
    endif()
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    enable_testing()
    add_subdirectory(tests/host)
    return()
endif()

option(PNX_ENABLE_USB_CDC "Build the additive F407 USB CDC closure" OFF)
set(PNX_F407_USB_ENABLED "${PNX_ENABLE_USB_CDC}")
option(PNX_ENABLE_PWM_A2
    "Build the attended F407 TIM1/PE11 servo vertical slice" OFF)
option(PNX_ENABLE_PS2_VALIDATION
    "Build the attended F407 PS2 remoter validation closure" OFF)

foreach(PNX_RETIRED_VALIDATION_OPTION
        PNX_ENABLE_CAN_M2006_VALIDATION
        PNX_ENABLE_BMI088
        PNX_ENABLE_DBUS_RX)
    if(DEFINED ${PNX_RETIRED_VALIDATION_OPTION} AND
       ${PNX_RETIRED_VALIDATION_OPTION})
        message(FATAL_ERROR
            "${PNX_RETIRED_VALIDATION_OPTION} was retired when "
            "pnx_devices/pnx_libs/pnx_modules returned to the pnx_template "
            "gitlinks; this repository no longer claims that validation image")
    endif()
endforeach()

set(PNX_SELECTED_OPTIONAL_APPLICATIONS 0)
foreach(PNX_APPLICATION_OPTION
        PNX_ENABLE_USB_CDC PNX_ENABLE_PWM_A2 PNX_ENABLE_PS2_VALIDATION)
    if(${PNX_APPLICATION_OPTION})
        math(EXPR PNX_SELECTED_OPTIONAL_APPLICATIONS
            "${PNX_SELECTED_OPTIONAL_APPLICATIONS}+1")
    endif()
endforeach()
if(PNX_SELECTED_OPTIONAL_APPLICATIONS GREATER 1)
    message(FATAL_ERROR
        "USB CDC, PWM/A2, and PS2 validation select different applications")
endif()

option(PNX_USB_DEVICE_IDENTITY_CONFIRMED
    "Confirm that the configured USB VID/PID identity is authorized" OFF)
set(PNX_USB_DEVICE_VID "0x0000" CACHE STRING
    "Authorized USB vendor ID; 0x0000 is fail-closed")
set(PNX_USB_DEVICE_PID "0x0000" CACHE STRING
    "Authorized USB product ID; 0x0000 is fail-closed")
set(PNX_USB_DEVICE_MANUFACTURER "PnX Development" CACHE STRING
    "USB manufacturer string")
set(PNX_USB_DEVICE_PRODUCT "PnX F407 CDC (unassigned)" CACHE STRING
    "USB product string")
set(PNX_USB_DEVICE_SERIAL "UNASSIGNED" CACHE STRING
    "Stable USB serial string")

if(PNX_USB_DEVICE_IDENTITY_CONFIRMED)
    if(NOT PNX_ENABLE_USB_CDC)
        message(FATAL_ERROR
            "USB identity confirmation requires PNX_ENABLE_USB_CDC")
    endif()
    if(NOT PNX_USB_DEVICE_VID MATCHES
       "^(0[xX][0-9A-Fa-f]+|[0-9]+)$" OR
       NOT PNX_USB_DEVICE_PID MATCHES
       "^(0[xX][0-9A-Fa-f]+|[0-9]+)$")
        message(FATAL_ERROR "USB VID and PID must be integer literals")
    endif()
    math(EXPR PNX_USB_DEVICE_VID_VALUE "${PNX_USB_DEVICE_VID}")
    math(EXPR PNX_USB_DEVICE_PID_VALUE "${PNX_USB_DEVICE_PID}")
    if(PNX_USB_DEVICE_VID_VALUE EQUAL 0 OR
       PNX_USB_DEVICE_PID_VALUE EQUAL 0)
        message(FATAL_ERROR
            "Confirmed USB identity requires non-zero VID and PID")
    endif()
    if(PNX_USB_DEVICE_SERIAL STREQUAL "" OR
       PNX_USB_DEVICE_SERIAL STREQUAL "UNASSIGNED")
        message(FATAL_ERROR
            "Confirmed USB identity requires an assigned serial")
    endif()
    set(PNX_USB_DEVICE_IDENTITY_CONFIRMED_VALUE 1)
else()
    set(PNX_USB_DEVICE_VID_VALUE 0)
    set(PNX_USB_DEVICE_PID_VALUE 0)
    set(PNX_USB_DEVICE_IDENTITY_CONFIRMED_VALUE 0)
endif()

set(PNX_BOARD_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/boards/dji_c_board_f407")
set(PNX_BSP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/pnx_bsp")
set(PNX_BOARD_IOC
    "${PNX_BOARD_ROOT}/dji_c_board_f407.ioc")
set(PNX_BOARD_PARAMS_DEFAULT
    "${CMAKE_CURRENT_SOURCE_DIR}/configs/boards/dji_c_board_f407/params.json")
set(PNX_BOARD_PARAMS
    "${PNX_BOARD_PARAMS_DEFAULT}" CACHE FILEPATH
    "Board params JSON used for config generation")
set(PNX_BOARD_ROBOT_CONFIG
    "${CMAKE_CURRENT_SOURCE_DIR}/configs/boards/dji_c_board_f407/robot.json")
set(PNX_BOARD_CUBEMX_DIR
    "${PNX_BOARD_ROOT}/cmake/stm32cubemx")

if(NOT CMAKE_TOOLCHAIN_FILE)
    set(PNX_EMBEDDED_COMPILER_HINT
        "${CMAKE_C_COMPILER};${CMAKE_CXX_COMPILER}")
    if(PNX_EMBEDDED_COMPILER_HINT MATCHES "arm-none-eabi")
        include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/gcc-arm-none-eabi.cmake")
    endif()
    unset(PNX_EMBEDDED_COMPILER_HINT)
endif()

project(pnx_embedded LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PNX_EMBEDDED_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/configs")
set(PNX_EMBEDDED_GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${PNX_BOARD_IOC}"
    "${PNX_BOARD_PARAMS}"
    "${PNX_BOARD_ROBOT_CONFIG}"
    "${PNX_EMBEDDED_CONFIG_DIR}/cmake/generate_config.cmake"
    "${PNX_EMBEDDED_CONFIG_DIR}/cmake/import_ioc.cmake"
)

set(IOC "${PNX_BOARD_IOC}")
set(PARAMS "${PNX_BOARD_PARAMS}")
set(ROBOT_CONFIG "${PNX_BOARD_ROBOT_CONFIG}")
set(OUT_DIR "${PNX_EMBEDDED_GENERATED_DIR}")
include("${PNX_EMBEDDED_CONFIG_DIR}/cmake/generate_config.cmake")
unset(IOC)
unset(PARAMS)
unset(ROBOT_CONFIG)
unset(OUT_DIR)

set(PNX_EMBEDDED_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/demo/app.cpp"
    "${PNX_BSP_ROOT}/dwt/src/bsp_dwt.cpp"
    "${PNX_BSP_ROOT}/diagnostics/src/bsp_diagnostics.cpp"
    "${PNX_BSP_ROOT}/indicator/src/bsp_indicator.cpp"
    "${PNX_BSP_ROOT}/diagnostics/src/fault_handlers.S"
)

set(PNX_EMBEDDED_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/demo/common"
    "${CMAKE_CURRENT_SOURCE_DIR}/pnx_bsp/diagnostics/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/pnx_bsp/dwt/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/pnx_bsp/indicator/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/pnx_libs/common/include"
    "${PNX_EMBEDDED_GENERATED_DIR}"
)

if(PNX_ENABLE_PWM_A2)
    list(APPEND PNX_EMBEDDED_SOURCES
        "${PNX_BSP_ROOT}/pwm/src/bsp_pwm.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/demo/cboard/pwm_a2/pwm_a2.cpp"
    )
    list(APPEND PNX_EMBEDDED_INCLUDE_DIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/pnx_bsp/pwm/include"
        "${PNX_BSP_ROOT}/pwm/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/demo/cboard/pwm_a2"
    )
elseif(PNX_ENABLE_USB_CDC)
    list(APPEND PNX_EMBEDDED_SOURCES
        "${PNX_BSP_ROOT}/usb/src/bsp_usb.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/demo/cboard/usb_cdc/usb_cdc.cpp"
    )
    list(APPEND PNX_EMBEDDED_INCLUDE_DIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/pnx_bsp/usb/include"
        "${PNX_BSP_ROOT}/usb/src"
    )
elseif(PNX_ENABLE_PS2_VALIDATION)
    list(APPEND PNX_EMBEDDED_SOURCES
        "${PNX_BSP_ROOT}/usart/src/bsp_usart.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/pnx_libs/msg/src/msg.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/pnx_modules/remoter/src/remoter.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/pnx_modules/remoter/src/ps2.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/demo/common/demo_debug.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/demo/cboard/ps2_validation/ps2_validation.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/demo/ps2/ps2_demo.cpp"
    )
    list(APPEND PNX_EMBEDDED_INCLUDE_DIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/pnx_bsp/usart/include"
        "${PNX_BSP_ROOT}/usart/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/pnx_libs/msg/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/pnx_modules/remoter/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/demo/cboard/ps2_validation"
        "${CMAKE_CURRENT_SOURCE_DIR}/demo/ps2"
    )
else()
    list(APPEND PNX_EMBEDDED_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/demo/cboard/board_smoke/board_smoke.cpp"
    )
endif()

set(PNX_F407_CAN_ENABLED OFF)
list(FIND PNX_EMBEDDED_SOURCES
    "${PNX_BSP_ROOT}/can/src/bsp_can.cpp"
    PNX_F407_CAN_SOURCE_INDEX
)
if(NOT PNX_F407_CAN_SOURCE_INDEX EQUAL -1)
    set(PNX_F407_CAN_ENABLED ON)
endif()

set(PNX_F407_USART_ENABLED OFF)
list(FIND PNX_EMBEDDED_SOURCES
    "${PNX_BSP_ROOT}/usart/src/bsp_usart.cpp"
    PNX_F407_USART_SOURCE_INDEX
)
if(NOT PNX_F407_USART_SOURCE_INDEX EQUAL -1)
    set(PNX_F407_USART_ENABLED ON)
endif()
set(PNX_F407_USART1_ENABLED OFF)
set(PNX_F407_USART3_ENABLED OFF)
set(PNX_F407_USART6_ENABLED OFF)
if(PNX_F407_USART_ENABLED)
    if(PNX_F407_USART1_ACTIVE)
        set(PNX_F407_USART1_ENABLED ON)
    endif()
    if(PNX_F407_USART3_ACTIVE)
        set(PNX_F407_USART3_ENABLED ON)
    endif()
    if(PNX_F407_USART6_ACTIVE)
        set(PNX_F407_USART6_ENABLED ON)
    endif()
endif()

add_executable(${PROJECT_NAME})
add_subdirectory("${PNX_BOARD_CUBEMX_DIR}" board_support)

target_sources(${PROJECT_NAME} PRIVATE ${PNX_EMBEDDED_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE
    ${PNX_EMBEDDED_INCLUDE_DIRS}
)
target_link_libraries(${PROJECT_NAME} stm32cubemx)
if(PNX_ENABLE_PWM_A2)
    target_compile_definitions(${PROJECT_NAME} PRIVATE PNX_APP_PWM_A2=1)
elseif(PNX_ENABLE_USB_CDC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        PNX_APP_USB_CDC=1
        "PNX_USB_DEVICE_IDENTITY_CONFIRMED=${PNX_USB_DEVICE_IDENTITY_CONFIRMED_VALUE}"
        "PNX_USB_DEVICE_VID=${PNX_USB_DEVICE_VID_VALUE}"
        "PNX_USB_DEVICE_PID=${PNX_USB_DEVICE_PID_VALUE}"
        "PNX_USB_DEVICE_MANUFACTURER=\"${PNX_USB_DEVICE_MANUFACTURER}\""
        "PNX_USB_DEVICE_PRODUCT=\"${PNX_USB_DEVICE_PRODUCT}\""
        "PNX_USB_DEVICE_SERIAL=\"${PNX_USB_DEVICE_SERIAL}\""
    )
elseif(PNX_ENABLE_PS2_VALIDATION)
    target_compile_definitions(${PROJECT_NAME} PRIVATE PNX_APP_PS2_VALIDATION=1)
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE PNX_APP_BOARD_SMOKE=1)
endif()
target_compile_definitions(${PROJECT_NAME} PRIVATE
    "PNX_F407_CAN_ENABLED=$<BOOL:${PNX_F407_CAN_ENABLED}>"
    "PNX_F407_USART_ENABLED=$<BOOL:${PNX_F407_USART_ENABLED}>"
    "PNX_F407_USART1_ENABLED=$<BOOL:${PNX_F407_USART1_ENABLED}>"
    "PNX_F407_USART3_ENABLED=$<BOOL:${PNX_F407_USART3_ENABLED}>"
    "PNX_F407_USART6_ENABLED=$<BOOL:${PNX_F407_USART6_ENABLED}>"
    "PNX_F407_USB_ENABLED=$<BOOL:${PNX_F407_USB_ENABLED}>"
)

message(STATUS "PnX target: DJI C-board STM32F407")
message(STATUS "PnX USB CDC closure: ${PNX_ENABLE_USB_CDC}")
message(STATUS "PnX PWM/A2 closure: ${PNX_ENABLE_PWM_A2}")
message(STATUS "PnX PS2 validation closure: ${PNX_ENABLE_PS2_VALIDATION}")
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)
