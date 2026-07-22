#include "motor_controller.hpp"

#include "config.hpp"
#include "control_math.hpp"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

namespace galleon {

MotorController::MotorController(unsigned int left_enable_pin,
                                 unsigned int left_direction_pin,
                                 unsigned int right_enable_pin,
                                 unsigned int right_direction_pin,
                                 bool left_inverted, bool right_inverted)
    : left_enable_pin_(left_enable_pin),
      left_direction_pin_(left_direction_pin),
      right_enable_pin_(right_enable_pin),
      right_direction_pin_(right_direction_pin),
      left_inverted_(left_inverted),
      right_inverted_(right_inverted) {}

void MotorController::configure_pwm_pin(unsigned int pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    const unsigned int slice = pwm_gpio_to_slice_num(pin);
    pwm_config pwm_configuration = pwm_get_default_config();
    pwm_config_set_wrap(&pwm_configuration, config::kMotorPwmWrap);
    const float divider = static_cast<float>(clock_get_hz(clk_sys)) /
                          (static_cast<float>(config::kMotorPwmFrequencyHz) *
                           static_cast<float>(config::kMotorPwmWrap + 1U));
    pwm_config_set_clkdiv(&pwm_configuration, divider);
    pwm_init(slice, &pwm_configuration, true);
    pwm_set_gpio_level(pin, 0);
}

void MotorController::init() {
    gpio_init(left_direction_pin_);
    gpio_set_dir(left_direction_pin_, GPIO_OUT);
    gpio_put(left_direction_pin_, false);
    gpio_init(right_direction_pin_);
    gpio_set_dir(right_direction_pin_, GPIO_OUT);
    gpio_put(right_direction_pin_, false);

    configure_pwm_pin(left_enable_pin_);
    configure_pwm_pin(right_enable_pin_);
    stop();
}

void MotorController::set_motor(unsigned int enable_pin, unsigned int direction_pin,
                                bool inverted, int speed, int& recorded_speed,
                                std::uint16_t& recorded_duty) {
    speed = clamp_speed(speed, config::kMaxSpeed);
    if (speed >= config::kDeadzoneLow && speed <= config::kDeadzoneHigh) speed = 0;

    const bool forward = speed > 0;
    const bool direction_high = (speed == 0) ? false : ((!forward) ^ inverted);
    const std::uint16_t duty = quadratic_pwm_duty(speed, config::kMaxSpeed);

    gpio_put(direction_pin, direction_high);
    pwm_set_gpio_level(enable_pin, duty);
    recorded_speed = speed;
    recorded_duty = duty;
}

void MotorController::set_left_speed(int speed) {
    set_motor(left_enable_pin_, left_direction_pin_, left_inverted_, speed,
              left_speed_, left_duty_);
}

void MotorController::set_right_speed(int speed) {
    set_motor(right_enable_pin_, right_direction_pin_, right_inverted_, speed,
              right_speed_, right_duty_);
}

void MotorController::set_speeds(int left_speed, int right_speed) {
    set_left_speed(left_speed);
    set_right_speed(right_speed);
}

void MotorController::stop() {
    pwm_set_gpio_level(left_enable_pin_, 0);
    pwm_set_gpio_level(right_enable_pin_, 0);
    left_speed_ = 0;
    right_speed_ = 0;
    left_duty_ = 0;
    right_duty_ = 0;
}

}  // namespace galleon
