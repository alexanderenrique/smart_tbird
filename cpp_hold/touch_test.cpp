#include <Arduino.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

void setup() {
  Serial.begin(9600);
  tft.init();
  tft.setRotation(1);
  Serial.println("Touch test started. Touch the screen!");
}

void loop() {
  uint16_t x, y;
  if (tft.getTouch(&x, &y, 600)) { // 600 = touch threshold, adjust if needed
    Serial.print("Touch: X=");
    Serial.print(x);
    Serial.print(" Y=");
    Serial.println(y);
  }
  delay(100); // Print at most 10 times per second
} 