if(NOT DEFINED PNX_SOURCE_DIR)
    message(FATAL_ERROR "PNX_SOURCE_DIR is required")
endif()

set(PNX_DIRECT_BSP_MODULES
    can
    diagnostics
    dwt
    exti
    flash
    indicator
    pwm
    spi
    usart
    usb
)

foreach(module IN LISTS PNX_DIRECT_BSP_MODULES)
    set(shared_source
        "${PNX_SOURCE_DIR}/pnx_bsp/${module}/src/bsp_${module}.cpp")
    if(NOT EXISTS "${shared_source}")
        message(FATAL_ERROR
            "Direct BSP source is missing from pnx_bsp: ${shared_source}")
    endif()

    set(parent_source
        "${PNX_SOURCE_DIR}/boards/dji_c_board_f407/bsp/bsp_${module}.cpp")
    if(EXISTS "${parent_source}")
        message(FATAL_ERROR
            "Direct BSP implementation still lives in the parent Board tree: "
            "${parent_source}")
    endif()
endforeach()

file(GLOB_RECURSE direct_bsp_sources
    "${PNX_SOURCE_DIR}/pnx_bsp/*/src/*.cpp")
foreach(source IN LISTS direct_bsp_sources)
    file(READ "${source}" source_text)
    if(source_text MATCHES "detail::backend_")
        message(FATAL_ERROR
            "Retired backend forwarding symbol found in ${source}")
    endif()
endforeach()
