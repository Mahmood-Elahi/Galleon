add_executable(servo_test
    main.cpp
    ../servo/servo.cpp
)
target_link_libraries(servo_test PRIVATE pico_stdlib hardware_pwm hardware_clocks)
target_include_directories(servo_test PRIVATE ../servo)
galleon_configure_firmware(servo_test)
