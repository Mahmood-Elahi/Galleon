add_executable(transmitter main.cpp)
target_link_libraries(transmitter PRIVATE
    galleon_common
    galleon_status_led
    pico_stdlib
    hardware_adc
    pico_btstack_ble
    pico_btstack_cyw43
    pico_cyw43_arch_none
)
target_include_directories(transmitter PRIVATE ${CMAKE_CURRENT_LIST_DIR})
pico_btstack_make_gatt_header(transmitter PRIVATE transmitter.gatt)
galleon_configure_firmware(transmitter)
