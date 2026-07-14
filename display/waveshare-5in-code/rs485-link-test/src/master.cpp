/*
 * Waveshare RS-485 master (raw, no Modbus).
 *
 * Mirrors the Waveshare 02_RS485_Test UART setup:
 *   Serial1 / UART1 @ 115200 8N1, RX=GPIO43, TX=GPIO44
 * (onboard SP3485 handles DE/RE automatically)
 *
 * Sends a short ASCII line every second so a listen-only slave can prove
 * the bus is working. Also prints any bytes that come back.
 *
 * Ref: https://docs.waveshare.com/ESP32-S3-Touch-LCD-5/Development-Environment-Setup-Arduino
 */

#include <Arduino.h>

static constexpr int RS485_RX = 43;
static constexpr int RS485_TX = 44;
static constexpr unsigned long BAUD = 115200;
static constexpr unsigned long SEND_INTERVAL_MS = 1000;

static HardwareSerial& RS485 = Serial1;
static uint32_t seq = 0;

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println();
    Serial.println("RS-485 master (Waveshare 02_RS485_Test style)");
    Serial.printf("TX path: UART1 @ %lu 8N1  RX=%d TX=%d (auto DE/RE)\n",
                  BAUD, RS485_RX, RS485_TX);

    RS485.begin(BAUD, SERIAL_8N1, RS485_RX, RS485_TX);
    Serial.println("Sending one line per second. Watch the slave monitor.");
}

void loop() {
    static unsigned long last_send = 0;
    const unsigned long now = millis();

    if (now - last_send >= SEND_INTERVAL_MS) {
        last_send = now;
        seq++;

        char msg[64];
        const int n = snprintf(msg, sizeof(msg), "RS485 TEST %lu\n",
                               static_cast<unsigned long>(seq));
        RS485.write(reinterpret_cast<const uint8_t*>(msg), n);
        RS485.flush();

        Serial.printf("TX [%lu]: %s", static_cast<unsigned long>(seq), msg);
    }

    while (RS485.available() > 0) {
        const int b = RS485.read();
        if (b < 0) {
            break;
        }
        if (b >= 32 && b < 127) {
            Serial.printf("RX ascii: '%c' (0x%02X)\n", b, b);
        } else {
            Serial.printf("RX byte: 0x%02X\n", b);
        }
    }
}
