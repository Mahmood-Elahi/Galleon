#include "control_math.hpp"

#include <cassert>
#include <cstdlib>
#include <iostream>

int main() {
    assert(galleon::adc12_to_protocol(0) == 0);
    assert(galleon::adc12_to_protocol(4095) == 65535);
    assert(galleon::adc12_to_protocol(5000) == 65535);

    assert(galleon::protocol_adc_to_speed(0, -2, 2) == -100);
    assert(galleon::protocol_adc_to_speed(65535, -2, 2) == 100);
    assert(std::abs(galleon::protocol_adc_to_speed(32767, -2, 2)) <= 1);
    assert(galleon::protocol_adc_to_speed(32100, -2, 2) == 0);
    assert(galleon::protocol_adc_to_speed(33400, -2, 2) == 0);

    assert(galleon::clamp_speed(-200) == -100);
    assert(galleon::clamp_speed(200) == 100);
    assert(galleon::clamp_speed(25) == 25);

    assert(galleon::quadratic_pwm_duty(0) == 0);
    assert(galleon::quadratic_pwm_duty(100) == 65535);
    assert(galleon::quadratic_pwm_duty(-100) == 65535);
    assert(galleon::quadratic_pwm_duty(200) == 65535);

    std::uint16_t previous = 0;
    for (int speed = 0; speed <= 100; ++speed) {
        const std::uint16_t duty = galleon::quadratic_pwm_duty(speed);
        assert(duty >= previous);
        assert(duty == galleon::quadratic_pwm_duty(-speed));
        previous = duty;
    }

    std::cout << "control_math_tests: all assertions passed\n";
    return 0;
}
