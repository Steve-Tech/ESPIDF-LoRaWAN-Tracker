#pragma once

#include "board_pins.h"

// We can't use some GPS pins as they're shared with the radio
// #define SOC_GPIO_PIN_GNSS_RST   D2
#define SOC_GPIO_PIN_GNSS_RST   -1
// #define SOC_GPIO_PIN_GNSS_WAKE   D0
#define SOC_GPIO_PIN_GNSS_WAKE  -1
#define SOC_GPIO_PIN_GNSS_RX    D6
#define SOC_GPIO_PIN_GNSS_TX    D7

/* SPI2 */
#define SOC_GPIO_PIN_MOSI       D10
#define SOC_GPIO_PIN_MISO       D9
#define SOC_GPIO_PIN_SCK        D8
#define SOC_GPIO_PIN_SS         D3

/* SX1262 */
#define SOC_GPIO_PIN_RST        D2
#define SOC_GPIO_PIN_DIO1       D0
#define SOC_GPIO_PIN_BUSY       D1

/* RF antenna switch */
#define SOC_GPIO_PIN_ANT_RXTX   D4 // Tx = HIGH, Rx = LOW

#define GPS_BAUDRATE            115200
