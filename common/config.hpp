#pragma once

#include <cstdint>

#ifndef GALLEON_DEBUG
#define GALLEON_DEBUG 0
#endif

namespace galleon::config {

inline constexpr char kDeviceName[] = "B8_B4";
inline constexpr std::uint16_t kServiceUuid = 0x1848;
inline constexpr std::uint16_t kControlCharacteristicUuid = 0x2A6E;
inline constexpr std::uint16_t kBleAppearance = 384;

inline constexpr std::uint32_t kAdvertisingIntervalMs = 250;
inline constexpr std::uint32_t kControlSampleIntervalMs = 50;
inline constexpr std::uint32_t kControlKeepaliveIntervalMs = 200;
inline constexpr std::uint32_t kCommandWatchdogMs = 500;
inline constexpr std::uint16_t kJoystickChangeThreshold = 64;

inline constexpr unsigned int kLeftJoystickPin = 27;
inline constexpr unsigned int kRightJoystickPin = 26;
inline constexpr unsigned int kControllerButtonPin = 13;
inline constexpr unsigned int kJoystickTestSwitchPin = 22;

inline constexpr unsigned int kLeftMotorEnablePin = 0;
inline constexpr unsigned int kLeftMotorDirectionPin = 1;
inline constexpr unsigned int kExternalGreenLedPin = 2;
inline constexpr unsigned int kRightMotorEnablePin = 3;
inline constexpr unsigned int kRightMotorDirectionPin = 4;

inline constexpr int kDeadzoneLow = -2;
inline constexpr int kDeadzoneHigh = 2;
inline constexpr int kMaxSpeed = 100;

// Production polarity: LOW is forward and HIGH is backward.
// Change either flag after physical direction testing if a motor is mirrored.
inline constexpr bool kLeftMotorDirectionInverted = false;
inline constexpr bool kRightMotorDirectionInverted = false;

inline constexpr std::uint32_t kMotorPwmFrequencyHz = 500;
inline constexpr std::uint16_t kMotorPwmWrap = 65535;

}  // namespace galleon::config
