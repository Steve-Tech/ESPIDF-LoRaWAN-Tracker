#pragma once

#include <freertos/queue.h>
#include <stdint.h>

#define LATITUDE_MIN 0
#define LATITUDE_MAX ((1 << 22) - 1)
#define LONGITUDE_MIN 0
#define LONGITUDE_MAX ((1 << 23) - 1)
#define ALTITUDE_MIN -1024
#define ALTITUDE_MAX (((1 << 14) - 1) + ALTITUDE_MIN)

struct packet_payload {
    // seconds
    uint32_t timestamp;

    // -- byte 4 --
    // 2°C steps (0 to 63°C)
    uint8_t temperature : 5;

    // -90-90 mapped to LATITUDE_MIN-LATITUDE_MAX
    uint32_t latitude : 22;

    // -180-180 mapped to LONGITUDE_MIN-LONGITUDE_MAX
    uint32_t longitude : 23;

    // meters ALTITUDE_MIN-ALTITUDE_MAX mapped to 0-16383
    uint16_t altitude : 14;

    // -- byte 12 --

    // 1° (0-360)
    uint16_t course : 9;

    // 0.1 km/h (max 204.7)
    uint16_t speed : 11;

    // NMEA specs max of 12 (allow max 15)
    uint8_t sats_in_use : 4;

    // -- byte 15 --

    // hdop 0.1 (max 25.5)
    uint8_t hdop;

    // -- byte 16 --
} __attribute__((packed));

struct packet {
    uint8_t port;
    union {
        struct packet_payload payload;
        uint8_t payload_bytes[sizeof(struct packet_payload)];
    };
} __attribute__((packed));

extern QueueHandle_t packet_queue;
