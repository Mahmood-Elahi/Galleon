add_executable(led_test main.cpp)
target_link_libraries(led_test PRIVATE pico_stdlib)
target_include_directories(led_test PRIVATE ${PROJECT_SOURCE_DIR}/common)
galleon_configure_firmware(led_test)
