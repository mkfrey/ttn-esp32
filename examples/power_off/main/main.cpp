/*******************************************************************************
 *
 * ttn-esp32 - The Things Network device library for ESP-IDF / SX127x
 *
 * Copyright (c) 2021 Manuel Bleichenbacher
 *
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 *
 * Sample program sending messages and going to deep sleep in-between.
 *******************************************************************************/

#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"

#include "TheThingsNetwork.h"

// NOTE:
// The LoRaWAN frequency and the radio chip must be configured by running 'idf.py menuconfig'.
// Go to Components / The Things Network, select the appropriate values and save.

// Copy the below hex strings from the TTN console (Applications > Your application > End devices
// > Your device > Activation information)

// AppEUI (sometimes called JoinEUI)
const char *appEui = "????????????????";
// DevEUI
const char *devEui = "????????????????";
// AppKey
const char *appKey = "????????????????????????????????";

// Pins and other resources
#define TTN_SPI_HOST HSPI_HOST
#define TTN_SPI_DMA_CHAN 1
#define TTN_PIN_SPI_SCLK 5
#define TTN_PIN_SPI_MOSI 27
#define TTN_PIN_SPI_MISO 19
#define TTN_PIN_NSS 18
#define TTN_PIN_RXTX TTN_NOT_CONNECTED
#define TTN_PIN_RST 14
#define TTN_PIN_DIO0 26
#define TTN_PIN_DIO1 35

static TheThingsNetwork ttn;

const unsigned TX_INTERVAL = 60;
static uint8_t msgData[] = "Hello, world";

void messageReceived(const uint8_t *message, size_t length, ttn_port_t port)
{
    printf("Message of %d bytes received on port %d:", length, port);
    for (int i = 0; i < length; i++)
        printf(" %02x", message[i]);
    printf("\n");
}

extern "C" void app_main(void)
{
    esp_err_t err;
    // Initialize the GPIO ISR handler service
    err = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    ESP_ERROR_CHECK(err);

    // Initialize the NVS (non-volatile storage) for saving and restoring the keys
    err = nvs_flash_init();
    ESP_ERROR_CHECK(err);

    // Initialize SPI bus
    spi_bus_config_t spi_bus_config;
    spi_bus_config.miso_io_num = TTN_PIN_SPI_MISO;
    spi_bus_config.mosi_io_num = TTN_PIN_SPI_MOSI;
    spi_bus_config.sclk_io_num = TTN_PIN_SPI_SCLK;
    spi_bus_config.quadwp_io_num = -1;
    spi_bus_config.quadhd_io_num = -1;
    spi_bus_config.max_transfer_sz = 0;
    err = spi_bus_initialize(TTN_SPI_HOST, &spi_bus_config, TTN_SPI_DMA_CHAN);
    ESP_ERROR_CHECK(err);

    // Configure the SX127x pins
    ttn.configurePins(TTN_SPI_HOST, TTN_PIN_NSS, TTN_PIN_RXTX, TTN_PIN_RST, TTN_PIN_DIO0, TTN_PIN_DIO1);

    // The below line can be commented after the first run as the data is saved in NVS
    ttn.provision(devEui, appEui, appKey);

    // Register callback for received messages
    ttn.onMessage(messageReceived);

    //    ttn.setAdrEnabled(false);
    //    ttn.setDataRate(kTTNDataRate_US915_SF7);
    //    ttn.setMaxTxPower(14);

    if (ttn.resumeAfterPowerOff(60))
    {
        printf("Resumed from power off.\n");
    }
    else
    {
        printf("Joining...\n");
        if (ttn.join())
        {
            printf("Joined.\n");
        }
        else
        {
            printf("Join failed. Goodbye\n");
            return;
        }
    }

    printf("Sending message...\n");
    TTNResponseCode res = ttn.transmitMessage(msgData, sizeof(msgData) - 1);
    printf(res == kTTNSuccessfulTransmission ? "Message sent.\n" : "Transmission failed.\n");

    // Wait until TTN communication is idle and save state
    ttn.waitForIdle();
    ttn.prepareForPowerOff();

    printf("Power off...\n");
    // Do whatever is needed to power off the device.
    // For testing, press reset button to simulate power cycle.
    while (true)
        vTaskDelay(pdMS_TO_TICKS(1000));
}
