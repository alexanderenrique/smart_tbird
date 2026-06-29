#include "updi_phy.h"

#include "driver/gpio.h"
#include "driver/uart.h"
#include "pins.h"
#include "updi_debug.h"

static constexpr uart_port_t UPDI_UART_NUM = UART_NUM_2;

// Match test/uart_tx_0x55 configureUpdiPhy(): UART mode + open-drain TX pull-up.
void UpdiPhy::applyUpdiPhyConfig() {
    uart_set_mode(UPDI_UART_NUM, UART_MODE_UART);
    const gpio_num_t txGpio = static_cast<gpio_num_t>(_txPin);
    gpio_set_pull_mode(txGpio, GPIO_PULLUP_ONLY);
    gpio_set_direction(txGpio, GPIO_MODE_INPUT_OUTPUT_OD);
}

// Release UART2 and both tied-bus pads (see test/tx2_toggle_1hz releaseUart2Pins).
void UpdiPhy::haltUart() {
    if (_serial && _initialized) {
        uart_wait_tx_done(UPDI_UART_NUM, pdMS_TO_TICKS(50));
    }
    if (_serial) {
        _serial->end();
    }
    _initialized = false;

    const gpio_num_t rxGpio = static_cast<gpio_num_t>(_rxPin);
    const gpio_num_t txGpio = static_cast<gpio_num_t>(_txPin);
    gpio_reset_pin(rxGpio);
    gpio_reset_pin(txGpio);
    gpio_set_direction(rxGpio, GPIO_MODE_INPUT);
    gpio_set_pull_mode(rxGpio, GPIO_FLOATING);
}

bool UpdiPhy::openUart(uint32_t baud, uint32_t config) {
    if (!_serial) {
        return false;
    }

    haltUart();
    _serial->begin(baud, config, _rxPin, _txPin, false);
    applyUpdiPhyConfig();

    while (_serial->available()) {
        _serial->read();
    }

    _initialized = true;
    return true;
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

    if (!openUart(baud, SERIAL_8E2)) {
        return false;
    }

    updiDebugFmt("UART open RX GPIO", rxPin);
    updiDebugFmt("UART open TX GPIO", txPin);
    updiDebugFmt("UART baud", baud);
    debugPinStates();
    updiDebugFlushRx(_serial, "RX flush after open");

    return true;
}

void UpdiPhy::end() {
    haltUart();
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

bool UpdiPhy::sendSynchBurst(uint16_t count) {
    if (!_initialized || !_serial || count == 0) {
        return false;
    }

    const uint8_t synch = 0x55;
    updiDebugFmt("SYNCH burst count", count);
    for (uint16_t index = 0; index < count; ++index) {
        if (_serial->write(&synch, 1) != 1) {
            updiDebugMsg("SYNCH burst TX fail");
            return false;
        }
    }
    uart_wait_tx_done(UPDI_UART_NUM, pdMS_TO_TICKS(100));
    flushEcho(count);
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

    if (!openUart(_baud, SERIAL_8E2)) {
        return false;
    }

    updiDebugFmt("UART restore baud", _baud);
    updiDebugFlushRx(_serial, "RX flush after baud restore");
    return true;
}

bool UpdiPhy::sendBreakPulse() {
    updiDebugFmt("BREAK pulse us", UPDI_BREAK_LOW_US);
    debugPinStates();

    haltUart();

    const gpio_num_t txGpio = static_cast<gpio_num_t>(_txPin);
    gpio_set_pull_mode(txGpio, GPIO_PULLUP_ONLY);
    gpio_set_direction(txGpio, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_level(txGpio, 0);
    delayMicroseconds(UPDI_BREAK_LOW_US);
    gpio_set_level(txGpio, 1);

    debugPinStates();
    return true;
}

bool UpdiPhy::sendBreak() {
    if (!sendBreakPulse()) {
        return false;
    }
    return restoreOperationalBaud();
}

bool UpdiPhy::sendDoubleBreak() {
    updiDebugMsg("double BREAK start");
    if (!sendBreakPulse()) {
        return false;
    }

    delayMicroseconds(UPDI_BREAK_GAP_US);

    if (!sendBreakPulse()) {
        return false;
    }

    updiDebugMsg("double BREAK done, restoring operational baud");
    return restoreOperationalBaud();
}
