#pragma once

#include <cstdint>

namespace galleon {

struct ServoCalibration {
    std::uint32_t frequency_hz = 50;
    std::uint16_t minimum_duty = 1638;
    std::uint16_t maximum_duty = 7864;
    float minimum_angle = 0.0F;
    float maximum_angle = 180.0F;
};

class Servo {
public:
    explicit Servo(unsigned int pin, ServoCalibration calibration = {});

    void init();
    void move(float angle_degrees);
    void set_calibration(const ServoCalibration& calibration);
    const ServoCalibration& calibration() const { return calibration_; }
    std::uint16_t duty_for_angle(float angle_degrees) const;

private:
    void configure_pwm();

    unsigned int pin_;
    unsigned int slice_ = 0;
    ServoCalibration calibration_;
    std::int32_t last_angle_centidegrees_ = -1;
    bool initialized_ = false;
};

}  // namespace galleon
