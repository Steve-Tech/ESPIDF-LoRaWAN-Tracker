#ifndef ESP_HAL_H
#define ESP_HAL_H

// include RadioLib
#include <RadioLib.h>

// include all the dependencies
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/gpio_hal.h"
#include "soc/dport_reg.h"
#include "soc/rtc.h"
#include "soc/spi_reg.h"
#include "soc/spi_struct.h"

// define Arduino-style macros
#define LOW (0x0)
#define HIGH (0x1)
#define INPUT (0x01)
#define OUTPUT (0x03)
#define RISING (0x01)
#define FALLING (0x02)
#define NOP() asm volatile("nop")

// create a new ESP-IDF hardware abstraction layer
// the HAL must inherit from the base RadioLibHal class
// and implement all of its virtual methods
// this is pretty much just copied from Arduino ESP32 core
class EspHal : public RadioLibHal {
  public:
    // default constructor - initializes the base HAL and any needed private
    // members
    EspHal(int8_t sck, int8_t miso, int8_t mosi)
        : RadioLibHal(INPUT, OUTPUT, LOW, HIGH, RISING, FALLING), spiSCK(sck),
          spiMISO(miso), spiMOSI(mosi) {}

    void init() override {
        // we only need to init the SPI here
        spiBegin();
    }

    void term() override {
        // we only need to stop the SPI here
        spiEnd();
    }

    // GPIO-related methods (pinMode, digitalWrite etc.) should check
    // RADIOLIB_NC as an alias for non-connected pins
    void pinMode(uint32_t pin, uint32_t mode) override {
        if (pin == RADIOLIB_NC) {
            return;
        }

        gpio_hal_context_t gpiohal;
        gpiohal.dev = GPIO_LL_GET_HW(GPIO_PORT_0);

        gpio_config_t conf = {
            .pin_bit_mask = (1ULL << pin),
            .mode = (gpio_mode_t)mode,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = (gpio_int_type_t)gpiohal.dev->pin[pin].int_type,
        };
        gpio_config(&conf);
    }

    void digitalWrite(uint32_t pin, uint32_t value) override {
        if (pin == RADIOLIB_NC) {
            return;
        }

        gpio_set_level((gpio_num_t)pin, value);
    }

    uint32_t digitalRead(uint32_t pin) override {
        if (pin == RADIOLIB_NC) {
            return (0);
        }

        return (gpio_get_level((gpio_num_t)pin));
    }

    void attachInterrupt(uint32_t interruptNum, void (*interruptCb)(void),
                         uint32_t mode) override {
        if (interruptNum == RADIOLIB_NC) {
            return;
        }

        gpio_install_isr_service((int)ESP_INTR_FLAG_IRAM);
        gpio_set_intr_type((gpio_num_t)interruptNum,
                           (gpio_int_type_t)(mode & 0x7));

        // this uses function typecasting, which is not defined when the
        // functions have different signatures untested and might not work
        gpio_isr_handler_add((gpio_num_t)interruptNum,
                             (void (*)(void*))interruptCb, NULL);
    }

    void detachInterrupt(uint32_t interruptNum) override {
        if (interruptNum == RADIOLIB_NC) {
            return;
        }

        gpio_isr_handler_remove((gpio_num_t)interruptNum);
        gpio_wakeup_disable((gpio_num_t)interruptNum);
        gpio_set_intr_type((gpio_num_t)interruptNum, GPIO_INTR_DISABLE);
    }

    void delay(unsigned long ms) override { vTaskDelay(pdMS_TO_TICKS(ms)); }

    void delayMicroseconds(unsigned long us) override {
        uint64_t m = (uint64_t)esp_timer_get_time();
        if (us) {
            uint64_t e = (m + us);
            if (m > e) { // overflow
                while ((uint64_t)esp_timer_get_time() > e) {
                    NOP();
                }
            }
            while ((uint64_t)esp_timer_get_time() < e) {
                NOP();
            }
        }
    }

    unsigned long millis() override {
        return ((unsigned long)(esp_timer_get_time() / 1000ULL));
    }

    unsigned long micros() override {
        return ((unsigned long)(esp_timer_get_time()));
    }

    long pulseIn(uint32_t pin, uint32_t state, unsigned long timeout) override {
        if (pin == RADIOLIB_NC) {
            return (0);
        }

        this->pinMode(pin, INPUT);
        uint32_t start = this->micros();
        uint32_t curtick = this->micros();

        while (this->digitalRead(pin) == state) {
            if ((this->micros() - curtick) > timeout) {
                return (0);
            }
        }

        return (this->micros() - start);
    }

    void spiBegin() {
        spi_bus_config_t buscfg = {.mosi_io_num = this->spiMOSI,
                                   .miso_io_num = this->spiMISO,
                                   .sclk_io_num = this->spiSCK,
                                   .quadwp_io_num = -1,
                                   .quadhd_io_num = -1,
                                   .data4_io_num = -1,
                                   .data5_io_num = -1,
                                   .data6_io_num = -1,
                                   .data7_io_num = -1,
                                   .data_io_default_level = 0,
                                   .max_transfer_sz = SOC_SPI_MAXIMUM_BUFFER_SIZE,
                                   .flags = 0,
                                   .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
                                   .intr_flags = 0};
        spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_DISABLED);

        spi_device_interface_config_t devcfg = {
            .command_bits = 0,
            .address_bits = 0,
            .dummy_bits = 0,
            .mode = 0,
            .clock_source = SPI_CLK_SRC_DEFAULT,
            .duty_cycle_pos = 128,
            .cs_ena_pretrans = 0,
            .cs_ena_posttrans = 0,
            .clock_speed_hz = 2000000,
            .input_delay_ns = 0,
            .sample_point = SPI_SAMPLING_POINT_PHASE_0,
            .spics_io_num = -1,
            .flags = 0,
            .queue_size = 1,
            .pre_cb = NULL,
            .post_cb = NULL};
        spi_bus_add_device(SPI2_HOST, &devcfg, &this->spi_device_handle);
    }

    void spiBeginTransaction() {
        // not needed - in ESP32 Arduino core, this function
        // repeats clock div, mode and bit order configuration
    }

    void spiTransfer(uint8_t* out, size_t len, uint8_t* in) {
        spi_transaction_t spi_transaction = {
            .flags = 0,
            .cmd = 0,
            .addr = 0,
            .length = len * 8,
            .rxlength = 0,
            .override_freq_hz = 0,
            .user = NULL,
            .tx_buffer = out,
            .rx_buffer = in,
        };
        spi_device_transmit(this->spi_device_handle, &spi_transaction);
    }

    void spiEndTransaction() {
        // nothing needs to be done here
    }

    void spiEnd() {
        // detach pins
        spi_bus_remove_device(this->spi_device_handle);
        spi_bus_free(SPI2_HOST);
    }

  private:
    // the HAL can contain any additional private members
    int8_t spiSCK;
    int8_t spiMISO;
    int8_t spiMOSI;
    spi_device_handle_t spi_device_handle;
};

#endif
