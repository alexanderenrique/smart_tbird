#pragma once

#include <stdint.h>

#include "updi_phy.h"

enum UpdiAddressSize : uint8_t {
    UPDI_ADDRESS_8 = 0,
    UPDI_ADDRESS_16 = 1,
    UPDI_ADDRESS_24 = 2,
};

enum UpdiDataSize : uint8_t {
    UPDI_DATA_8 = 0,
    UPDI_DATA_16 = 1,
};

class UpdiLink {
public:
    explicit UpdiLink(UpdiPhy &phy);

    bool init();
    bool initDatalink();
    bool checkDatalink(uint8_t &statusA);
    bool readSib(uint8_t *buffer, size_t maxLength, size_t &outLength);
    bool reset();
    bool stcs(uint8_t address, uint8_t value);
    bool ldcs(uint8_t address, uint8_t &value);
    bool sts(uint32_t address, const uint8_t *data, uint8_t length, UpdiAddressSize addressSize);
    bool lds(uint32_t address, uint8_t *data, uint8_t length, UpdiAddressSize addressSize);
    bool key64(uint64_t key);
    bool repeat(uint16_t count);
    bool st(uint8_t data);
    bool ld(uint8_t &data);

private:
    UpdiPhy &_phy;

    bool sendInstruction(uint8_t instruction);
    bool sendOperand(uint8_t value);
    bool sendAddress(uint32_t address, UpdiAddressSize addressSize);
    size_t echoLengthForInstruction(uint8_t instruction, UpdiAddressSize addressSize, uint8_t dataLength) const;
};

// UPDI control/status register addresses.
static const uint8_t UPDI_CS_STATUSA = 0x00;
static const uint8_t UPDI_CS_STATUSB = 0x01;
static const uint8_t UPDI_CS_CTRLA = 0x02;
static const uint8_t UPDI_CS_CTRLB = 0x03;
static const uint8_t UPDI_CS_CTRLC = 0x09;
static const uint8_t UPDI_CS_ASI_KEY_STATUS = 0x07;
static const uint8_t UPDI_CS_ASI_RESET_REQ = 0x08;
static const uint8_t UPDI_CS_ASI_SYS_CTRLA = 0x0C;
static const uint8_t UPDI_CS_ASI_SYS_STATUS = 0x0B;

// UPDI instruction opcodes (upper 6 bits + size bits).
static const uint8_t UPDI_INST_STCS = 0xC0;
static const uint8_t UPDI_INST_LDCS = 0x80;
static const uint8_t UPDI_INST_STS = 0x40;
static const uint8_t UPDI_INST_LDS = 0x00;
static const uint8_t UPDI_INST_KEY = 0xE0;
static const uint8_t UPDI_INST_REPEAT = 0xA0;
static const uint8_t UPDI_INST_ST = 0x60;
static const uint8_t UPDI_INST_LD = 0x20;

static const uint8_t UPDI_KEY_SIB = 0x04;
static const uint8_t UPDI_SIB_32BYTES = 0x02;

static const uint8_t UPDI_CTRLA_IBDLY = 0x80;
static const uint8_t UPDI_CTRLB_CCDETDIS = 0x08;

// Key signatures (shift LSB first).
static const uint64_t UPDI_KEY_CHIP_ERASE = 0x4E564D4572617365ULL;
static const uint64_t UPDI_KEY_NVM_PROG = 0x4E564D50726F6720ULL;
