#pragma once

#include <cstdint>

namespace galleon {

class MotorController {
public:
    MotorController(unsigned int left_enable_pin, unsigned int left_direction_pin,
                    unsigned int right_enable_pin, unsigned int right_direction_pin,
                    bool left_inverted, bool right_inverted);

    void init();
    void set_left_speed(int speed);
    void set_right_speed(int speed);
    void set_speeds(int left_speed, int right_speed);
    void stop();

    int left_speed() const { return left_speed_; }
    int right_speed() const { return right_speed_; }
    std::uint16_t left_duty() const { return left_duty_; }
    std::uint16_t right_duty() const { return right_duty_; }

private:
    void configure_pwm_pin(unsigned int pin);
    void set_motor(unsigned int enable_pin, unsigned int direction_pin,
                   bool inverted, int speed, int& recorded_speed,
                   std::uint16_t& recorded_duty);

    unsigned int left_enable_pin_;
    unsigned int left_direction_pin_;
    unsigned int right_enable_pin_;
    unsigned int right_direction_pin_;
    bool left_inverted_;
    bool right_inverted_;
    int left_speed_ = 0;
    int right_speed_ = 0;
    std::uint16_t left_duty_ = 0;
    std::uint16_t right_duty_ = 0;
};

}  // namespace galleon
