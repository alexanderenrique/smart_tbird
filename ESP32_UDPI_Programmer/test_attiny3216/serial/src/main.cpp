#include <Arduino.h>

// USART0 defaults: TX = PA1, RX = PA2 (megaTinyCore).
// For ESP32 UPDI programmer serial bridge: connect PA1 (TX) to ESP32 GPIO25.

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println(F("ATtiny3216 serial test ready"));
}

void loop() {
  static uint32_t count = 0;
  Serial.print(F("tick "));
  Serial.println(count++);
  delay(1000);
}
