#include "control_math.hpp"

#include <algorithm>
#include <cstdint>

namespace galleon {

std::uint16_t adc12_to_protocol(std::uint16_t native_adc) {
    const std::uint32_t clamped = std::min<std::uint32_t>(native_adc, 4095U);
    return static_cast<std::uint16_t>((clamped * 65535U + 2047U) / 4095U);
}

int clamp_speed(int speed, int max_speed) {
    if (max_speed < 0) max_speed = -max_speed;
    return std::clamp(speed, -max_speed, max_speed);
}

int protocol_adc_to_speed(std::uint16_t protocol_adc, int deadzone_low,
                          int deadzone_high, int max_speed) {
    if (max_speed < 0) max_speed = -max_speed;
    const std::uint32_t span = static_cast<std::uint32_t>(max_speed * 2);
    const int speed = static_cast<int>(
        (static_cast<std::uint32_t>(protocol_adc) * span + 32767U) / 65535U) -
        max_speed;
    if (speed >= deadzone_low && speed <= deadzone_high) return 0;
    return clamp_speed(speed, max_speed);
}

std::uint16_t quadratic_pwm_duty(int speed, int max_speed) {
    if (max_speed <= 0) return 0;
    const std::uint32_t magnitude = static_cast<std::uint32_t>(
        speed < 0 ? -static_cast<std::int64_t>(speed) : speed);
    const std::uint32_t clamped =
        std::min<std::uint32_t>(magnitude, static_cast<std::uint32_t>(max_speed));
    const std::uint64_t numerator =
        static_cast<std::uint64_t>(clamped) * clamped * 65535U;
    const std::uint64_t denominator =
        static_cast<std::uint64_t>(max_speed) * max_speed;
    return static_cast<std::uint16_t>((numerator + denominator / 2U) / denominator);
}

}  // namespace galleon
