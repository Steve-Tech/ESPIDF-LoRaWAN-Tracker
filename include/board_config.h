#pragma once

#include "board_pins.h"

#define SOC_GPIO_PIN_GNSS_RST   D2
#define SOC_GPIO_PIN_GNSS_WAKE  D0
#define SOC_GPIO_PIN_GNSS_RX    D6
#define SOC_GPIO_PIN_GNSS_TX    D7
#define SOC_GPIO_PIN_LED1       21

/* SPI2 */
#define SOC_GPIO_PIN_MOSI       D10
#define SOC_GPIO_PIN_MISO       D9
#define SOC_GPIO_PIN_SCK        D8

/* SX1262 */
// Source: https://github.com/Seeed-Studio/one_channel_hub/blob/735bb8e9efca9092c9580eeaff2d8678c509e5d5/components/smtc_ral/bsp/sx126x/seeed_xiao_esp32s3_devkit_sx1262.c
#define SOC_GPIO_PIN_SS         41
#define SOC_GPIO_PIN_RST        42
#define SOC_GPIO_PIN_DIO1       39
#define SOC_GPIO_PIN_BUSY       40
/* RF antenna switch */
#define SOC_GPIO_PIN_ANT_RXTX   38 // Tx = HIGH, Rx = LOW
#define SOC_GPIO_PIN_LED2       48

#define GPS_BAUDRATE            115200
