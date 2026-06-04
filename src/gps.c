#include <driver/gpio.h>
#include <driver/temperature_sensor.h>
#include <driver/uart.h>
#include <esp_log.h>
#include <time.h>

#include <lwgps/lwgps.h>

#include "board_config.h"
#include "macros.h"
#include "packet.h"

static const char* TAG = "gps";

#define GPS_SEND_INTERVAL_MS 60 * 1000 // 1 minute
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

static uint32_t map_coords_to_pkt(lwgps_float_t x, ssize_t in_min,
                                  ssize_t in_max, size_t out_min,
                                  size_t out_max) {
    const size_t run = in_max - in_min;
    const size_t rise = out_max - out_min;
    const lwgps_float_t delta = x - in_min;
    return (delta * rise) / run + out_min;
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
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&conf));

    ESP_ERROR_CHECK_WITHOUT_ABORT(
        gpio_set_level((gpio_num_t)SOC_GPIO_PIN_GNSS_RST, 0));
    ESP_ERROR_CHECK_WITHOUT_ABORT(
        gpio_set_level((gpio_num_t)SOC_GPIO_PIN_GNSS_WAKE, 1));

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
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, NMEA_MAX_LENGTH * 32, 0, 16,
                                        &uart0_queue, 0));
    ESP_ERROR_CHECK(uart0_queue == NULL ? ESP_FAIL : ESP_OK);
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));
    // ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, SOC_GPIO_PIN_GNSS_RX,
    //                              SOC_GPIO_PIN_GNSS_TX, -1, -1));

    // Trigger on newlines, which is the end of a NMEA message
    ESP_ERROR_CHECK(
        uart_enable_pattern_det_baud_intr(UART_NUM_0, '\n', 1, 9, 0, 0));
    ESP_ERROR_CHECK(uart_pattern_queue_reset(UART_NUM_0, NMEA_MAX_LENGTH));

    lwgps_init(&hgps);

    ESP_ERROR_CHECK_WITHOUT_ABORT(
        gpio_set_level((gpio_num_t)SOC_GPIO_PIN_GNSS_RST, 1));

    ESP_LOGI(TAG, "Configuring temperature sensor...");
    temperature_sensor_handle_t temp_sensor = NULL;
    temperature_sensor_config_t temp_sensor_config =
        TEMPERATURE_SENSOR_CONFIG_DEFAULT(0, 63);
    ESP_ERROR_CHECK_WITHOUT_ABORT(
        temperature_sensor_install(&temp_sensor_config, &temp_sensor));
    if (temp_sensor != NULL)
        ESP_ERROR_CHECK_WITHOUT_ABORT(temperature_sensor_enable(temp_sensor));

    lwgps_float_t last_latitude = 0.0;
    lwgps_float_t last_longitude = 0.0;
    lwgps_float_t distance_travelled = 0.0;
    size_t skip_count = SIZE_MAX;
    TickType_t last_queue_time = 0;

    ESP_LOGI(TAG, "Ready");

    for (;;) {
        uart_event_t event;
        if (xQueueReceive(uart0_queue, &event, portMAX_DELAY) == pdFALSE) {
            ESP_LOGW(TAG, "Failed to receive UART event");
            continue;
        }

        if (event.type != UART_PATTERN_DET) {
            if (event.type != UART_DATA) {
                ESP_LOGW(TAG, "Unexpected UART event type: %d", event.type);
            }
            continue;
        }

        int pos = uart_pattern_pop_pos(UART_NUM_0);
        if (pos == -1) {
            ESP_LOGW(TAG, "Pattern not found in UART buffer");
            continue;
        }
        // pos + 1 to include the newline character
        if (++pos > NMEA_MAX_LENGTH) {
            ESP_LOGW(TAG, "Received NMEA message exceeds maximum length: %d",
                     pos);
            // Flush the buffer to avoid getting stuck on this message
            ESP_ERROR_CHECK(uart_flush_input(UART_NUM_0));
            continue;
        }

        uint8_t data[NMEA_MAX_LENGTH + 1];
        uart_read_bytes(UART_NUM_0, data, pos, 100 / portTICK_PERIOD_MS);

        lwgps_process(&hgps, data, pos);

        if (!lwgps_is_valid(&hgps) || hgps.fix == 0 || packet_queue == NULL) {
            // Skip sending invalid data, or if the queue isn't ready yet
            continue;
        }

        TickType_t currentTicks = xTaskGetTickCount();
        if (currentTicks - last_queue_time <
            pdMS_TO_TICKS(GPS_SEND_INTERVAL_MS)) {
            // Time hasn't passed yet
            continue;
        }

        uint_fast8_t pkt_temp_value;
        float temp_value;
        if (temp_sensor != NULL && temperature_sensor_get_celsius(
                                       temp_sensor, &temp_value) == ESP_OK) {
            pkt_temp_value = (uint_fast8_t)(temp_value) >> 1;
            ESP_LOGI(TAG, "Temperature: %f °C", temp_value);
        } else {
            pkt_temp_value = 0;
            ESP_LOGW(TAG, "Failed to read temperature sensor");
        }

        ESP_LOGI(TAG, "Latitude: %f", hgps.latitude);
        ESP_LOGI(TAG, "Longitude: %f", hgps.longitude);
        ESP_LOGI(TAG, "Altitude: %f", hgps.altitude);
        ESP_LOGI(TAG, "Course: %f", hgps.course);
        ESP_LOGI(TAG, "Speed: %f", hgps.speed);
        ESP_LOGI(TAG, "Sats in use: %d", hgps.sats_in_use);
        ESP_LOGI(TAG, "HDOP: %f", hgps.dop_h);

        uint16_t pkt_speed_value =
            (uint16_t)(lwgps_to_speed(hgps.speed, LWGPS_SPEED_KPH) * 10);

        struct packet pkt = {
            // Use the port for fix type, should never be 0
            .port = MIN(hgps.fix & 0x07, 1),
            .payload = {
                .timestamp = (uint32_t)gps_to_unix_time(&hgps),

                .temperature = pkt_temp_value,

                .latitude = map_coords_to_pkt(hgps.latitude, -90, 90,
                                              LATITUDE_MIN, LATITUDE_MAX),

                .longitude = map_coords_to_pkt(hgps.longitude, -180, 180,
                                               LONGITUDE_MIN, LONGITUDE_MAX),

                .altitude = CONSTRAIN((int16_t)hgps.altitude, ALTITUDE_MIN,
                                      ALTITUDE_MAX) -
                            ALTITUDE_MIN,

                .course = (uint16_t)hgps.course,

                .speed = MIN(pkt_speed_value, 2047),

                .sats_in_use = MIN(hgps.sats_in_use, 15),

                .hdop = MIN((uint8_t)(hgps.dop_h * 10), 255)}};

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
            skip_count = 0;
        } else {
            if (xQueueSendToBack(packet_queue, &pkt, pdMS_TO_TICKS(100)) !=
                pdTRUE) {
                ESP_LOGW(TAG, "Failed to send packet to back of queue");
            } else {
                ESP_LOGI(TAG, "Packet sent to back of queue");
            }
            skip_count++;
        }

        last_queue_time = currentTicks;
        last_latitude = hgps.latitude;
        last_longitude = hgps.longitude;
    }
}
