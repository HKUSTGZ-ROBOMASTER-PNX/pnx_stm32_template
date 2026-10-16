cmake_minimum_required(VERSION 3.22)

add_library(libs INTERFACE)
target_compile_features(libs INTERFACE cxx_std_17)
file(GLOB _libs_include_dirs CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*/include")
target_include_directories(libs INTERFACE ${_libs_include_dirs})

# boards supplies the selected board's ThreadX headers and configuration.
target_link_libraries(libs INTERFACE stm32cubemx)

file(GLOB_RECURSE _libs_headers CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*/include/*.hpp"
)
target_sources(libs INTERFACE ${_libs_headers})
