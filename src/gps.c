#include <driver/gpio.h>
#include <driver/uart.h>
#include <esp_log.h>
#include <time.h>

#include <lwgps/lwgps.h>

#include "board_config.h"
#include "packet.h"

static const char* TAG = "gps";

#define GPS_POLL_INTERVAL_MS 60 * 1000 // 1 minute
#define MAX_SKIP_COUNT 5
// "Messages have a maximum length of 82 characters" - Wikipedia
#define NMEA_MAX_LENGTH 82
static QueueHandle_t uart0_queue;

lwgps_t hgps;

static time_t gps_to_unix_time(lwgps_t* gps) {
    struct tm timeinfo = {
        .tm_sec = gps->seconds,
        .tm_min = gps->minutes,
        .tm_hour = gps->hours,
        .tm_mday = gps->date,
        .tm_mon = gps->month - 1,   // tm_mon is 0-11
        .tm_year = gps->year + 100, // tm_year is years since 1900
    };
    return mktime(&timeinfo);
}

static void gps_queue_timer(TimerHandle_t xTimer) {
    // Unblock the GPS task to check if we should send a packet
    xTaskNotifyGive((TaskHandle_t)pvTimerGetTimerID(xTimer));
}

void gps_task(void* pvParameters) {
    ESP_LOGI(TAG, "Initialising...");

    gpio_config_t conf = {
        .pin_bit_mask =
            (1ULL << SOC_GPIO_PIN_GNSS_RST) | (1ULL << SOC_GPIO_PIN_GNSS_WAKE),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&conf);

    gpio_set_level((gpio_num_t)SOC_GPIO_PIN_GNSS_RST, 0);
    gpio_set_level((gpio_num_t)SOC_GPIO_PIN_GNSS_WAKE, 1);

    uart_config_t uart_config = {
        .baud_rate = GPS_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT,
        .flags =
            {
                .allow_pd = 0,
                .backup_before_sleep = 0,
            },
    };
    uart_driver_install(UART_NUM_0, NMEA_MAX_LENGTH * 32, 0, 16, &uart0_queue,
                        0);
    uart_param_config(UART_NUM_0, &uart_config);
    // uart_set_pin(UART_NUM_0, SOC_GPIO_PIN_GNSS_RX, SOC_GPIO_PIN_GNSS_TX, -1,
    //              -1);

    // Trigger on newlines, which is the end of a NMEA message
    uart_enable_pattern_det_baud_intr(UART_NUM_0, '\n', 1, 9, 0, 0);
    uart_pattern_queue_reset(UART_NUM_0, NMEA_MAX_LENGTH);

    lwgps_init(&hgps);

    gpio_set_level((gpio_num_t)SOC_GPIO_PIN_GNSS_RST, 1);

    uint8_t data[NMEA_MAX_LENGTH + 1];
    uart_event_t event;
    size_t buffered_size;
    struct packet pkt;
    lwgps_float_t last_latitude = 0.0;
    lwgps_float_t last_longitude = 0.0;
    lwgps_float_t distance_travelled = 0.0;
    size_t skip_count = SIZE_MAX;

    TimerHandle_t timer = xTimerCreate(
        "gps_queue_timer", pdMS_TO_TICKS(GPS_POLL_INTERVAL_MS), pdTRUE,
        (void*)xTaskGetCurrentTaskHandle(), gps_queue_timer);
    xTimerStart(timer, 0);

    ESP_LOGI(TAG, "Ready");

    for (;;) {
        if (xQueueReceive(uart0_queue, &event, portMAX_DELAY) == pdTRUE) {
            if (event.type != UART_PATTERN_DET) {
                if (event.type != UART_DATA) {
                    ESP_LOGW(TAG, "Unexpected UART event type: %d", event.type);
                }
                continue;
            }

            uart_get_buffered_data_len(UART_NUM_0, &buffered_size);
            int pos = uart_pattern_pop_pos(UART_NUM_0);
            if (pos == -1) {
                ESP_LOGW(TAG, "Pattern not found in UART buffer");
                continue;
            }
            // pos + 1 to include the newline character
            if (++pos > NMEA_MAX_LENGTH) {
                ESP_LOGW(TAG,
                         "Received NMEA message exceeds maximum length: %d",
                         pos);
                // Flush the buffer to avoid getting stuck on this message
                uart_flush_input(UART_NUM_0);
                continue;
            }
            uart_read_bytes(UART_NUM_0, data, pos, 100 / portTICK_PERIOD_MS);
            // ESP_LOGI(TAG, "read data: %s", pos, data);

            lwgps_process(&hgps, data, pos);

            if (ulTaskNotifyTake(pdTRUE, 0) == 0 || packet_queue == NULL) {
                // Timer hasn't fired yet, or the queue isn't ready
                continue;
            }

            ESP_LOGI(TAG, "is_valid: %d", hgps.is_valid);

            if (!lwgps_is_valid(&hgps)) {
                ESP_LOGW(TAG, "GPS data is not valid, skipping...");
                continue;
            }

            ESP_LOGI(TAG, "latitude: %f", hgps.latitude);
            ESP_LOGI(TAG, "longitude: %f", hgps.longitude);
            ESP_LOGI(TAG, "altitude: %f", hgps.altitude);

            pkt.timestamp = (uint32_t)gps_to_unix_time(&hgps);
            pkt.latitude = (int32_t)(hgps.latitude * 10000);
            pkt.longitude = (int32_t)(hgps.longitude * 10000);
            pkt.altitude = (uint16_t)hgps.altitude;
            pkt.course = (uint16_t)(hgps.course);
            pkt.speed =
                (uint16_t)(lwgps_to_speed(hgps.speed, LWGPS_SPEED_KPH) * 10);
            pkt.sats_in_use = hgps.sats_in_use;
            pkt.hdop = (uint8_t)(hgps.dop_h * 10);

            if (skip_count <= MAX_SKIP_COUNT) {
                if (!lwgps_distance_bearing(last_latitude, last_longitude,
                                            hgps.latitude, hgps.longitude,
                                            &distance_travelled, NULL)) {
                    ESP_LOGW(TAG, "Failed to calculate distance travelled");
                    distance_travelled = 0.0;
                }
                ESP_LOGI(TAG, "Distance travelled since last packet: %f meters",
                         distance_travelled);
            }

            // Prioritise sending newer packets if we're moving
            if (skip_count > MAX_SKIP_COUNT || distance_travelled >= 10.0) {
                if (xQueueSendToFront(packet_queue, &pkt, pdMS_TO_TICKS(100)) !=
                    pdTRUE) {
                    ESP_LOGW(TAG, "Failed to send packet to front of queue");
                } else {
                    ESP_LOGI(TAG, "Packet sent to front of queue");
                }
            } else {
                if (xQueueSendToBack(packet_queue, &pkt, pdMS_TO_TICKS(100)) !=
                    pdTRUE) {
                    ESP_LOGW(TAG, "Failed to send packet to back of queue");
                } else {
                    ESP_LOGI(TAG, "Packet sent to back of queue");
                }
                skip_count++;
            }

            last_latitude = hgps.latitude;
            last_longitude = hgps.longitude;
        }
    }
}
