#include <cstdio>

#include "config.hpp"
#include "pico/stdlib.h"

int main() {
    stdio_init_all();
    gpio_init(galleon::config::kControllerButtonPin);
    gpio_set_dir(galleon::config::kControllerButtonPin, GPIO_IN);
    gpio_pull_up(galleon::config::kControllerButtonPin);
    while (true) {
        std::printf(gpio_get(galleon::config::kControllerButtonPin)
                        ? "WAITING...\n"
                        : "PRESS\n");
        sleep_ms(1000);
    }
}
