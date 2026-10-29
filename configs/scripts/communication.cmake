# UI 与 PS2 的发送超时和输入死区。
config_integer(_ui_timeout 0 2147483647 ui timeout_ms)
config_number(_deadzone 0 1 ps2 deadzone)
if(_deadzone GREATER_EQUAL 1)
    message(FATAL_ERROR "ps2.deadzone must be less than 1")
endif()
if(NOT _deadzone MATCHES "[.eE]")
    string(APPEND _deadzone ".0")
endif()
config_integer(_spi_timeout 1 2147483647 ps2 spi_timeout_ms)

# 裁判接收与应用遥控处理的调度参数。
config_integer(_remoter_stack 1024 16384 remoter stack_size)
config_integer(_referee_stack 2048 16384 referee stack_size)

foreach(_module IN ITEMS referee remoter)
    config_integer(_${_module}_priority 0 31 ${_module} thread_priority)
    config_integer(_${_module}_period 1 2147483647 ${_module} period_ticks)
    config_integer(_${_module}_offline 1 2147483647 ${_module} offline_timeout_ms)
endforeach()
config_integer(_remoter_ps2_period 1 2147483647 remoter ps2_period_ms)
