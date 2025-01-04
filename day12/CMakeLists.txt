cmake_minimum_required(VERSION 3.12)

include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(blink C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

pico_sdk_init()

add_executable(blink blink.cpp)

target_link_libraries(blink pico_stdlib hardware_adc hardware_pwm hardware_i2c)

pico_enable_stdio_usb(blink 1)
pico_enable_stdio_uart(blink 1)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(blink)
