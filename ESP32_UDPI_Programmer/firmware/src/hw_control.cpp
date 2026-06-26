#include "hw_control.h"

#include "pins.h"

void HwControl::begin() {
    pinMode(PIN_TARGET_RESET, OUTPUT);
    digitalWrite(PIN_TARGET_RESET, HIGH);
}

void HwControl::resetTarget() {
    digitalWrite(PIN_TARGET_RESET, LOW);
    delay(20);
    digitalWrite(PIN_TARGET_RESET, HIGH);
    delay(20);
}

void HwControl::powerOn() {
    // Target is powered directly from ESP32 3.3V; no GPIO control.
}

void HwControl::powerOff() {
    // Target is powered directly from ESP32 3.3V; no GPIO control.
}
