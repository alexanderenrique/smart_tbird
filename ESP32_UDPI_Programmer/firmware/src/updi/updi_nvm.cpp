#include "updi_nvm.h"

#include "pins.h"

UpdiNvm::UpdiNvm(UpdiLink &link) : _link(link) {}

bool UpdiNvm::readSignature(uint8_t signature[3]) {
    return _link.lds(SIGROW_BASE, signature, 3, UPDI_ADDRESS_16);
}

bool UpdiNvm::enterNvmProg() {
    if (!_link.key64(UPDI_KEY_NVM_PROG)) {
        return false;
    }

    for (uint8_t attempt = 0; attempt < 50; ++attempt) {
        uint8_t status = 0;
        if (!_link.ldcs(UPDI_CS_ASI_SYS_STATUS, status)) {
            return false;
        }
        if (status & 0x08) {
            return true;
        }
        delay(5);
    }
    return false;
}

bool UpdiNvm::waitNvmReady() {
    for (uint8_t attempt = 0; attempt < 100; ++attempt) {
        uint8_t status = 0;
        if (!_link.lds(NVMCTRL_STATUS, &status, 1, UPDI_ADDRESS_16)) {
            return false;
        }
        if ((status & 0x03) == 0) {
            return true;
        }
        delay(2);
    }
    return false;
}

bool UpdiNvm::setPointer(uint32_t address) {
    uint8_t ptrHigh = static_cast<uint8_t>((address >> 16) & 0xFF);
    uint8_t ptrMid = static_cast<uint8_t>((address >> 8) & 0xFF);
    uint8_t ptrLow = static_cast<uint8_t>(address & 0xFF);

    if (!_link.stcs(UPDI_CS_CTRLB, ptrMid)) {
        return false;
    }
    if (!_link.stcs(UPDI_CS_CTRLA, ptrLow)) {
        return false;
    }
    return _link.stcs(UPDI_CS_CTRLC, ptrHigh);
}

bool UpdiNvm::pageWrite() {
    const uint8_t ccpValue = 0x9D;
    if (!_link.sts(CPU_CCP, &ccpValue, 1, UPDI_ADDRESS_8)) {
        return false;
    }

    const uint8_t command = NVM_CMD_PAGE_WRITE;
    if (!_link.sts(NVMCTRL_CTRLA, &command, 1, UPDI_ADDRESS_16)) {
        return false;
    }

    return waitNvmReady();
}

bool UpdiNvm::chipErase() {
    if (!_link.key64(UPDI_KEY_CHIP_ERASE)) {
        return false;
    }
    delay(100);
    return _link.reset();
}

bool UpdiNvm::writeFlashPage(uint32_t address, const uint8_t *data, size_t length) {
    if (length == 0 || length > FLASH_PAGE_SIZE) {
        return false;
    }

    if (!enterNvmProg()) {
        return false;
    }

    if (!setPointer(address)) {
        return false;
    }

    if (length > 1) {
        if (!_link.repeat(static_cast<uint16_t>(length - 1))) {
            return false;
        }
    }

    for (size_t index = 0; index < length; ++index) {
        if (!_link.st(data[index])) {
            return false;
        }
    }

    return pageWrite();
}

bool UpdiNvm::readFlash(uint32_t address, uint8_t *data, size_t length) {
    for (size_t offset = 0; offset < length; ++offset) {
        if (!_link.lds(address + offset, &data[offset], 1, UPDI_ADDRESS_24)) {
            return false;
        }
    }
    return true;
}

bool UpdiNvm::verifyFlash(uint32_t address, const uint8_t *expected, size_t length, uint32_t &failAddress) {
    for (size_t offset = 0; offset < length; ++offset) {
        uint8_t actual = 0;
        if (!_link.lds(address + offset, &actual, 1, UPDI_ADDRESS_24)) {
            failAddress = address + offset;
            return false;
        }
        if (actual != expected[offset]) {
            failAddress = address + offset;
            return false;
        }
    }
    return true;
}
