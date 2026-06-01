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

// include the hardware abstraction layer
#include "EspHal.h"
#include "board_config.h"

// create a new instance of the HAL class
EspHal* hal = new EspHal(SOC_GPIO_PIN_SCK, SOC_GPIO_PIN_MISO, SOC_GPIO_PIN_MOSI);

// now we can create the radio module
SX1262 radio = new Module(hal, SOC_GPIO_PIN_SS, SOC_GPIO_PIN_DIO1, SOC_GPIO_PIN_RST, SOC_GPIO_PIN_BUSY);

static const char *TAG = "main";

// the entry point for the program
// it must be declared as "extern C" because the compiler assumes this will be a C function
extern "C" void app_main(void) {
  hal->delay(2000);
  // initialize just like with Arduino
  ESP_LOGI(TAG, "[SX1262] Initializing ... ");
  int state = radio.begin({});
  if (state != RADIOLIB_ERR_NONE) {
    ESP_LOGI(TAG, "failed, code %d\n", state);
    while(true) {
      hal->delay(1000);
    }
  }
  ESP_LOGI(TAG, "success!\n");

  radio.setRfSwitchPins(RADIOLIB_NC, SOC_GPIO_PIN_ANT_RXTX);

  // loop forever
  for(;;) {
    // send a packet
    ESP_LOGI(TAG, "[SX1262] Transmitting packet ... ");
    state = radio.transmit("Hello World!");
    if(state == RADIOLIB_ERR_NONE) {
      // the packet was successfully transmitted
      ESP_LOGI(TAG, "success!");

    } else {
      ESP_LOGI(TAG, "failed, code %d\n", state);

    }

    // wait for a second before transmitting again
    hal->delay(1000);

  }

}