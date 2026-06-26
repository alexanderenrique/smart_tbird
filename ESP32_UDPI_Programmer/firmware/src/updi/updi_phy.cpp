#include "updi_phy.h"

#include "driver/gpio.h"
#include "driver/uart.h"
#include "pins.h"
#include "updi_debug.h"

static constexpr uart_port_t UPDI_UART_NUM = UART_NUM_2;

void UpdiPhy::assignUartPins() {
    esp_err_t err = uart_set_pin(UPDI_UART_NUM, _txPin, _rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        updiDebugFmt("uart_set_pin err", static_cast<uint32_t>(err));
        return;
    }
    updiDebugFmt("UART2 TX GPIO", _txPin);
    updiDebugFmt("UART2 RX GPIO", _rxPin);
}

void UpdiPhy::configureUartMode() {
    // Hardware-tied RX2/TX2 on GPIO 16/17. Do not use RS485 half-duplex here —
    // that mode gates RX via RTS for an external transceiver we do not have.
    uart_set_mode(UPDI_UART_NUM, UART_MODE_UART);
    updiDebugMsg("UART mode standard (hardware-tied RX2/TX2)");
}

void UpdiPhy::configureOpenDrainTx() {
    // Open-drain TX only. Never call pinMode/gpio_set_direction on the RX pin —
    // that disconnects UART2 RX from GPIO 16 and kills loopback on the tied bus.
    const gpio_num_t txGpio = static_cast<gpio_num_t>(_txPin);
    gpio_set_pull_mode(txGpio, GPIO_PULLUP_ONLY);
    gpio_set_direction(txGpio, GPIO_MODE_INPUT_OUTPUT_OD);
}

void UpdiPhy::debugPinStates() const {
    updiDebugPinState(_rxPin);
    updiDebugPinState(_txPin);
}

bool UpdiPhy::begin(uint8_t rxPin, uint8_t txPin, uint32_t baud) {
    _rxPin = rxPin;
    _txPin = txPin;
    _baud = baud;
    _serial = &Serial2;

    _serial->end();
    _serial->begin(baud, SERIAL_8E2, _rxPin, _txPin, false);
    assignUartPins();
    configureUartMode();
    configureOpenDrainTx();

    delay(10);
    updiDebugFmt("UART open RX GPIO", rxPin);
    updiDebugFmt("UART open TX GPIO", txPin);
    updiDebugFmt("UART baud", baud);
    debugPinStates();
    updiDebugFlushRx(_serial, "RX flush after open");

    _initialized = true;
    return true;
}

void UpdiPhy::end() {
    if (_serial) {
        _serial->end();
    }
    _initialized = false;
}

bool UpdiPhy::sendBytes(const uint8_t *data, size_t length) {
    if (!_initialized || !_serial) {
        updiDebugMsg("TX fail: UART not initialized");
        return false;
    }
    updiDebugHex("TX", data, length);
    if (_serial->write(data, length) != length) {
        updiDebugMsg("TX fail: short write");
        return false;
    }
    uart_wait_tx_done(UPDI_UART_NUM, pdMS_TO_TICKS(100));
    return true;
}

bool UpdiPhy::receiveBytes(uint8_t *data, size_t length, uint32_t timeoutMs) {
    if (!_initialized || !_serial) {
        updiDebugMsg("RX fail: UART not initialized");
        return false;
    }

    uart_wait_tx_done(UPDI_UART_NUM, pdMS_TO_TICKS(100));

    size_t received = 0;
    uint32_t start = millis();
    while (received < length) {
        if (_serial->available()) {
            data[received++] = static_cast<uint8_t>(_serial->read());
            continue;
        }
        if ((millis() - start) > timeoutMs) {
            updiDebugFmt("RX timeout ms", timeoutMs);
            updiDebugFmt("RX got bytes", received);
            if (received > 0) {
                updiDebugHex("RX partial", data, received);
            }
            updiDebugFlushRx(_serial, "RX stray after timeout");
            return false;
        }
        delayMicroseconds(100);
    }

    updiDebugHex("RX", data, length);
    return true;
}

bool UpdiPhy::sendSynch() {
    const uint8_t synch = 0x55;
    updiDebugMsg("SYNCH");
    if (!sendBytes(&synch, 1)) {
        return false;
    }
    flushEcho(1);
    return true;
}

size_t UpdiPhy::flushEcho(size_t expectedEchoBytes) {
    uint8_t scratch[64];
    size_t totalRead = 0;
    size_t remaining = expectedEchoBytes;
    uint32_t start = millis();
    while (remaining > 0 && _serial) {
        if (_serial->available()) {
            size_t chunk = min(remaining, sizeof(scratch));
            size_t readCount = _serial->readBytes(scratch, chunk);
            if (readCount == 0) {
                break;
            }
            updiDebugHex("ECHO", scratch, readCount);
            totalRead += readCount;
            remaining -= readCount;
            continue;
        }
        if ((millis() - start) > UPDI_ECHO_TIMEOUT_MS) {
            updiDebugFmt("ECHO timeout ms", UPDI_ECHO_TIMEOUT_MS);
            updiDebugFmt("ECHO expected", expectedEchoBytes);
            updiDebugFmt("ECHO got", totalRead);
            break;
        }
        delayMicroseconds(50);
    }
    return totalRead;
}

bool UpdiPhy::restoreOperationalBaud() {
    if (!_serial || _baud == 0) {
        return false;
    }

    _serial->end();
    _serial->begin(_baud, SERIAL_8E2, _rxPin, _txPin, false);
    assignUartPins();
    configureUartMode();
    configureOpenDrainTx();

    delay(10);
    updiDebugFmt("UART restore baud", _baud);
    updiDebugFlushRx(_serial, "RX flush after baud restore");

    _initialized = true;
    return true;
}

bool UpdiPhy::sendBreakCharacter() {
    updiDebugMsg("BREAK @ 300 baud 8E1");
    debugPinStates();

    if (_initialized && _serial) {
        uart_wait_tx_done(UPDI_UART_NUM, pdMS_TO_TICKS(100));
        _serial->end();
        _initialized = false;
    }

    // pymcuprog / serialupdi: 0x00 at 300 baud 8E1 holds the line low ~10 bit times
    // (~33 ms), then read one byte to wait for the frame to finish on the wire.
    _serial->begin(UPDI_BREAK_BAUD, SERIAL_8E1, _rxPin, _txPin, false);
    assignUartPins();
    configureUartMode();
    configureOpenDrainTx();

    const uint8_t breakCharacter = 0x00;
    if (_serial->write(&breakCharacter, 1) != 1) {
        updiDebugMsg("BREAK TX fail");
        return false;
    }
    uart_wait_tx_done(UPDI_UART_NUM, pdMS_TO_TICKS(100));
    updiDebugHex("BREAK TX", &breakCharacter, 1);

    uint8_t discard = 0;
    if (_serial->readBytes(&discard, 1) == 1) {
        updiDebugHex("BREAK drain", &discard, 1);
    } else {
        updiDebugMsg("BREAK drain timeout");
    }

    _serial->end();
    pinMode(_txPin, INPUT_PULLUP);
    debugPinStates();
    return true;
}

bool UpdiPhy::sendDoubleBreak() {
    updiDebugMsg("double BREAK start");
    if (!sendBreakCharacter()) {
        return false;
    }

    delay(UPDI_DOUBLE_BREAK_GAP_MS);

    if (!sendBreakCharacter()) {
        return false;
    }

    updiDebugMsg("double BREAK done, restoring operational baud");
    return restoreOperationalBaud();
}
