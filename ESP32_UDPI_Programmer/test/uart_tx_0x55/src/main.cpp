// Scope test: stream 0x55 on Serial2 TX continuously at 115200 baud.
// Probe GPIO 17 (TX2) — see ../README.md

#include <Arduino.h>

#ifndef TX_PIN
#define TX_PIN 17
#endif
#ifndef RX_PIN
#define RX_PIN 16
#endif
#ifndef BAUD
#define BAUD 115200
#endif
#ifndef USB_SERIAL_BAUD
#define USB_SERIAL_BAUD 115200
#endif
#ifndef STATUS_INTERVAL_MS
#define STATUS_INTERVAL_MS 1000
#endif

#if defined(MATCH_UPDI_PHY)
#include "driver/gpio.h"
#include "driver/uart.h"

static constexpr uart_port_t UPDI_UART_NUM = UART_NUM_2;

static void configureUpdiPhy() {
    uart_set_mode(UPDI_UART_NUM, UART_MODE_UART);
    const gpio_num_t txGpio = static_cast<gpio_num_t>(TX_PIN);
    gpio_set_pull_mode(txGpio, GPIO_PULLUP_ONLY);
    gpio_set_direction(txGpio, GPIO_MODE_INPUT_OUTPUT_OD);
}
#endif

static uint32_t gTxBytes = 0;
static uint32_t gRxEchoBytes = 0;
static uint32_t gRxMismatch = 0;
static uint32_t gLastStatusMs = 0;

void setup() {
    Serial.begin(USB_SERIAL_BAUD);
    delay(300);

#if defined(MATCH_UPDI_PHY)
    Serial2.begin(BAUD, SERIAL_8E2, RX_PIN, TX_PIN, false);
    configureUpdiPhy();
    Serial.printf("uart_tx_0x55: GPIO%d @ %lu 8E2 open-drain, streaming 0x55\n",
                  TX_PIN, static_cast<unsigned long>(BAUD));
#else
    Serial2.begin(BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
    Serial.printf("uart_tx_0x55: GPIO%d @ %lu 8N1, streaming 0x55\n",
                  TX_PIN, static_cast<unsigned long>(BAUD));
#endif

    while (Serial2.available()) {
        Serial2.read();
    }

    gLastStatusMs = millis();
}

void loop() {
    Serial2.write(0x55);
    gTxBytes++;

    while (Serial2.available()) {
        const uint8_t byte = static_cast<uint8_t>(Serial2.read());
        gRxEchoBytes++;
        if (byte != 0x55) {
            gRxMismatch++;
        }
    }

    const uint32_t now = millis();
    if ((now - gLastStatusMs) >= STATUS_INTERVAL_MS) {
        Serial.printf("loopback TX=%lu RX_echo=%lu mismatch=%lu\n",
                      static_cast<unsigned long>(gTxBytes),
                      static_cast<unsigned long>(gRxEchoBytes),
                      static_cast<unsigned long>(gRxMismatch));
        gLastStatusMs = now;
    }
}
