// Scope test: square wave on a plain GPIO (default GPIO 5).
// Timing comes from HALF_PERIOD_MS in platformio.ini (not the fallback below).

#include <Arduino.h>

#ifndef GPIO_PIN
#define GPIO_PIN 5
#endif
#ifndef HALF_PERIOD_MS
#define HALF_PERIOD_MS 500
#endif
#ifndef USB_SERIAL_BAUD
#define USB_SERIAL_BAUD 115200
#endif

void setup() {
    Serial.begin(USB_SERIAL_BAUD);
    delay(300);

    pinMode(GPIO_PIN, OUTPUT);
    digitalWrite(GPIO_PIN, LOW);

    const float hz = 1000.0f / (2.0f * static_cast<float>(HALF_PERIOD_MS));
    Serial.printf("gpio5_toggle: GPIO%d, %.3f Hz (%lu ms per half-cycle)\n",
                  GPIO_PIN, hz, static_cast<unsigned long>(HALF_PERIOD_MS));
}

void loop() {
    digitalWrite(GPIO_PIN, HIGH);
    delay(HALF_PERIOD_MS);
    Serial.println("HIGH");
    digitalWrite(GPIO_PIN, LOW);
    delay(HALF_PERIOD_MS);
    Serial.println("LOW");
    
}
