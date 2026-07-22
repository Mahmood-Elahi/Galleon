add_executable(button_test main.cpp)
target_link_libraries(button_test PRIVATE pico_stdlib)
target_include_directories(button_test PRIVATE ${PROJECT_SOURCE_DIR}/common)
galleon_configure_firmware(button_test)
