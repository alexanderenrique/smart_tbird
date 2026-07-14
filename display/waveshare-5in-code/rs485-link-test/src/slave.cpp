/*
 * Dummy ESP32 RS-485 slave — receive-only listen test.
 *
 * Holds DE+RE low so the MAX485 stays in receive. Prints every byte that
 * arrives on the bus (ASCII if printable). Pair with waveshare-master.
 *
 * Baud matches Waveshare 02_RS485_Test: 115200 8N1.
 *
 * Heartbeat LED (GPIO2):
 *   1 Hz blink          = sketch running
 *   double-blink burst  = received an RS-485 line
 */

#include <Arduino.h>

static constexpr int RS485_RX = 16;    // RO → ESP32 RX2
static constexpr int RS485_TX = 17;    // DI → ESP32 TX2 (unused while listening)
static constexpr int RS485_DE_RE = 4;  // DE+RE tied; LOW = receive
static constexpr int HEARTBEAT_LED = 2;  // most ESP32 DevKit blue LED
static constexpr unsigned long BAUD = 115200;
static constexpr unsigned long HEARTBEAT_MS = 500;

static HardwareSerial RS485(2);

static uint32_t lines_rx = 0;
static uint32_t bytes_rx = 0;
static bool line_pending_blink = false;

static void blinkReceived() {
    // Short double-blink so it's distinct from the 1 Hz heartbeat.
    digitalWrite(HEARTBEAT_LED, HIGH);
    delay(40);
    digitalWrite(HEARTBEAT_LED, LOW);
    delay(40);
    digitalWrite(HEARTBEAT_LED, HIGH);
    delay(40);
    digitalWrite(HEARTBEAT_LED, LOW);
}

void setup() {
    pinMode(HEARTBEAT_LED, OUTPUT);
    digitalWrite(HEARTBEAT_LED, LOW);

    pinMode(RS485_DE_RE, OUTPUT);
    digitalWrite(RS485_DE_RE, LOW);

    Serial.begin(115200);
    delay(300);
    Serial.println();
    Serial.println("RS-485 slave listen-only");
    Serial.printf("UART2 @ %lu 8N1  RX=%d TX=%d DE=%d (forced RX)\n",
                  BAUD, RS485_RX, RS485_TX, RS485_DE_RE);
    Serial.printf("Heartbeat LED GPIO%d: 1Hz=alive, double-blink=RX line\n",
                  HEARTBEAT_LED);
    Serial.flush();

    RS485.begin(BAUD, SERIAL_8N1, RS485_RX, RS485_TX);
    Serial.println("Listening for master 'RS485 TEST n' lines...");
    Serial.flush();

    // Three quick blinks = setup finished.
    for (int i = 0; i < 3; i++) {
        digitalWrite(HEARTBEAT_LED, HIGH);
        delay(80);
        digitalWrite(HEARTBEAT_LED, LOW);
        delay(80);
    }
}

void loop() {
    static unsigned long last_beat = 0;
    static bool led_on = false;
    static unsigned long last_status = 0;

    while (RS485.available() > 0) {
        const int b = RS485.read();
        if (b < 0) {
            break;
        }

        bytes_rx++;

        if (b == '\n') {
            lines_rx++;
            line_pending_blink = true;
            Serial.println();
        } else if (b == '\r') {
            // ignore
        } else if (b >= 32 && b < 127) {
            Serial.write(static_cast<char>(b));
        } else {
            Serial.printf("[%02X]", b);
        }
    }

    if (line_pending_blink) {
        line_pending_blink = false;
        blinkReceived();
        last_beat = millis();  // keep heartbeat phase clean
        led_on = false;
    }

    const unsigned long now = millis();
    if (now - last_beat >= HEARTBEAT_MS) {
        last_beat = now;
        led_on = !led_on;
        digitalWrite(HEARTBEAT_LED, led_on ? HIGH : LOW);
    }

    // Serial status every 5s.
    if (now - last_status >= 5000) {
        last_status = now;
        Serial.printf("hb alive  lines=%lu bytes=%lu\n",
                      static_cast<unsigned long>(lines_rx),
                      static_cast<unsigned long>(bytes_rx));
        Serial.flush();
    }
}
