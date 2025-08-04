/*
 * TFT Touch Screen Test
 * =====================
 * 
 * PURPOSE:
 * Simple test application for TFT touchscreen to verify touch input functionality.
 * Reads and displays touch coordinates via serial output without LVGL.
 * 
 * ENVIRONMENT:
 * - Hardware: ESP32 with TFT touchscreen display
 * - Platform: PlatformIO with Arduino framework
 * - Libraries: TFT_eSPI (display only, no LVGL)
 * 
 * FUNCTIONALITY:
 * 1. Initializes TFT display without LVGL
 * 2. Reads touch input directly from display
 * 3. Displays touch coordinates (X, Y) via serial monitor
 * 4. Configurable touch threshold (default: 600)
 * 5. Simple touch detection without GUI framework
 * 
 * CONNECTIONS:
 * - TFT Display: Uses TFT_eSPI library with custom pin configuration
 * - Touch Input: Integrated capacitive touch on display
 * 
 * USAGE:
 * Use this to test basic touchscreen functionality without LVGL.
 * Touch the screen and monitor serial output to see coordinate data.
 */

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