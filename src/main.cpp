/*
   RadioLib Non-Arduino ESP-IDF Example

   This example shows how to use RadioLib without Arduino.
   In this case, a Liligo T-BEAM (ESP32 and SX1276)
   is used.

   Can be used as a starting point to port RadioLib to any platform!
   See this API reference page for details on the RadioLib hardware abstraction
   https://jgromes.github.io/RadioLib/class_hal.html

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

#include "EspHal.h"
#include "lorawan_config.h"

static const char* TAG = "main";

// the entry point for the program
// it must be declared as "extern C" because the compiler assumes this will be a
// C function
extern "C" void app_main(void) {
    hal->pinMode(SOC_GPIO_PIN_RST, hal->GpioModeOutput);
    hal->digitalWrite(SOC_GPIO_PIN_RST, hal->GpioLevelLow);
    hal->delay(2000);
    // initialize just like with Arduino
    ESP_LOGI(TAG, "[SX1262] Initializing ... ");
    ConfigLoRa_t config;
    config.frequency = 915;
    int state = radio.begin(config);
    if (state != RADIOLIB_ERR_NONE) {
        ESP_LOGI(TAG, "failed, code %d\n", state);
        while (true) {
            hal->delay(1000);
        }
    }
    ESP_LOGI(TAG, "success!\n");

    radio.setRfSwitchPins(RADIOLIB_NC, SOC_GPIO_PIN_ANT_RXTX);

    node.setActivityLeds(activity_pins);

    // Tracker may not have stable RF conditions
    node.setADR(false);

    // Set a datarate to start off with
    node.setDatarate(2);

    // Manages uplink intervals to the TTN Fair Use Policy
    node.setDutyCycle(true, 1250);

    // Update dwell time limits - 400ms is the limit for the US
    node.setDwellTime(true, 400);

    // Externally powered
    node.setDeviceStatus(0);

    node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);

    hal->delay(5000);

    // loop forever
    for (;;) {
        if (!node.isActivated()) {
            ESP_LOGI(TAG, "Not joined to a network yet...");
            state = node.activateOTAA();
            if (state != RADIOLIB_LORAWAN_NEW_SESSION) {
                ESP_LOGI(TAG, "Join failed - (%d)\n", state);
                hal->delay(uplinkIntervalSeconds * 1000UL);
                continue;
            }

            node.setADR(false);
            // Up the datarate to reduce airtime
            // Should allow us 4 hours before we start hitting the TTN FUP limits
            node.setDatarate(4);
        }

        ESP_LOGI(TAG, "Sending uplink");

        // This is the place to gather the sensor inputs
        // Instead of reading any real sensor, we just generate some random
        // numbers as example
        uint8_t value1 = radio.random(100);
        uint16_t value2 = radio.random(2000);

        // Build payload byte array
        uint8_t uplinkPayload[3];
        uplinkPayload[0] = value1;
        uplinkPayload[1] = value2 >> 8; // See notes for high/lowByte functions
        uplinkPayload[2] = value2 & 0xFF;

        // Perform an uplink
        int16_t state = node.sendReceive(uplinkPayload, sizeof(uplinkPayload));
        if (state != RADIOLIB_ERR_NONE) {
            ESP_LOGI(TAG, "Error in sendReceive - (%d)\n", state);
        }

        // Check if a downlink was received
        // (state 0 = no downlink, state 1/2 = downlink in window Rx1/Rx2)
        if (state > 0) {
            ESP_LOGI(TAG, "Received a downlink");
        } else {
            ESP_LOGI(TAG, "No downlink received");
        }

        ESP_LOGI(TAG, "Next uplink in %d seconds", uplinkIntervalSeconds);

        // Wait until next uplink - observing legal & TTN FUP constraints
        hal->delay(uplinkIntervalSeconds * 1000UL); // delay needs milli-seconds
    }
}
