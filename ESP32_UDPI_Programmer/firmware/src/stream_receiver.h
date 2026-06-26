#pragma once

#include <Arduino.h>
#include <vector>

#include "pins.h"
#include "updi/updi_nvm.h"

class StreamReceiver {
public:
    explicit StreamReceiver(UpdiNvm &nvm);

    void begin(uint32_t size, uint32_t startAddress, std::vector<uint8_t> &sessionImage);
    bool consumeChunk(const uint8_t *data, size_t length, String &response);
    bool finalize(String &response);
    bool active() const;
    uint32_t remainingBytes() const;

private:
    UpdiNvm &_nvm;
    bool _active = false;
    uint32_t _expectedSize = 0;
    uint32_t _receivedSize = 0;
    uint32_t _startAddress = 0;
    uint32_t _currentAddress = 0;
    uint8_t _pageBuffer[FLASH_PAGE_SIZE];
    size_t _pageOffset = 0;
    std::vector<uint8_t> *_sessionImage = nullptr;

    bool flushPage(String &response);
};
