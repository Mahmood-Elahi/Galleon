#include "servo.hpp"

#include <algorithm>
#include <cmath>

#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

namespace galleon {

Servo::Servo(unsigned int pin, ServoCalibration calibration)
    : pin_(pin), calibration_(calibration) {}

void Servo::configure_pwm() {
    gpio_set_function(pin_, GPIO_FUNC_PWM);
    slice_ = pwm_gpio_to_slice_num(pin_);
    pwm_config pwm_configuration = pwm_get_default_config();
    pwm_config_set_wrap(&pwm_configuration, 65535);
    const float divider = static_cast<float>(clock_get_hz(clk_sys)) /
                          (static_cast<float>(calibration_.frequency_hz) * 65536.0F);
    pwm_config_set_clkdiv(&pwm_configuration, divider);
    pwm_init(slice_, &pwm_configuration, true);
}

void Servo::init() {
    configure_pwm();
    initialized_ = true;
    last_angle_centidegrees_ = -1;
}

std::uint16_t Servo::duty_for_angle(float angle_degrees) const {
    const float minimum = std::min(calibration_.minimum_angle, calibration_.maximum_angle);
    const float maximum = std::max(calibration_.minimum_angle, calibration_.maximum_angle);
    const float clamped = std::clamp(angle_degrees, minimum, maximum);
    const float angle_span = calibration_.maximum_angle - calibration_.minimum_angle;
    if (angle_span == 0.0F) return calibration_.minimum_duty;
    const float fraction = (clamped - calibration_.minimum_angle) / angle_span;
    const float duty_span = static_cast<float>(calibration_.maximum_duty) -
                            static_cast<float>(calibration_.minimum_duty);
    const long rounded = std::lround(static_cast<float>(calibration_.minimum_duty) +
                                     fraction * duty_span);
    return static_cast<std::uint16_t>(std::clamp<long>(rounded, 0, 65535));
}

void Servo::move(float angle_degrees) {
    if (!initialized_) init();
    const float minimum = std::min(calibration_.minimum_angle, calibration_.maximum_angle);
    const float maximum = std::max(calibration_.minimum_angle, calibration_.maximum_angle);
    const float clamped = std::clamp(angle_degrees, minimum, maximum);
    const std::int32_t centidegrees = static_cast<std::int32_t>(std::lround(clamped * 100.0F));
    if (centidegrees == last_angle_centidegrees_) return;
    last_angle_centidegrees_ = centidegrees;
    pwm_set_gpio_level(pin_, duty_for_angle(clamped));
}

void Servo::set_calibration(const ServoCalibration& calibration) {
    calibration_ = calibration;
    last_angle_centidegrees_ = -1;
    if (initialized_) configure_pwm();
}

}  // namespace galleon
