#include <cstdio>

#include "config.hpp"
#include "pico/stdlib.h"

int main() {
    stdio_init_all();
    gpio_init(galleon::config::kExternalGreenLedPin);
    gpio_set_dir(galleon::config::kExternalGreenLedPin, GPIO_OUT);
    std::printf("External LED test on GP2\n");
    while (true) {
        gpio_put(galleon::config::kExternalGreenLedPin, true);
        sleep_ms(1000);
        gpio_put(galleon::config::kExternalGreenLedPin, false);
        sleep_ms(1000);
    }
}
