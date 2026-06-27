#include "updi_link.h"

#include "pins.h"
#include "updi_debug.h"

bool UpdiLink::init() {
    if (!_phy.begin(PIN_UPDI_RX, PIN_UPDI_TX, UPDI_BAUD)) {
        return false;
    }

    if (!_phy.sendDoubleBreak()) {
        return false;
    }

    uint8_t statusA = 0;
    if (checkDatalink(statusA)) {
        return true;
    }

    if (!_phy.sendDoubleBreak()) {
        return false;
    }

    return checkDatalink(statusA);
}

bool UpdiLink::initDatalink() {
    // Debug: STCS writes disabled — BREAK -> SYNCH -> LDCS STATUSA only.
    updiDebugMsg("init datalink skipped (STCS disabled)");
    return true;
}

bool UpdiLink::checkDatalink(uint8_t &statusA) {
    statusA = 0;
    updiDebugMsg("check datalink: LDCS STATUSA");
    if (!ldcs(UPDI_CS_STATUSA, statusA)) {
        updiDebugMsg("check datalink fail: LDCS STATUSA timeout");
        return false;
    }
    updiDebugFmt("STATUSA", statusA);
    return true;
}

bool UpdiLink::readSib(uint8_t *buffer, size_t maxLength, size_t &outLength) {
    outLength = 0;
    if (buffer == nullptr || maxLength == 0) {
        return false;
    }

    uint8_t instruction = UPDI_INST_KEY | UPDI_KEY_SIB | UPDI_SIB_32BYTES;
    updiDebugFmt("SIB KEY inst", instruction);
    if (!sendInstruction(instruction)) {
        updiDebugMsg("read SIB fail: send KEY instruction");
        return false;
    }

    updiDebugMsg("read SIB: waiting for response");
    while (outLength < maxLength) {
        uint8_t byte = 0;
        if (!_phy.receiveBytes(&byte, 1, UPDI_RX_TIMEOUT_MS)) {
            updiDebugFmt("SIB bytes received", outLength);
            break;
        }
        buffer[outLength++] = byte;
        if (byte == '\n' || byte == '\r') {
            break;
        }
    }

    if (outLength > 0) {
        updiDebugHex("SIB data", buffer, outLength);
    }
    return outLength > 0;
}

bool UpdiLink::reset() {
    uint8_t resetValue = 0x59;
    if (!stcs(UPDI_CS_ASI_RESET_REQ, resetValue)) {
        return false;
    }
    delay(50);
    return true;
}

UpdiLink::UpdiLink(UpdiPhy &phy) : _phy(phy) {}

bool UpdiLink::sendInstruction(uint8_t instruction) {
    if (!_phy.sendSynch()) {
        return false;
    }
    if (!_phy.sendBytes(&instruction, 1)) {
        return false;
    }
    _phy.flushEcho(1);
    return true;
}

bool UpdiLink::sendOperand(uint8_t value) {
    if (!_phy.sendBytes(&value, 1)) {
        return false;
    }
    _phy.flushEcho(1);
    return true;
}

bool UpdiLink::sendAddress(uint32_t address, UpdiAddressSize addressSize) {
    uint8_t bytes = static_cast<uint8_t>(addressSize) + 1;
    for (int8_t index = bytes - 1; index >= 0; --index) {
        uint8_t value = static_cast<uint8_t>((address >> (index * 8)) & 0xFF);
        if (!sendOperand(value)) {
            return false;
        }
    }
    return true;
}

size_t UpdiLink::echoLengthForInstruction(uint8_t instruction, UpdiAddressSize addressSize, uint8_t dataLength) const {
    (void)instruction;
    size_t addressBytes = static_cast<size_t>(addressSize) + 1;
    return 1 + addressBytes + dataLength;
}

bool UpdiLink::stcs(uint8_t address, uint8_t value) {
    (void)address;
    (void)value;
    updiDebugMsg("STCS disabled (debug)");
    return false;
}

bool UpdiLink::ldcs(uint8_t address, uint8_t &value) {
    uint8_t instruction = UPDI_INST_LDCS | (address & 0x0F);
    updiDebugFmt("LDCS addr", address);
    updiDebugFmt("LDCS inst", instruction);
    if (!sendInstruction(instruction)) {
        updiDebugMsg("LDCS fail: send instruction");
        return false;
    }
    if (!_phy.receiveBytes(&value, 1)) {
        updiDebugMsg("LDCS fail: no response byte");
        return false;
    }
    updiDebugFmt("LDCS data", value);
    return true;
}

bool UpdiLink::sts(uint32_t address, const uint8_t *data, uint8_t length, UpdiAddressSize addressSize) {
    uint8_t instruction = UPDI_INST_STS | (static_cast<uint8_t>(addressSize) << 2) | ((length - 1) & 0x03);
    if (!sendInstruction(instruction)) {
        return false;
    }
    if (!sendAddress(address, addressSize)) {
        return false;
    }
    for (uint8_t index = 0; index < length; ++index) {
        if (!sendOperand(data[index])) {
            return false;
        }
    }
    return true;
}

bool UpdiLink::lds(uint32_t address, uint8_t *data, uint8_t length, UpdiAddressSize addressSize) {
    uint8_t instruction = UPDI_INST_LDS | (static_cast<uint8_t>(addressSize) << 2) | ((length - 1) & 0x03);
    if (!sendInstruction(instruction)) {
        return false;
    }
    if (!sendAddress(address, addressSize)) {
        return false;
    }
    return _phy.receiveBytes(data, length);
}

bool UpdiLink::key64(uint64_t key) {
    uint8_t instruction = UPDI_INST_KEY | 0x03;
    if (!sendInstruction(instruction)) {
        return false;
    }

    for (uint8_t index = 0; index < 8; ++index) {
        uint8_t value = static_cast<uint8_t>((key >> (index * 8)) & 0xFF);
        if (!sendOperand(value)) {
            return false;
        }
    }
    return true;
}

bool UpdiLink::repeat(uint16_t count) {
    uint8_t instruction = UPDI_INST_REPEAT | ((count >> 8) & 0x0F);
    if (!sendInstruction(instruction)) {
        return false;
    }
    return sendOperand(static_cast<uint8_t>(count & 0xFF));
}

bool UpdiLink::st(uint8_t data) {
    uint8_t instruction = UPDI_INST_ST | 0x00;
    if (!sendInstruction(instruction)) {
        return false;
    }
    return sendOperand(data);
}

bool UpdiLink::ld(uint8_t &data) {
    uint8_t instruction = UPDI_INST_LD | 0x00;
    if (!sendInstruction(instruction)) {
        return false;
    }
    return _phy.receiveBytes(&data, 1);
}
