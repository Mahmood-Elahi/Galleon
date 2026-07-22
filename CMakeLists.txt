cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Keep this block compatible with the Raspberry Pi Pico VS Code extension.
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.0)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if(EXISTS ${picoVscode})
    include(${picoVscode})
endif()

set(PICO_BOARD pico_w CACHE STRING "Board type")
include(pico_sdk_import.cmake)

project(Galleon VERSION 1.0.0 LANGUAGES C CXX ASM)
pico_sdk_init()

option(GALLEON_DEBUG "Enable detailed ADC, packet, PWM, and BLE diagnostics" OFF)

add_library(galleon_common STATIC
    common/control_protocol.cpp
    common/control_math.cpp
)
target_include_directories(galleon_common PUBLIC common)
target_compile_features(galleon_common PUBLIC cxx_std_17)

add_library(galleon_status_led STATIC common/status_led.cpp)
target_include_directories(galleon_status_led PUBLIC common)
target_link_libraries(galleon_status_led PUBLIC pico_stdlib pico_cyw43_arch_none)
target_compile_features(galleon_status_led PUBLIC cxx_std_17)

function(galleon_configure_firmware target)
    pico_set_program_name(${target} ${target})
    pico_set_program_version(${target} ${PROJECT_VERSION})
    pico_enable_stdio_usb(${target} 1)
    pico_enable_stdio_uart(${target} 0)
    pico_add_extra_outputs(${target})
    target_compile_features(${target} PRIVATE cxx_std_17)
    target_compile_definitions(${target} PRIVATE GALLEON_DEBUG=$<BOOL:${GALLEON_DEBUG}>)
    target_compile_options(${target} PRIVATE -Wall -Wextra)
endfunction()

add_subdirectory(transmitter)
add_subdirectory(receiver)
add_subdirectory(components/motor_driver_test)
add_subdirectory(components/led_test)
add_subdirectory(components/joystick_test)
add_subdirectory(components/button_test)
add_subdirectory(components/servo_test)
