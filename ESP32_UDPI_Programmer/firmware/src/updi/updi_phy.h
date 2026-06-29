#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>

#include "pins.h"

class UpdiPhy {
public:
    bool begin(uint8_t rxPin, uint8_t txPin, uint32_t baud);
    void end();

    bool sendBytes(const uint8_t *data, size_t length);
    bool receiveBytes(uint8_t *data, size_t length, uint32_t timeoutMs = UPDI_RX_TIMEOUT_MS);

    bool sendSynch();
    bool sendSynchBurst(uint16_t count);
    size_t flushEcho(size_t expectedEchoBytes);

    // Low-baud 0x00 BREAK frame(s), then restore 115200 8E2 UART (open-drain).
    bool sendBreak();
    bool sendDoubleBreak();

private:
    HardwareSerial *_serial = nullptr;
    uint8_t _rxPin = 0;
    uint8_t _txPin = 0;
    uint32_t _baud = 0;
    bool _initialized = false;

    bool sendUartBreakFrame();
    bool restoreOperationalBaud();
    bool openUart(uint32_t baud, uint32_t config);
    void applyUpdiPhyConfig();
    void haltUart();
    void debugPinStates() const;
};
