#pragma once

#include <cstdint>

namespace galleon {

std::uint16_t adc12_to_protocol(std::uint16_t native_adc);
int protocol_adc_to_speed(std::uint16_t protocol_adc, int deadzone_low,
                          int deadzone_high, int max_speed = 100);
int clamp_speed(int speed, int max_speed = 100);
std::uint16_t quadratic_pwm_duty(int speed, int max_speed = 100);

}  // namespace galleon
