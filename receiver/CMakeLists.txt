add_executable(receiver
    main.cpp
    motor_controller.cpp
)
target_link_libraries(receiver PRIVATE
    galleon_common
    galleon_status_led
    pico_stdlib
    hardware_pwm
    hardware_clocks
    pico_btstack_ble
    pico_btstack_cyw43
    pico_cyw43_arch_none
)
target_include_directories(receiver PRIVATE ${CMAKE_CURRENT_LIST_DIR})
galleon_configure_firmware(receiver)
