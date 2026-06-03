#include <freertos/queue.h>
#include <stdint.h>

struct packet {
    // Unix timestamp in seconds
    uint32_t timestamp;

    // lon/lat 0.0001°
    int32_t latitude : 24;
    int32_t longitude : 24;

    // meters (-32768 to 32767)
    int16_t altitude;

    // 1° (0-360)
    uint16_t course : 9;

    // 0.1 km/h (max 204.7)
    uint16_t speed : 11;

    // NMEA specs max of 12
    uint8_t sats_in_use : 4;

    // hdop 0.1 (max 25.5)
    uint8_t hdop;
} __attribute__((packed));

extern QueueHandle_t packet_queue;
