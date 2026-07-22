#include <cstdio>

#include "config.hpp"
#include "control_math.hpp"
#include "hardware/adc.h"
#include "pico/stdlib.h"

namespace {

std::uint16_t read_protocol_adc(unsigned int input) {
    adc_select_input(input);
    return galleon::adc12_to_protocol(adc_read());
}

const char* x_direction(std::uint16_t value) {
    if (value < 20000) return "left";
    if (value > 50000) return "right";
    return "middle";
}

const char* y_direction(std::uint16_t value) {
    if (value < 20000) return "up";
    if (value > 50000) return "down";
    return "middle";
}

}  // namespace

int main() {
    stdio_init_all();
    adc_init();
    adc_gpio_init(galleon::config::kLeftJoystickPin);
    adc_gpio_init(galleon::config::kRightJoystickPin);
    gpio_init(galleon::config::kJoystickTestSwitchPin);
    gpio_set_dir(galleon::config::kJoystickTestSwitchPin, GPIO_IN);
    gpio_pull_up(galleon::config::kJoystickTestSwitchPin);

    while (true) {
        const std::uint16_t x = read_protocol_adc(1);  // GP27
        const std::uint16_t y = read_protocol_adc(0);  // GP26
        if (!gpio_get(galleon::config::kJoystickTestSwitchPin)) std::printf("PRESS\n");
        std::printf("X=%u Y=%u: %s, %s\n", x, y, x_direction(x), y_direction(y));
        sleep_ms(1000);
    }
}
