#include <cstdio>

#include "config.hpp"
#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"

// PHYSICAL SAFETY WARNING:
// Lift propellers clear of people, clothing, wiring, and the work surface before
// flashing or running this target. This test deliberately starts motors at a
// substantial duty cycle. This target uses HIGH as forward and LOW as backward.

namespace {

constexpr std::uint16_t kTestDuty = 40000;

void configure_pwm(unsigned int pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    const unsigned int slice = pwm_gpio_to_slice_num(pin);
    pwm_config configuration = pwm_get_default_config();
    pwm_config_set_wrap(&configuration, 65535);
    const float divider = static_cast<float>(clock_get_hz(clk_sys)) /
                          (500.0F * 65536.0F);
    pwm_config_set_clkdiv(&configuration, divider);
    pwm_init(slice, &configuration, true);
    pwm_set_gpio_level(pin, 0);
}

void motor_control(unsigned int enable_pin, unsigned int direction_pin,
                   const char* direction, std::uint16_t duty) {
    if (direction[0] == 'f') {
        gpio_put(direction_pin, true);   // Test target: HIGH = forward.
    } else if (direction[0] == 'b') {
        gpio_put(direction_pin, false);  // Test target: LOW = backward.
    }
    pwm_set_gpio_level(enable_pin, duty);
}

void run_one(unsigned int enable_pin, unsigned int direction_pin, const char* name) {
    std::printf("%s forward\n", name);
    motor_control(enable_pin, direction_pin, "forward", kTestDuty);
    sleep_ms(2000);
    std::printf("%s backward\n", name);
    motor_control(enable_pin, direction_pin, "backward", kTestDuty);
    sleep_ms(2000);
    std::printf("%s stop\n", name);
    motor_control(enable_pin, direction_pin, "stop", 0);
    sleep_ms(1000);
}

}  // namespace

int main() {
    stdio_init_all();
    gpio_init(galleon::config::kLeftMotorDirectionPin);
    gpio_set_dir(galleon::config::kLeftMotorDirectionPin, GPIO_OUT);
    gpio_init(galleon::config::kRightMotorDirectionPin);
    gpio_set_dir(galleon::config::kRightMotorDirectionPin, GPIO_OUT);
    configure_pwm(galleon::config::kLeftMotorEnablePin);
    configure_pwm(galleon::config::kRightMotorEnablePin);
    std::printf("WARNING: motor driver test starting; clear both propellers\n");

    while (true) {
        run_one(galleon::config::kLeftMotorEnablePin,
                galleon::config::kLeftMotorDirectionPin, "Left motor");
        run_one(galleon::config::kRightMotorEnablePin,
                galleon::config::kRightMotorDirectionPin, "Right motor");

        std::printf("Both forward\n");
        motor_control(galleon::config::kLeftMotorEnablePin,
                      galleon::config::kLeftMotorDirectionPin, "forward", kTestDuty);
        motor_control(galleon::config::kRightMotorEnablePin,
                      galleon::config::kRightMotorDirectionPin, "forward", kTestDuty);
        sleep_ms(2000);
        std::printf("Both backward\n");
        motor_control(galleon::config::kLeftMotorEnablePin,
                      galleon::config::kLeftMotorDirectionPin, "backward", kTestDuty);
        motor_control(galleon::config::kRightMotorEnablePin,
                      galleon::config::kRightMotorDirectionPin, "backward", kTestDuty);
        sleep_ms(2000);
        std::printf("Both stop\n");
        motor_control(galleon::config::kLeftMotorEnablePin,
                      galleon::config::kLeftMotorDirectionPin, "stop", 0);
        motor_control(galleon::config::kRightMotorEnablePin,
                      galleon::config::kRightMotorDirectionPin, "stop", 0);
        sleep_ms(1000);
    }
}
