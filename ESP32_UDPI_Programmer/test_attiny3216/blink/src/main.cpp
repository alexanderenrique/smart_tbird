#include <Arduino.h>

// Status LED on PB1 (temp_fan_node board). Change if your LED is elsewhere.
static const uint8_t LED_PIN = PIN_PB1;

void setup() {
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
  delay(500);
}
