#pragma once

#include <cstdint>

namespace galleon {

class StatusLed {
public:
    explicit StatusLed(bool external_enabled = false, unsigned int external_pin = 0);

    void init();
    void update(std::uint32_t now_ms, bool connected);
    void off();

private:
    void write(bool on);

    bool external_enabled_;
    unsigned int external_pin_;
    bool state_ = false;
    bool last_connected_ = false;
    std::uint32_t next_toggle_ms_ = 0;
};

}  // namespace galleon
