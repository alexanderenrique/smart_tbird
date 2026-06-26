#pragma once

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

#include "pins.h"

inline void updiDebugMsg(const char *message) {
    if (!UPDI_DEBUG) {
        return;
    }
    Serial.print("DEBUG UPDI ");
    Serial.println(message);
}

inline void updiDebugFmt(const char *label, uint32_t value) {
    if (!UPDI_DEBUG) {
        return;
    }
    Serial.print("DEBUG UPDI ");
    Serial.print(label);
    Serial.print("=0x");
    Serial.println(value, HEX);
}

inline void updiDebugHex(const char *label, const uint8_t *data, size_t length) {
    if (!UPDI_DEBUG || data == nullptr) {
        return;
    }
    Serial.print("DEBUG UPDI ");
    Serial.print(label);
    Serial.print(" [");
    for (size_t index = 0; index < length; ++index) {
        if (index > 0) {
            Serial.print(' ');
        }
        if (data[index] < 0x10) {
            Serial.print('0');
        }
        Serial.print(data[index], HEX);
    }
    Serial.println("]");
}

inline void updiDebugPinState(uint8_t pin) {
    if (!UPDI_DEBUG) {
        return;
    }
    Serial.print("DEBUG UPDI GPIO");
    Serial.print(pin);
    Serial.print(" level=");
    Serial.println(digitalRead(pin) ? "HIGH" : "LOW");
}

inline void updiDebugFlushRx(HardwareSerial *serial, const char *label) {
    if (!UPDI_DEBUG || serial == nullptr) {
        return;
    }
    uint8_t pending[32];
    size_t count = 0;
    while (serial->available() && count < sizeof(pending)) {
        pending[count++] = static_cast<uint8_t>(serial->read());
    }
    if (count > 0) {
        updiDebugHex(label, pending, count);
    } else {
        Serial.print("DEBUG UPDI ");
        Serial.print(label);
        Serial.println(" (empty)");
    }
}
