#include <cstdint>
#include <cstdio>
#include <cstring>

#include "btstack.h"
#include "config.hpp"
#include "control_math.hpp"
#include "control_protocol.hpp"
#include "hardware/adc.h"
#include "hardware/sync.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "status_led.hpp"
#include "transmitter.h"

namespace {

using galleon::ControlPacket;

hci_con_handle_t connection_handle = HCI_CON_HANDLE_INVALID;
bool connected = false;
bool notifications_enabled = false;
bool notification_pending = false;
ControlPacket current_packet{};
ControlPacket last_sent_packet{};
bool has_sent_packet = false;
char control_value[galleon::kControlPacketBufferSize] = "L:0,R:0,B:0";
std::size_t control_value_length = 13;
std::uint32_t last_sent_ms = 0;

btstack_packet_callback_registration_t hci_event_registration{};
btstack_timer_source_t control_timer{};
btstack_timer_source_t led_timer{};
galleon::StatusLed status_led;

constexpr std::uint8_t advertising_data[] = {
    0x02, BLUETOOTH_DATA_TYPE_FLAGS, 0x06,
    0x03, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, 0x48, 0x18,
    0x03, BLUETOOTH_DATA_TYPE_APPEARANCE, 0x80, 0x01,
    0x06, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'B', '8', '_', 'B', '4',
};

bool changed_enough(std::uint16_t value, std::uint16_t previous) {
    const std::uint16_t difference = value > previous ? value - previous : previous - value;
    return difference >= galleon::config::kJoystickChangeThreshold;
}

void start_advertising() {
    constexpr std::uint16_t interval_units =
        static_cast<std::uint16_t>((galleon::config::kAdvertisingIntervalMs * 1000U) / 625U);
    bd_addr_t null_address{};
    gap_advertisements_set_params(interval_units, interval_units, 0, 0,
                                  null_address, 0x07, 0x00);
    gap_advertisements_set_data(sizeof(advertising_data),
                                const_cast<std::uint8_t*>(advertising_data));
    gap_advertisements_enable(1);
    std::printf("Advertising\n");
}

std::uint16_t read_adc(unsigned int input) {
    adc_select_input(input);
    return galleon::adc12_to_protocol(adc_read());
}

void sample_controls(btstack_timer_source_t* timer) {
    current_packet.left_adc = read_adc(1);   // GP27 / ADC1
    current_packet.right_adc = read_adc(0);  // GP26 / ADC0
    current_packet.button_pressed = !gpio_get(galleon::config::kControllerButtonPin);
    galleon::format_control_packet(current_packet, control_value,
                                    sizeof(control_value), control_value_length);

    const std::uint32_t now = btstack_run_loop_get_time_ms();
    const bool changed = !has_sent_packet ||
        changed_enough(current_packet.left_adc, last_sent_packet.left_adc) ||
        changed_enough(current_packet.right_adc, last_sent_packet.right_adc) ||
        current_packet.button_pressed != last_sent_packet.button_pressed;
    const bool keepalive_due = !has_sent_packet ||
        static_cast<std::uint32_t>(now - last_sent_ms) >=
            galleon::config::kControlKeepaliveIntervalMs;

    if (connected && notifications_enabled && !notification_pending &&
        (changed || keepalive_due)) {
        notification_pending = true;
        att_server_request_can_send_now_event(connection_handle);
    }

#if GALLEON_DEBUG
    std::printf("ADC L=%u R=%u B=%u\n", current_packet.left_adc,
                current_packet.right_adc, current_packet.button_pressed ? 1U : 0U);
#endif
    btstack_run_loop_set_timer(timer, galleon::config::kControlSampleIntervalMs);
    btstack_run_loop_add_timer(timer);
}

void update_led(btstack_timer_source_t* timer) {
    status_led.update(btstack_run_loop_get_time_ms(), connected);
    btstack_run_loop_set_timer(timer, 50);
    btstack_run_loop_add_timer(timer);
}

std::uint16_t att_read_callback(hci_con_handle_t, std::uint16_t attribute_handle,
                                std::uint16_t offset, std::uint8_t* buffer,
                                std::uint16_t buffer_size) {
    if (attribute_handle != ATT_CHARACTERISTIC_2A6E_01_VALUE_HANDLE) {
        return 0;
    }
    return att_read_callback_handle_blob(
        reinterpret_cast<const std::uint8_t*>(control_value),
        static_cast<std::uint16_t>(control_value_length), offset, buffer, buffer_size);
}

int att_write_callback(hci_con_handle_t handle, std::uint16_t attribute_handle,
                       std::uint16_t transaction_mode, std::uint16_t offset,
                       std::uint8_t* buffer, std::uint16_t buffer_size) {
    (void)transaction_mode;
    if (attribute_handle !=
        ATT_CHARACTERISTIC_2A6E_01_CLIENT_CONFIGURATION_HANDLE) {
        return 0;
    }
    if (offset != 0 || buffer_size != 2) return ATT_ERROR_INVALID_ATTRIBUTE_VALUE_LENGTH;
    const std::uint16_t configuration = little_endian_read_16(buffer, 0);
    notifications_enabled =
        configuration == GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION;
    connection_handle = handle;
    notification_pending = false;
    has_sent_packet = false;
    std::printf(notifications_enabled ? "Notifications enabled\n" : "Notifications disabled\n");
    return 0;
}

void packet_handler(std::uint8_t packet_type, std::uint16_t,
                    std::uint8_t* packet, std::uint16_t) {
    if (packet_type != HCI_EVENT_PACKET) return;
    switch (hci_event_packet_get_type(packet)) {
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) start_advertising();
            break;
        case HCI_EVENT_META_GAP:
            if (hci_event_gap_meta_get_subevent_code(packet) == GAP_SUBEVENT_LE_CONNECTION_COMPLETE &&
                gap_subevent_le_connection_complete_get_status(packet) == ERROR_CODE_SUCCESS) {
                connection_handle = gap_subevent_le_connection_complete_get_connection_handle(packet);
                connected = true;
                notifications_enabled = false;
                notification_pending = false;
                has_sent_packet = false;
                std::printf("Receiver connected\n");
            }
            break;
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            connected = false;
            notifications_enabled = false;
            notification_pending = false;
            has_sent_packet = false;
            connection_handle = HCI_CON_HANDLE_INVALID;
            std::printf("Receiver disconnected\n");
            start_advertising();
            break;
        case ATT_EVENT_CAN_SEND_NOW:
            if (!notification_pending || !connected || !notifications_enabled) break;
            notification_pending = false;
            if (att_server_notify(connection_handle,
                                  ATT_CHARACTERISTIC_2A6E_01_VALUE_HANDLE,
                                  reinterpret_cast<std::uint8_t*>(control_value),
                                  static_cast<std::uint16_t>(control_value_length)) == ERROR_CODE_SUCCESS) {
                last_sent_packet = current_packet;
                last_sent_ms = btstack_run_loop_get_time_ms();
                has_sent_packet = true;
#if GALLEON_DEBUG
                std::printf("TX %s\n", control_value);
#endif
            }
            break;
        default:
            break;
    }
}

}  // namespace

int main() {
    stdio_init_all();
    std::printf("Transmitter starting\n");
    if (cyw43_arch_init() != 0) {
        std::printf("CYW43 initialization failed\n");
        return 1;
    }

    status_led.init();
    adc_init();
    adc_gpio_init(galleon::config::kLeftJoystickPin);
    adc_gpio_init(galleon::config::kRightJoystickPin);
    gpio_init(galleon::config::kControllerButtonPin);
    gpio_set_dir(galleon::config::kControllerButtonPin, GPIO_IN);
    gpio_pull_up(galleon::config::kControllerButtonPin);

    l2cap_init();
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    att_server_init(profile_data, att_read_callback, att_write_callback);

    hci_event_registration.callback = packet_handler;
    hci_add_event_handler(&hci_event_registration);
    att_server_register_packet_handler(packet_handler);

    control_timer.process = sample_controls;
    btstack_run_loop_set_timer(&control_timer, galleon::config::kControlSampleIntervalMs);
    btstack_run_loop_add_timer(&control_timer);
    led_timer.process = update_led;
    btstack_run_loop_set_timer(&led_timer, 50);
    btstack_run_loop_add_timer(&led_timer);

    hci_power_control(HCI_POWER_ON);
    while (true) __wfi();
}
