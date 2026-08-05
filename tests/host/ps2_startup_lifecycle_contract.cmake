if(NOT DEFINED PNX_SOURCE_DIR)
    message(FATAL_ERROR
        "ps2_startup_lifecycle_contract requires PNX_SOURCE_DIR")
endif()

file(READ
    "${PNX_SOURCE_DIR}/demo/ps2/ps2_demo.cpp"
    ps2_demo_source)

string(FIND "${ps2_demo_source}" "void monitor_entry(" monitor_entry_index)
string(FIND "${ps2_demo_source}" "void run() noexcept" run_index)
string(FIND "${ps2_demo_source}"
    "::remoter::service::instance().init(cfg)" service_init_index)
string(FIND "${ps2_demo_source}"
    "raw_sub = msg::subscribe<::remoter::ps2_state>();" raw_subscribe_index)
string(FIND "${ps2_demo_source}"
    "generic_sub = msg::subscribe<::remoter::state>();" generic_subscribe_index)
string(FIND "${ps2_demo_source}"
    "tx_thread_create(&monitor_thread" monitor_create_index)

foreach(required_index IN ITEMS
        monitor_entry_index
        run_index
        service_init_index
        raw_subscribe_index
        generic_subscribe_index
        monitor_create_index)
    if(${required_index} EQUAL -1)
        message(FATAL_ERROR
            "PS2 startup lifecycle contract could not find ${required_index}")
    endif()
endforeach()

foreach(thread_only_index IN ITEMS
        service_init_index
        raw_subscribe_index
        generic_subscribe_index)
    if(NOT (${thread_only_index} GREATER monitor_entry_index AND
            ${thread_only_index} LESS run_index))
        message(FATAL_ERROR
            "${thread_only_index} must execute from monitor_entry after "
            "ThreadX scheduling starts, not from app_start/run")
    endif()
endforeach()

if(NOT monitor_create_index GREATER run_index)
    message(FATAL_ERROR
        "run must create the PS2 monitor thread for deferred initialization")
endif()

if(NOT ps2_demo_source MATCHES
   "monitor_stack\\[([0-9]+)\\]")
    message(FATAL_ERROR
        "PS2 startup lifecycle contract could not read monitor stack size")
endif()
set(monitor_stack_bytes "${CMAKE_MATCH_1}")
if(monitor_stack_bytes LESS 1536)
    message(FATAL_ERROR
        "PS2 monitor stack must cover deferred service/UART initialization")
endif()

file(READ
    "${PNX_SOURCE_DIR}/pnx_modules/remoter/include/remoter.hpp"
    remoter_header)
string(FIND "${remoter_header}" "class ps2" ps2_class_index)
string(FIND "${remoter_header}" "class service" service_class_index)
if(ps2_class_index EQUAL -1)
    message(FATAL_ERROR
        "PS2 startup lifecycle contract could not find remoter::ps2")
endif()
if(service_class_index EQUAL -1)
    message(FATAL_ERROR
        "PS2 startup lifecycle contract could not find remoter::service")
endif()
math(EXPR ps2_class_length "${service_class_index} - ${ps2_class_index}")
string(SUBSTRING "${remoter_header}" ${ps2_class_index}
    ${ps2_class_length} ps2_class)
if(NOT ps2_class MATCHES "stack_\\[([0-9]+)\\]")
    message(FATAL_ERROR
        "PS2 startup lifecycle contract could not read remoter::ps2 stack size")
endif()
set(ps2_stack_bytes "${CMAKE_MATCH_1}")
if(ps2_stack_bytes LESS 1536)
    message(FATAL_ERROR
        "remoter PS2 stack must cover sustained UART frame processing")
endif()

string(SUBSTRING "${remoter_header}" ${service_class_index} -1 service_class)
if(NOT service_class MATCHES "stack_\\[([0-9]+)\\]")
    message(FATAL_ERROR
        "PS2 startup lifecycle contract could not read service stack size")
endif()
set(service_stack_bytes "${CMAKE_MATCH_1}")
if(service_stack_bytes LESS 1536)
    message(FATAL_ERROR
        "remoter service stack must cover the unified PS2 merge state")
endif()
