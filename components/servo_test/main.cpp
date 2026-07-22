#include <cstdio>

#include "pico/stdlib.h"
#include "servo.hpp"

int main() {
    stdio_init_all();
    galleon::Servo servo(22);
    servo.init();
    constexpr float sequence[] = {0.0F, 90.0F, 180.0F, 90.0F};
    while (true) {
        for (const float angle : sequence) {
            std::printf("Servo %.0f degrees\n", static_cast<double>(angle));
            servo.move(angle);
            sleep_ms(2000);
        }
    }
}
