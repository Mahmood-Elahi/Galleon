cmake_minimum_required(VERSION 3.16)
project(galleon_host_tests LANGUAGES CXX)

enable_testing()

add_library(galleon_host_common OBJECT
    ../common/control_protocol.cpp
    ../common/control_math.cpp
)
target_include_directories(galleon_host_common PUBLIC ../common)
target_compile_features(galleon_host_common PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(galleon_host_common PRIVATE /W4)
else()
    target_compile_options(galleon_host_common PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(protocol_tests protocol_tests.cpp)
target_link_libraries(protocol_tests PRIVATE galleon_host_common)
add_test(NAME protocol_tests COMMAND protocol_tests)

add_executable(control_math_tests control_math_tests.cpp)
target_link_libraries(control_math_tests PRIVATE galleon_host_common)
add_test(NAME control_math_tests COMMAND control_math_tests)
