#include <stdint.h>
#include <freertos/queue.h>

struct packet {
    // Unix timestamp in seconds
    uint32_t timestamp;
    // 3 byte lon/lat 0.0001°
    int32_t latitude : 24;
    int32_t longitude : 24;
    // 2 byte altitude in meters
    uint16_t altitude;
    // 1°
    uint16_t course : 9;
    // 0.1 km/h
    uint16_t speed : 15;
    uint8_t sats_in_use;
    // 1 byte hdop 0.1
    uint8_t hdop;
} __attribute__((packed));

extern QueueHandle_t packet_queue;
