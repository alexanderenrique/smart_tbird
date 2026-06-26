#include "stream_receiver.h"

#include <string.h>

#include "pins.h"

StreamReceiver::StreamReceiver(UpdiNvm &nvm) : _nvm(nvm) {}

void StreamReceiver::begin(uint32_t size, uint32_t startAddress, std::vector<uint8_t> &sessionImage) {
    _active = true;
    _expectedSize = size;
    _receivedSize = 0;
    _startAddress = startAddress;
    _currentAddress = startAddress;
    _pageOffset = 0;
    _sessionImage = &sessionImage;
    memset(_pageBuffer, 0xFF, sizeof(_pageBuffer));
}

bool StreamReceiver::active() const {
    return _active;
}

uint32_t StreamReceiver::remainingBytes() const {
    if (!_active || _receivedSize >= _expectedSize) {
        return 0;
    }
    return _expectedSize - _receivedSize;
}

bool StreamReceiver::flushPage(String &response) {
    if (_pageOffset == 0) {
        return true;
    }

    if (!_nvm.writeFlashPage(_currentAddress, _pageBuffer, _pageOffset)) {
        response = "ERROR write failed";
        _active = false;
        return false;
    }

    _currentAddress += _pageOffset;
    _pageOffset = 0;
    memset(_pageBuffer, 0xFF, sizeof(_pageBuffer));
    response = "OK";
    return true;
}

bool StreamReceiver::consumeChunk(const uint8_t *data, size_t length, String &response) {
    if (!_active) {
        response = "ERROR not in program mode";
        return false;
    }

    for (size_t index = 0; index < length; ++index) {
        uint8_t value = data[index];
        if (_sessionImage && _receivedSize < _sessionImage->size()) {
            (*_sessionImage)[_receivedSize] = value;
        }

        _pageBuffer[_pageOffset++] = value;
        _receivedSize++;

        if (_pageOffset >= FLASH_PAGE_SIZE) {
            if (!flushPage(response)) {
                return false;
            }
        }
    }

    response = "OK";
    return true;
}

bool StreamReceiver::finalize(String &response) {
    if (!_active) {
        response = "ERROR not in program mode";
        return false;
    }

    if (_pageOffset > 0) {
        if (!flushPage(response)) {
            return false;
        }
    }

    if (_receivedSize != _expectedSize) {
        response = "ERROR size mismatch";
        _active = false;
        return false;
    }

    _active = false;
    response = "OK";
    return true;
}
