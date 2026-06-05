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

#include "EspHal.hpp"
#include "lorawan_config.hpp"
extern "C" {
#include "packet.h"
#include "persistence.h"
}

static const char* TAG = "lorawan";

QueueHandle_t packet_queue;

// the entry point for the program
// it must be declared as "extern C" because the compiler assumes this will be a
// C function
extern "C" void lorawan_task(void) {
    ESP_LOGI(TAG, "Initializing ... ");

    packet_queue = xQueueCreate(64, sizeof(struct packet));
    if (packet_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create packet queue");
        abort();
    }

    persistence_init();

    ConfigLoRa_t config;
    config.frequency = 915;
    int state = radio.begin(config);
    if (state != RADIOLIB_ERR_NONE) {
        ESP_LOGE(TAG, "failed, code %d\n", state);
        abort();
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
    // node.setDwellTime(true, 400); // Not needed for AU

    // Externally powered
    node.setDeviceStatus(0);

    node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);

    persistence_open("radiolib");
    if (persistence_has_key("nonces")) {
        uint8_t buffer[RADIOLIB_LORAWAN_NONCES_BUF_SIZE];
        persistence_get_val("nonces", buffer, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);
        if (node.setBufferNonces(buffer) == RADIOLIB_ERR_NONE) {
            ESP_LOGI(TAG, "Restored nonces from persistent storage");
        } else {
            ESP_LOGW(TAG, "Failed to restore nonces from persistent storage");
        }
    } else {
        ESP_LOGI(TAG, "No nonces found in persistent storage");
    }
    persistence_close();

    // loop forever
    for (;;) {
        if (!node.isActivated()) {
            ESP_LOGI(TAG, "Not joined to a network yet...");
            state = node.activateOTAA();

            // Save the nonces to persistent storage
            persistence_open("radiolib");
            uint8_t* nonces_buffer = node.getBufferNonces();
            persistence_set_val("nonces", nonces_buffer,
                                RADIOLIB_LORAWAN_NONCES_BUF_SIZE);
            persistence_close();

            if (state != RADIOLIB_LORAWAN_NEW_SESSION) {
                ESP_LOGI(TAG, "Join failed - (%d)\n", state);
                vTaskDelay(pdMS_TO_TICKS(uplinkIntervalMs));
                continue;
            } else {
                ESP_LOGI(TAG, "Join successful!");
            }

            node.setADR(false);
            // Up the datarate to reduce airtime
            // Should give us 4 hours of runtime before we start hitting the
            // TTN FUP limits
            node.setDatarate(4);

            // Doing this manually, as the count is per 24 hours
            node.setDutyCycle(false, 0);
        }

        TickType_t currentTicks = xTaskGetTickCount();
        // Add an extra minute every hour
        TickType_t additionalTicks =
            (currentTicks / pdMS_TO_TICKS(3600'000UL)) *
            pdMS_TO_TICKS(60'000UL);
        // Cap at 8 minutes
        if (additionalTicks > pdMS_TO_TICKS(8 * 60'000UL)) {
            additionalTicks = pdMS_TO_TICKS(8 * 60'000UL);
        }
        TickType_t ticksToWait =
            pdMS_TO_TICKS(uplinkIntervalMs) + additionalTicks;

        struct packet packet;
        if (xQueueReceive(packet_queue, &packet, ticksToWait) == pdTRUE) {
            ESP_LOGI(TAG, "Sending uplink");

            // Perform an uplink
            int16_t state =
                node.sendReceive(packet.payload_bytes,
                                 sizeof(packet.payload_bytes), packet.port);
            if (state < RADIOLIB_ERR_NONE) {
                ESP_LOGI(TAG, "Error in sendReceive - (%d)\n", state);
            }

            /*
            // Check if a downlink was received
            // (state 0 = no downlink, state 1/2 = downlink in window Rx1/Rx2)
            if (state > 0) {
                ESP_LOGI(TAG, "Received a downlink");
            } else {
                ESP_LOGI(TAG, "No downlink received");
            }
            */

            ESP_LOGI(TAG, "Next uplink in %d seconds", uplinkIntervalSeconds);
        } else {
            ESP_LOGI(TAG, "No data to send");
        }

        // Wait until next uplink
        xTaskDelayUntil(&currentTicks, ticksToWait);
    }
}
