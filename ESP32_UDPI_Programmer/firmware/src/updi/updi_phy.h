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
    size_t flushEcho(size_t expectedEchoBytes);

    // Send a BREAK (12+ consecutive low bits) to reset UPDI state.
    bool sendBreakCharacter();
    // Two consecutive BREAKs per Microchip UPDI spec (error recovery / init).
    bool sendDoubleBreak();

private:
    HardwareSerial *_serial = nullptr;
    uint8_t _rxPin = 0;
    uint8_t _txPin = 0;
    uint32_t _baud = 0;
    bool _initialized = false;

    bool restoreOperationalBaud();
    void configureUartMode();
    void configureOpenDrainTx();
    void assignUartPins();
    void debugPinStates() const;
};
