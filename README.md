# Steve's ESP-IDF LoRaWAN Tracker

This is a simple LoRaWAN tracker built around a Seeed Studio XIAO ESP32S3 & Wio-SX1262 with the L76K GNSS module. It uses OTAA to join The Things Network (TTN) and sends its GPS location every 60 seconds, although it will slow down after a few hours to comply with the TTN fair use policy. The code is written in C & C++ and built using the ESP-IDF framework. The ESP's built-in temperature sensor is also used to send a temperature reading along with the GPS data.

## Hardware

- [Seeed Studio XIAO ESP32S3 & Wio-SX1262 Kit](https://www.seeedstudio.com/Wio-SX1262-with-XIAO-ESP32S3-p-5982.html?sensecap_affiliate=WpDECrz&referring_service=github)
  - The Wio-SX1262 should be attached via the B2B connector, otherwise the GPS module's pins may conflict.

- [Seeed Studio L76K GNSS Module](https://www.seeedstudio.com/L76K-GNSS-Module-for-Seeed-Studio-XIAO-p-5864.html?sensecap_affiliate=WpDECrz&referring_service=github)

## Setup

1. Configure LoRaWAN for your region and TTN device in `include/lorawan_config.hpp`.
2. Upload the code to your ESP32S3 using PlatformIO or the ESP-IDF build system.
3. Add `decoder.js` to the Uplink Payload formatter in the TTN console.
4. Done! Your device should join the network and start sending GPS data.

    The payloads will look something like this when decoded:

    ```json
    {
        "altitude": 29,
        "hdop": 0.7,
        "heading": 293,
        "latitude": -27.4689,
        "longitude": 153.0234,
        "sats": 9,
        "speed": 0,
        "temp1": 30,
        "time": "2026-06-03T04:38:41.000Z"
      }
    ```

    This payload is also compatible with Traccar, I have previously written [a guide](https://stevetech.me/posts/seeedstudio-t1000-traccar#setting-up-the-ttn-webhook-for-traccar) for the T1000 that may be of use for this too.
