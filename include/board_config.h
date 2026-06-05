#pragma once

/* Derived from https://github.com/meshtastic/firmware/blob/v2.7.20.6658ec2/variants/esp32s3/seeed_xiao_s3/variant.h */

#define BOARD_LED 21

#define GPS_RESET_PIN 3
#define GPS_WAKE_PIN 1
#define GPS_RX_PIN 43
#define GPS_TX_PIN 44
#define GPS_BAUDRATE 9600

/* SPI2 */
#define LORA_MOSI 9
#define LORA_MISO 8
#define LORA_SCK 7

/* SX1262 */
#define SX126X_CS 41
#define SX126X_DIO1 39
#define SX126X_BUSY 40
#define SX126X_RESET 42
#define SX126X_RXEN 38
#define SX126X_TXEN RADIOLIB_NC
#define SX126X_DIO3_TCXO_VOLTAGE 1.8
#define SX126X_LED 48
