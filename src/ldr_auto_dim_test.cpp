#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>

/*
 * LDR Auto-Dimming Display Test
 * 
 * This sketch tests LDR functionality for auto-dimming a TFT display.
 * - LDR analog input on pin 34 (ADC1_CH6)
 * - MOSFET gate control on pin 25 (backlight)
 * - TFT display with SPI interface
 * - Automatically adjusts backlight brightness based on ambient light
 * 
 * TFT_eSPI pins (configure in User_Setup.h):
 * - SCLK: 32
 * - MOSI: 33
 * - CS:   27
 * - DC:   12
 * - RST:  13
 * - BL:   14 (optional backlight control)
 */

// Pin definitions
const int LDR_PIN = 4;        // LDR analog input pin
const int MOSFET_PIN = 25;    // MOSFET gate control pin (backlight)

// Display object
TFT_eSPI tft = TFT_eSPI();

// Configuration
const int MIN_BRIGHTNESS = 20;    // Minimum brightness (0-255) - prevents flickering
const int MAX_BRIGHTNESS = 255;   // Maximum brightness (0-255)
const int LDR_DARK_THRESHOLD = 100;    // LDR value below which is considered "dark"
const int LDR_BRIGHT_THRESHOLD = 800;  // LDR value above which is considered "bright"
const int UPDATE_DELAY = 100;          // Delay between updates in milliseconds
const float GAMMA = 2.2f;              // Gamma correction value for human perception

// Variables
int currentBrightness = 128;  // Current brightness level
int ldrValue = 0;            // Current LDR reading
int mappedBrightness = 0;    // Mapped brightness value
int targetBrightness = 128;  // Target brightness for smoothing
const int SMOOTHING_FACTOR = 8;  // Higher = slower changes (1-16)

// Background color cycling
unsigned long lastColorChange = 0;
const unsigned long COLOR_CHANGE_INTERVAL = 5000; // Change color every 5 seconds
int currentColorIndex = 0;
const uint16_t BACKGROUND_COLORS[] = {TFT_BLACK, TFT_RED, TFT_GREEN, TFT_BLUE, TFT_WHITE};
const char* COLOR_NAMES[] = {"BLACK", "RED", "GREEN", "BLUE", "WHITE"};
const int NUM_COLORS = 5;

// LDR averaging variables
const int AVERAGE_SAMPLES = 100;  // Number of samples to average (10 seconds at 100ms intervals)
int ldrReadings[AVERAGE_SAMPLES]; // Array to store LDR readings
int ldrIndex = 0;                 // Current index in the array
int ldrSum = 0;                   // Sum of all readings
bool ldrArrayFilled = false;      // Whether the array is fully populated
int averagedLdrValue = 0;         // The averaged LDR value

// Function to apply gamma correction for human perception
int applyGammaCorrection(float normalizedBrightness) {
  // Clamp input to 0.0-1.0 range
  normalizedBrightness = constrain(normalizedBrightness, 0.0f, 1.0f);
  
  // Apply gamma correction: duty = maxDuty * pow(brightness, gamma)
  float gammaCorrected = powf(normalizedBrightness, GAMMA);
  
  // Convert back to 0-255 range
  int result = (int)(gammaCorrected * (MAX_BRIGHTNESS - MIN_BRIGHTNESS) + MIN_BRIGHTNESS);
  
  // Ensure we stay within bounds
  return constrain(result, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
}

// Function to update LDR averaging
void updateLdrAverage(int newReading) {
  // Remove the oldest reading from the sum
  if (ldrArrayFilled) {
    ldrSum -= ldrReadings[ldrIndex];
  }
  
  // Add the new reading
  ldrReadings[ldrIndex] = newReading;
  ldrSum += newReading;
  
  // Move to next index
  ldrIndex = (ldrIndex + 1) % AVERAGE_SAMPLES;
  
  // Check if array is filled
  if (ldrIndex == 0) {
    ldrArrayFilled = true;
  }
  
  // Calculate average
  int sampleCount = ldrArrayFilled ? AVERAGE_SAMPLES : ldrIndex;
  averagedLdrValue = ldrSum / sampleCount;
}

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  Serial.println("LDR Auto-Dimming Display Test Started");
  
  // Initialize display
  tft.init();
  tft.setRotation(1); // Landscape orientation
  tft.fillScreen(BACKGROUND_COLORS[currentColorIndex]);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  
  // Display "Hello World"
  tft.setCursor(50, 100);
  tft.println("Hello World!");
  tft.setCursor(30, 130);
  tft.println("LDR Auto-Dim Test");
  
  // Initialize color change timer
  lastColorChange = millis();
  
  // Configure backlight PWM with higher frequency to reduce flicker
  pinMode(MOSFET_PIN, OUTPUT);
  ledcSetup(0, 25000, 8);  // Channel 0, 25kHz, 8-bit resolution (higher freq = less flicker)
  ledcAttachPin(MOSFET_PIN, 0);
  ledcWrite(0, currentBrightness);
  
  Serial.println("Pin configuration:");
  Serial.print("LDR Pin: "); Serial.println(LDR_PIN);
  Serial.print("MOSFET Pin: "); Serial.println(MOSFET_PIN);
  Serial.println("Display initialized");
  Serial.println("-------------------");
}

void loop() {
  // Check if it's time to change background color
  if (millis() - lastColorChange >= COLOR_CHANGE_INTERVAL) {
    currentColorIndex = (currentColorIndex + 1) % NUM_COLORS;
    tft.fillScreen(BACKGROUND_COLORS[currentColorIndex]);
    lastColorChange = millis();
    
    // Redraw the main text
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(50, 100);
    tft.println("Hello World!");
    tft.setCursor(30, 130);
    tft.println("LDR Auto-Dim Test");
    
    Serial.print("Background changed to: ");
    Serial.println(COLOR_NAMES[currentColorIndex]);
  }
  
  // Read LDR value
  ldrValue = analogRead(LDR_PIN);
  
  // Update the 10-second average
  updateLdrAverage(ldrValue);
  
  // Use the averaged LDR value for brightness calculation with gamma correction
  if (averagedLdrValue <= LDR_DARK_THRESHOLD) {
    // Very dark environment - minimum brightness (easier on eyes)
    targetBrightness = MIN_BRIGHTNESS;
  } else if (averagedLdrValue >= LDR_BRIGHT_THRESHOLD) {
    // Very bright environment - maximum brightness (visible in bright light)
    targetBrightness = MAX_BRIGHTNESS;
  } else {
    // Linear mapping between thresholds, then apply gamma correction
    float normalizedBrightness = (float)(averagedLdrValue - LDR_DARK_THRESHOLD) / 
                                (float)(LDR_BRIGHT_THRESHOLD - LDR_DARK_THRESHOLD);
    targetBrightness = applyGammaCorrection(normalizedBrightness);
  }
  
  // Smooth brightness transition to reduce flicker
  if (currentBrightness != targetBrightness) {
    int brightnessDiff = targetBrightness - currentBrightness;
    currentBrightness += brightnessDiff / SMOOTHING_FACTOR;
    
    // Ensure we don't overshoot
    if (abs(brightnessDiff) < SMOOTHING_FACTOR) {
      currentBrightness = targetBrightness;
    }
    
    // Constrain to valid range
    currentBrightness = constrain(currentBrightness, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
    
    // Apply the smoothed brightness
    ledcWrite(0, currentBrightness);
  }
  
  // Update display with current values
  tft.fillRect(0, 160, 320, 80, TFT_BLACK); // Clear info area
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(10, 170);
  tft.print("LDR: "); tft.print(ldrValue); tft.print(" (avg: "); tft.print(averagedLdrValue); tft.println(")");
  tft.setCursor(10, 190);
  tft.print("Brightness: "); tft.print(currentBrightness);
  tft.setCursor(10, 210);
  tft.print("Background: "); tft.println(COLOR_NAMES[currentColorIndex]);
  
  // Draw brightness bar
  int barWidth = map(currentBrightness, 0, 255, 0, 200);
  tft.fillRect(10, 230, 200, 20, TFT_DARKGREY);
  tft.fillRect(10, 230, barWidth, 20, TFT_YELLOW);
  tft.drawRect(10, 230, 200, 20, TFT_WHITE);
  
  // Print debug information
  Serial.print("LDR: "); Serial.print(ldrValue);
  Serial.print(" | Brightness: "); Serial.print(currentBrightness);
  Serial.print(" | Mapped: "); Serial.println(mappedBrightness);
  
  // Wait before next update
  delay(UPDATE_DELAY);
}
