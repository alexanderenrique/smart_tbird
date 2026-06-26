#pragma once

#include <stdint.h>

#include "updi_link.h"

class UpdiNvm {
public:
    explicit UpdiNvm(UpdiLink &link);

    bool readSignature(uint8_t signature[3]);
    bool chipErase();
    bool writeFlashPage(uint32_t address, const uint8_t *data, size_t length);
    bool readFlash(uint32_t address, uint8_t *data, size_t length);
    bool verifyFlash(uint32_t address, const uint8_t *expected, size_t length, uint32_t &failAddress);

private:
    UpdiLink &_link;

    bool enterNvmProg();
    bool waitNvmReady();
    bool setPointer(uint32_t address);
    bool pageWrite();
};

// ATtiny3216 NVMCTRL registers in IO memory.
static const uint16_t NVMCTRL_CTRLA = 0x1000;
static const uint16_t NVMCTRL_CTRLB = 0x1001;
static const uint16_t NVMCTRL_STATUS = 0x1002;
static const uint16_t CPU_CCP = 0x0034;

static const uint8_t NVM_CMD_PAGE_WRITE = 0x06;
static const uint8_t NVM_CMD_PAGE_ERASE = 0x07;
static const uint8_t NVM_CMD_PAGE_BUFFER_CLEAR = 0x08;

// Signature row base.
static const uint16_t SIGROW_BASE = 0x1100;

// Expected ATtiny3216 signature bytes.
static const uint8_t SIGNATURE_0 = 0x1E;
static const uint8_t SIGNATURE_1 = 0x95;
static const uint8_t SIGNATURE_2 = 0x21;
