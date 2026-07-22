# Galleon

<p align="left">
  <img src="https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white" alt="C++17" />
  <img src="https://img.shields.io/badge/Raspberry%20Pi-Pico%20WH-A22846?logo=raspberrypi&logoColor=white" alt="Raspberry Pi Pico WH" />
  <img src="https://img.shields.io/badge/Pico%20SDK-2.3.0-4B8BBE" alt="Pico SDK 2.3.0" />
</p>

Galleon is a Bluetooth-controlled waterborne rescue-vessel prototype developed
for ENGG 200 Winter 2026. Two Raspberry Pi Pico WH boards let it cross water,
maneuver around obstacles, and transport a removable payload.

The firmware is native Pico SDK C++17 using Pico SDK 2.3.0 and its included
BTstack BLE implementation.

## Project outcome

The completed vessel was physically tested in water and finished its test in
**2 min 27 s**. Its propulsion and Bluetooth controls operated while carrying
the required approximately **200 g payload** and maneuvering around rubber
duckies and floating balls. The payload remained transportable throughout.

All seven firmware targets compile for `pico_w`, the host-side tests pass, and
physical water testing succeeded. Long-duration waterproofing, radio range,
endurance, and broader safety validation remain outside the demonstrated scope.

## Design requirements

The course design challenge was to prototype a remotely controlled waterborne
rescue vessel under these principal constraints:

| Requirement | Design constraint |
|---|---|
| Control | Bluetooth link using transmitter and receiver Pico WH boards |
| Operation | Move across water and maneuver around obstacles |
| Payload | Removable load of approximately 200 g |
| Maximum vessel envelope | 250 mm x 250 mm x 200 mm |
| Test environment | Approximately 2600 mm x 1600 mm, 300 mm water depth |
| Electronics | Protected from water, enclosed, and accessible for repair |

## System overview

```text
Joysticks + button -> transmitter Pico WH -> BLE -> receiver Pico WH
                    -> L298N motor driver -> two DC motors
```

- **Transmitter:** BLE peripheral/GATT server that samples two joystick axes
  and an active-low pushbutton, then sends control notifications.
- **Receiver:** BLE central/GATT client that scans, connects, subscribes to
  notifications, validates commands, and drives two motors independently.
- **Steering:** differential thrust allows forward/reverse movement and turns.
- **Failsafe:** invalid data, disconnection, BLE setup failure, or a stale
  command immediately stops both motor PWM outputs.

## Hardware and wiring

### Transmitter

| Pico WH connection | Function |
|---|---|
| GP27 / ADC1 | Left joystick vertical axis |
| GP26 / ADC0 | Right joystick vertical axis |
| GP13 | Pushbutton input with internal pull-up |
| Onboard CYW43 LED | Bluetooth status |

GP13 is electrically active-low, but firmware transmits `B:0` for released and
`B:1` for pressed.

### Receiver

| Connection | Function |
|---|---|
| GP0 | Left motor PWM/enable E1 |
| GP1 | Left motor direction M1 |
| GP3 | Right motor PWM/enable E2 |
| GP4 | Right motor direction M2 |
| GP2 | External green status LED |
| GP22 | Standalone servo-test signal only |
| Onboard CYW43 LED | Bluetooth status |
| L298N | Dual motor driver |
| 9 V supply | Motor-driver power where applicable |
| GND | Common logic ground between Pico, driver, and external supplies |

- Disconnect the battery before connecting a Pico to a computer.
- Do not power motors or a servo from the Pico 3V3 rail.

## Firmware behavior

The transmitter advertises as `B8_B4` and the receiver reconnects automatically
after link or discovery failures.

| BLE setting | Value |
|---|---|
| Device name | `B8_B4` |
| Service UUID | `0x1848` |
| Control characteristic UUID | `0x2A6E` |
| Roles | Transmitter peripheral/server; receiver central/client |

Control packets use a fixed ASCII format:

```text
L:<left_adc>,R:<right_adc>,B:<button>
```

ADC values span 0-65535. The receiver accepts only the expected fields, order,
separators, ranges, and button values; malformed packets stop the motors without
partially applying a command.

Joystick values map to speeds from approximately -100 to +100 with a -2 to +2
dead zone. Motor duty uses a quadratic response for finer low-speed control:

```text
duty = (abs(speed) / 100)^2 * 65535
```

Production direction polarity is LOW for forward and HIGH for backward, with
independent left/right inversion constants in `common/config.hpp`. The button
state is reported but does not currently trigger a motor or servo action.

The transmitter sends a keepalive at least every **200 ms**. If the receiver
does not receive a fully valid packet for **500 ms**, its nonblocking watchdog
sets both motor PWM outputs to zero. Motors also stop on startup, disconnect,
discovery/subscription failure, malformed data, and internal BLE errors.

Status LEDs toggle approximately every 250 ms while disconnected/searching and
every 1000 ms while connected. The receiver drives both its onboard CYW43 LED
and the GP2 LED; the transmitter uses its onboard CYW43 LED.

## Servo and component tests

The reusable allocation-free Servo class uses approximately 50 Hz PWM with
default duty calibration 1638-7864 over 0-180 degrees. `servo_test` drives GP22
through 0, 90, 180, and 90 degrees at two-second intervals. The servo is not
integrated into production receiver behavior.

| Target | Purpose | UF2 location under `build/` |
|---|---|---|
| `transmitter` | Handheld BLE controller | `transmitter/transmitter.uf2` |
| `receiver` | Boat BLE client and motor control | `receiver/receiver.uf2` |
| `motor_driver_test` | Motor direction and stop sequence | `components/motor_driver_test/motor_driver_test.uf2` |
| `led_test` | GP2 external LED | `components/led_test/led_test.uf2` |
| `joystick_test` | GP27, GP26, and GP22 switch diagnostics | `components/joystick_test/joystick_test.uf2` |
| `button_test` | GP13 active-low button diagnostics | `components/button_test/button_test.uf2` |
| `servo_test` | GP22 servo sweep | `components/servo_test/servo_test.uf2` |

`motor_driver_test` uses HIGH for forward and LOW for backward, while production
firmware uses the polarity described above. Confirm direction on the actual
mechanical installation before fitting propellers.

## Repository structure

```text
common/       Shared protocol, math, configuration, and LED support
transmitter/  BLE peripheral firmware and GATT definition
receiver/     BLE central firmware and motor controller
components/   Isolated hardware tests and Servo class
tests/        Host-side C++ protocol and control-math tests
.vscode/      Pico extension build/debug configuration
```

## Build and flash

With Pico SDK 2.3.0 and Ninja available:

```powershell
cmake -S . -B build -G Ninja `
  -DPICO_BOARD=pico_w `
  -DPython3_EXECUTABLE=C:\Users\User\.pico-sdk\python\3.13.7\python.exe
cmake --build build --parallel
```

In VS Code, select board `pico_w`, run **CMake: Configure**, then **Compile
Project**. Configure with `-DGALLEON_DEBUG=ON` for detailed BLE, ADC, packet,
speed, and PWM diagnostics.

Run the host C++ tests separately with a native compiler:

```powershell
cmake -S tests -B build-host-tests
cmake --build build-host-tests --parallel
ctest --test-dir build-host-tests --output-on-failure
```

To flash a Pico WH, disconnect USB, hold BOOTSEL while reconnecting, release it
when `RPI-RP2` appears, and copy the required UF2 to that drive. Flash
`transmitter.uf2` to the handheld unit and `receiver.uf2` to the vessel.

USB standard I/O is enabled for all targets. Use the VS Code Serial Monitor or
another USB CDC terminal to view connection transitions, rejected packets,
watchdog events, and component-test output.

## Safe test order

1. Build all firmware and run the host tests.
2. With battery and motor power disconnected, run the button, joystick, and LED
   tests.
3. Test the servo from an adequate external supply with a common ground and no
   attached mechanism.
4. Run the motor test with propellers removed or fully guarded.
5. Verify neutral controls, BLE reconnect, malformed-packet stop, disconnect
   stop, and watchdog stop before installing propellers.
6. Check each motor direction at low power and adjust only its inversion flag if
   needed.

## Troubleshooting, safety, and limitations

- **Bluetooth:** verify both production UF2 files, `pico_w`, device name
  `B8_B4`, and the service/characteristic discovery messages over USB.
- **Motors:** verify GP0/GP3 PWM wiring, GP1/GP4 direction wiring, common grounds,
  supply voltage under load, and driver/motor temperature.
- **Controls:** use `joystick_test` to check centers and endpoints before tuning
  the change threshold or dead zone; GP13 should be high when released.
- **Servo:** reduce its calibrated duty range if it buzzes or reaches a
  mechanical stop.

## What I Learned

- How to build a two-board BLE control system using Raspberry Pi Pico WH microcontrollers.
- Why packet validation, automatic reconnection, and a motor watchdog are important for safe embedded systems.
- How joystick ADC readings can be converted into motor direction and PWM speed commands.

## Future Improvements

- Upgrade the firmware to C++20 to experiment with stronger compile-time checks and safer memory-management patterns.
- Add joystick calibration and smoother low-speed motor control.
- Integrate the servo and controller button into a payload-release mechanism.