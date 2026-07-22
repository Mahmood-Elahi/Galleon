#include <cstdint>
#include <cstdio>
#include <cstring>

#include "btstack.h"
#include "config.hpp"
#include "control_math.hpp"
#include "control_protocol.hpp"
#include "motor_controller.hpp"
#include "hardware/sync.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "status_led.hpp"

namespace {

enum class ReceiverState {
    idle,
    scanning,
    connecting,
    ready_service_query,
    waiting_service,
    ready_characteristic_query,
    waiting_characteristic,
    ready_subscription,
    waiting_subscription,
    connected,
};

ReceiverState state = ReceiverState::idle;
hci_con_handle_t connection_handle = HCI_CON_HANDLE_INVALID;
bd_addr_t transmitter_address{};
bd_addr_type_t transmitter_address_type = BD_ADDR_TYPE_UNKNOWN;
gatt_client_service_t control_service{};
gatt_client_characteristic_t control_characteristic{};
gatt_client_notification_t notification_listener{};
btstack_context_callback_registration_t query_request{};
btstack_packet_callback_registration_t hci_event_registration{};
btstack_timer_source_t maintenance_timer{};
bool listener_registered = false;
bool service_found = false;
bool characteristic_found = false;
bool valid_command_received = false;
bool watchdog_reported = false;
bool button_known = false;
bool last_button_pressed = false;
std::uint32_t last_valid_command_ms = 0;

galleon::MotorController motors(
    galleon::config::kLeftMotorEnablePin,
    galleon::config::kLeftMotorDirectionPin,
    galleon::config::kRightMotorEnablePin,
    galleon::config::kRightMotorDirectionPin,
    galleon::config::kLeftMotorDirectionInverted,
    galleon::config::kRightMotorDirectionInverted);
galleon::StatusLed status_led(true, galleon::config::kExternalGreenLedPin);

void start_scanning();
void request_next_query();
void gatt_event_handler(std::uint8_t packet_type, std::uint16_t channel,
                        std::uint8_t* packet, std::uint16_t size);

void stop_motors() {
    const bool was_running = motors.left_duty() != 0 || motors.right_duty() != 0;
    motors.stop();
    if (was_running) std::printf("Motors stopped\n");
}

void reset_connection_state() {
    valid_command_received = false;
    watchdog_reported = false;
    button_known = false;
    service_found = false;
    characteristic_found = false;
    control_service = {};
    control_characteristic = {};
}

void fail_connection(const char* reason) {
    std::printf("BLE error: %s\n", reason);
    stop_motors();
    reset_connection_state();
    if (connection_handle != HCI_CON_HANDLE_INVALID) {
        state = ReceiverState::idle;
        gap_disconnect(connection_handle);
    } else {
        start_scanning();
    }
}

bool advertisement_matches(std::uint8_t length, const std::uint8_t* data) {
    bool name_matches = false;
    bool service_matches = false;
    ad_context_t iterator;
    for (ad_iterator_init(&iterator, length, data); ad_iterator_has_more(&iterator);
         ad_iterator_next(&iterator)) {
        const std::uint8_t type = ad_iterator_get_data_type(&iterator);
        const std::uint8_t field_length = ad_iterator_get_data_len(&iterator);
        const std::uint8_t* field = ad_iterator_get_data(&iterator);
        if ((type == BLUETOOTH_DATA_TYPE_SHORTENED_LOCAL_NAME ||
             type == BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME) &&
            field_length == std::strlen(galleon::config::kDeviceName) &&
            std::memcmp(field, galleon::config::kDeviceName, field_length) == 0) {
            name_matches = true;
        }
        if (type == BLUETOOTH_DATA_TYPE_INCOMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS ||
            type == BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS) {
            for (std::uint8_t offset = 0; offset + 1U < field_length; offset += 2U) {
                if (little_endian_read_16(field, offset) == galleon::config::kServiceUuid) {
                    service_matches = true;
                }
            }
        }
    }
    return name_matches && service_matches;
}

void start_scanning() {
    stop_motors();
    reset_connection_state();
    connection_handle = HCI_CON_HANDLE_INVALID;
    state = ReceiverState::scanning;
    gap_set_scan_parameters(1, 0x0030, 0x0030);
    gap_start_scan();
    std::printf("Scanning\n");
}

void handle_control_notification(const std::uint8_t* data, std::uint16_t length) {
    galleon::ControlPacket packet{};
    const auto result = galleon::parse_control_packet(
        reinterpret_cast<const char*>(data), length, packet);
    if (result != galleon::ParseResult::ok) {
        std::printf("Packet rejected (%s)\n", galleon::parse_result_name(result));
        stop_motors();
        return;
    }

    const int left_speed = galleon::protocol_adc_to_speed(
        packet.left_adc, galleon::config::kDeadzoneLow,
        galleon::config::kDeadzoneHigh, galleon::config::kMaxSpeed);
    const int right_speed = galleon::protocol_adc_to_speed(
        packet.right_adc, galleon::config::kDeadzoneLow,
        galleon::config::kDeadzoneHigh, galleon::config::kMaxSpeed);
    motors.set_speeds(left_speed, right_speed);
    last_valid_command_ms = btstack_run_loop_get_time_ms();
    valid_command_received = true;
    watchdog_reported = false;

    if (!button_known || packet.button_pressed != last_button_pressed) {
        button_known = true;
        last_button_pressed = packet.button_pressed;
        std::printf("Button %s\n", packet.button_pressed ? "pressed" : "released");
        // Extension point: assign a future button action here after hardware requirements exist.
    }

#if GALLEON_DEBUG
    std::printf("RX L=%u R=%u B=%u speed=%d,%d pwm=%u,%u\n",
                packet.left_adc, packet.right_adc,
                packet.button_pressed ? 1U : 0U, left_speed, right_speed,
                motors.left_duty(), motors.right_duty());
#endif
}

void send_next_query(void*) {
    std::uint8_t status = ERROR_CODE_SUCCESS;
    switch (state) {
        case ReceiverState::ready_service_query:
            state = ReceiverState::waiting_service;
            status = gatt_client_discover_primary_services_by_uuid16(
                gatt_event_handler, connection_handle, galleon::config::kServiceUuid);
            break;
        case ReceiverState::ready_characteristic_query:
            state = ReceiverState::waiting_characteristic;
            status = gatt_client_discover_characteristics_for_service_by_uuid16(
                gatt_event_handler, connection_handle, &control_service,
                galleon::config::kControlCharacteristicUuid);
            break;
        case ReceiverState::ready_subscription:
            state = ReceiverState::waiting_subscription;
            status = gatt_client_write_client_characteristic_configuration(
                gatt_event_handler, connection_handle, &control_characteristic,
                GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION);
            break;
        default:
            return;
    }
    if (status != ERROR_CODE_SUCCESS) fail_connection("could not start GATT operation");
}

void request_next_query() {
    query_request.callback = send_next_query;
    query_request.context = nullptr;
    const std::uint8_t status =
        gatt_client_request_to_send_gatt_query(&query_request, connection_handle);
    if (status != ERROR_CODE_SUCCESS) fail_connection("GATT query scheduling failed");
}

void gatt_event_handler(std::uint8_t packet_type, std::uint16_t,
                        std::uint8_t* packet, std::uint16_t) {
    if (packet_type != HCI_EVENT_PACKET) return;
    const std::uint8_t event_type = hci_event_packet_get_type(packet);

    if (event_type == GATT_EVENT_NOTIFICATION) {
        if (state != ReceiverState::connected ||
            gatt_event_notification_get_value_handle(packet) != control_characteristic.value_handle) {
            return;
        }
        handle_control_notification(gatt_event_notification_get_value(packet),
                                    gatt_event_notification_get_value_length(packet));
        return;
    }

    switch (state) {
        case ReceiverState::waiting_service:
            if (event_type == GATT_EVENT_SERVICE_QUERY_RESULT) {
                gatt_event_service_query_result_get_service(packet, &control_service);
                service_found = true;
                std::printf("Service found\n");
            } else if (event_type == GATT_EVENT_QUERY_COMPLETE) {
                if (gatt_event_query_complete_get_att_status(packet) != ATT_ERROR_SUCCESS ||
                    !service_found) {
                    fail_connection("service discovery failed");
                    return;
                }
                state = ReceiverState::ready_characteristic_query;
                request_next_query();
            }
            break;
        case ReceiverState::waiting_characteristic:
            if (event_type == GATT_EVENT_CHARACTERISTIC_QUERY_RESULT) {
                gatt_event_characteristic_query_result_get_characteristic(
                    packet, &control_characteristic);
                characteristic_found = true;
                std::printf("Characteristic found\n");
            } else if (event_type == GATT_EVENT_QUERY_COMPLETE) {
                if (gatt_event_query_complete_get_att_status(packet) != ATT_ERROR_SUCCESS ||
                    !characteristic_found) {
                    fail_connection("characteristic discovery failed");
                    return;
                }
                gatt_client_listen_for_characteristic_value_updates(
                    &notification_listener, gatt_event_handler, connection_handle,
                    &control_characteristic);
                listener_registered = true;
                state = ReceiverState::ready_subscription;
                request_next_query();
            }
            break;
        case ReceiverState::waiting_subscription:
            if (event_type == GATT_EVENT_QUERY_COMPLETE) {
                if (gatt_event_query_complete_get_att_status(packet) != ATT_ERROR_SUCCESS) {
                    fail_connection("notification subscription failed");
                    return;
                }
                state = ReceiverState::connected;
                std::printf("Control subscription enabled\n");
            }
            break;
        default:
            break;
    }
}

void hci_event_handler(std::uint8_t packet_type, std::uint16_t,
                       std::uint8_t* packet, std::uint16_t) {
    if (packet_type != HCI_EVENT_PACKET) return;
    switch (hci_event_packet_get_type(packet)) {
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) start_scanning();
            break;
        case GAP_EVENT_ADVERTISING_REPORT: {
            if (state != ReceiverState::scanning) break;
            const std::uint8_t length = gap_event_advertising_report_get_data_length(packet);
            const std::uint8_t* data = gap_event_advertising_report_get_data(packet);
            if (!advertisement_matches(length, data)) break;
            gap_event_advertising_report_get_address(packet, transmitter_address);
            transmitter_address_type = static_cast<bd_addr_type_t>(
                gap_event_advertising_report_get_address_type(packet));
            gap_stop_scan();
            state = ReceiverState::connecting;
            std::printf("Device found\nConnecting\n");
            if (gap_connect(transmitter_address, transmitter_address_type) != ERROR_CODE_SUCCESS) {
                fail_connection("connection request failed");
            }
            break;
        }
        case HCI_EVENT_META_GAP:
            if (hci_event_gap_meta_get_subevent_code(packet) !=
                GAP_SUBEVENT_LE_CONNECTION_COMPLETE || state != ReceiverState::connecting) {
                break;
            }
            if (gap_subevent_le_connection_complete_get_status(packet) != ERROR_CODE_SUCCESS) {
                fail_connection("connection failed");
                break;
            }
            connection_handle = gap_subevent_le_connection_complete_get_connection_handle(packet);
            state = ReceiverState::ready_service_query;
            request_next_query();
            break;
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            stop_motors();
            if (listener_registered) {
                gatt_client_stop_listening_for_characteristic_value_updates(
                    &notification_listener);
                listener_registered = false;
            }
            connection_handle = HCI_CON_HANDLE_INVALID;
            std::printf("Receiver disconnected\n");
            start_scanning();
            break;
        default:
            break;
    }
}

void maintenance(btstack_timer_source_t* timer) {
    const std::uint32_t now = btstack_run_loop_get_time_ms();
    const bool fully_connected = state == ReceiverState::connected;
    status_led.update(now, fully_connected);
    if (fully_connected && valid_command_received &&
        static_cast<std::uint32_t>(now - last_valid_command_ms) >=
            galleon::config::kCommandWatchdogMs) {
        stop_motors();
        valid_command_received = false;
        if (!watchdog_reported) {
            watchdog_reported = true;
            std::printf("Command watchdog expired\n");
        }
    }
    btstack_run_loop_set_timer(timer, 50);
    btstack_run_loop_add_timer(timer);
}

}  // namespace

int main() {
    stdio_init_all();
    std::printf("Receiver starting\n");
    motors.init();
    std::printf("Motors stopped\n");

    if (cyw43_arch_init() != 0) {
        std::printf("CYW43 initialization failed\n");
        motors.stop();
        return 1;
    }
    status_led.init();

    l2cap_init();
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    gatt_client_init();

    hci_event_registration.callback = hci_event_handler;
    hci_add_event_handler(&hci_event_registration);

    maintenance_timer.process = maintenance;
    btstack_run_loop_set_timer(&maintenance_timer, 50);
    btstack_run_loop_add_timer(&maintenance_timer);

    hci_power_control(HCI_POWER_ON);
    while (true) __wfi();
}
