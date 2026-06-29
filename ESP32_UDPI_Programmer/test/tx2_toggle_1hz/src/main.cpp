// Scope test: square wave on GPIO 17 (TX2) at 1 Hz.
// Probe GPIO 17 — see ../README.md
//
// GPIO 17 is UART2 TX. Arduino pinMode/digitalWrite alone cannot drive it
// reliably; release UART2 and reset the pad mux first (see espressif/arduino-esp32#3032).

#include <Arduino.h>
#include "driver/gpio.h"

#ifndef TX_PIN
#define TX_PIN 17
#endif
#ifndef RX_PIN
#define RX_PIN 16
#endif
#ifndef HALF_PERIOD_MS
#define HALF_PERIOD_MS 10
#endif
#ifndef USB_SERIAL_BAUD
#define USB_SERIAL_BAUD 115200
#endif

static gpio_num_t txGpio;
static gpio_num_t rxGpio;

static void releaseUart2Pins() {
    Serial2.end();

    txGpio = static_cast<gpio_num_t>(TX_PIN);
    rxGpio = static_cast<gpio_num_t>(RX_PIN);

    gpio_reset_pin(txGpio);
    gpio_reset_pin(rxGpio);

    // If RX2/TX2 are hardware-tied, leave RX high-Z so it does not fight TX.
    gpio_set_direction(rxGpio, GPIO_MODE_INPUT);
    gpio_set_pull_mode(rxGpio, GPIO_FLOATING);

    gpio_set_direction(txGpio, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(txGpio, GPIO_FLOATING);
}

void setup() {
    Serial.begin(USB_SERIAL_BAUD);
    delay(300);

    releaseUart2Pins();
    gpio_set_level(txGpio, 0);

    Serial.printf("tx2_toggle_1hz: GPIO%d toggling every %lu ms (%.1f Hz)\n",
                  TX_PIN, static_cast<unsigned long>(HALF_PERIOD_MS),
                  1000.0f / (2.0f * static_cast<float>(HALF_PERIOD_MS)));
}

void loop() {
    gpio_set_level(txGpio, 1);
    delay(HALF_PERIOD_MS);
    gpio_set_level(txGpio, 0);
    delay(HALF_PERIOD_MS);
}
