#include "status_led.hpp"

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

namespace galleon {

StatusLed::StatusLed(bool external_enabled, unsigned int external_pin)
    : external_enabled_(external_enabled), external_pin_(external_pin) {}

void StatusLed::init() {
    if (external_enabled_) {
        gpio_init(external_pin_);
        gpio_set_dir(external_pin_, GPIO_OUT);
    }
    off();
}

void StatusLed::write(bool on) {
    state_ = on;
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on ? 1 : 0);
    if (external_enabled_) gpio_put(external_pin_, on);
}

void StatusLed::off() {
    write(false);
}

void StatusLed::update(std::uint32_t now_ms, bool connected) {
    if (connected != last_connected_) {
        last_connected_ = connected;
        next_toggle_ms_ = now_ms;
    }
    if (static_cast<std::int32_t>(now_ms - next_toggle_ms_) < 0) return;
    write(!state_);
    next_toggle_ms_ = now_ms + (connected ? 1000U : 250U);
}

}  // namespace galleon
