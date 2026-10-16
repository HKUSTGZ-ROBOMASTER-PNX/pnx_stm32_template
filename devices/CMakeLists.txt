cmake_minimum_required(VERSION 3.22)

add_library(devices INTERFACE)
target_compile_features(devices INTERFACE cxx_std_17)
target_link_libraries(devices INTERFACE board libs bsp)

file(GLOB _device_include_dirs CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/*/include"
)
target_include_directories(devices INTERFACE ${_device_include_dirs})

file(GLOB _device_headers CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*/include/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/*/include/*.hpp"
)
file(GLOB _device_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/*/src/*.cpp"
)
# Exactly one board LED backend; all other device implementations compile on both boards.
list(REMOVE_ITEM _device_sources
    "${CMAKE_CURRENT_SOURCE_DIR}/led/src/ws2812.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/led/src/gpio.cpp"
)
target_sources(devices INTERFACE ${_device_headers} ${_device_sources})

if(BOARD STREQUAL "h723_mc02")
    target_sources(devices INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/led/src/ws2812.cpp")
elseif(BOARD STREQUAL "f407_cboard")
    target_sources(devices INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/led/src/gpio.cpp")
else()
    message(FATAL_ERROR "No LED implementation selected for BOARD '${BOARD}'")
endif()
