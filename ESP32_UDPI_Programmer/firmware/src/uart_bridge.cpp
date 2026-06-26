#include "uart_bridge.h"

void UartBridge::begin() {
    // Phase 1: GPIO 16/17 are used for UPDI (Serial2). Bridge pins assigned in Phase 2.
    _targetSerial = nullptr;
}

void UartBridge::enable() {
    _enabled = true;
}

void UartBridge::disable() {
    _enabled = false;
}

void UartBridge::poll() {
    if (!_enabled || !_targetSerial) {
        return;
    }

    while (_targetSerial->available()) {
        Serial.write(static_cast<char>(_targetSerial->read()));
    }

    while (Serial.available()) {
        _targetSerial->write(static_cast<char>(Serial.read()));
    }
}
