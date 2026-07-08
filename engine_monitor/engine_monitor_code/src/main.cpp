#include <Arduino.h>

// Physical pin 18 on ATtiny3216-S (20-pin SOIC)
static const uint8_t LED_PIN = PIN_PA2;

void setup() {
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_PIN, HIGH);
  delay(1000);
  digitalWrite(LED_PIN, LOW);
  delay(1000);
}
