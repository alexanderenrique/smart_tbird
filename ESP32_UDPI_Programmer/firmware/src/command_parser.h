#pragma once

#include <Arduino.h>
#include <vector>

#include "pins.h"
#include "hw_control.h"
#include "stream_receiver.h"
#include "uart_bridge.h"
#include "updi/updi_link.h"
#include "updi/updi_nvm.h"
#include "updi/updi_phy.h"

class CommandParser {
public:
    CommandParser(
        UpdiLink &link,
        UpdiPhy &phy,
        UpdiNvm &nvm,
        StreamReceiver &streamReceiver,
        HwControl &hwControl,
        UartBridge &uartBridge
    );

    void setup();
    void loop();

private:
    UpdiLink &_link;
    UpdiPhy &_phy;
    UpdiNvm &_nvm;
    StreamReceiver &_streamReceiver;
    HwControl &_hwControl;
    UartBridge &_uartBridge;

    String _lineBuffer;
    std::vector<uint8_t> _sessionImage;
    uint32_t _sessionStartAddress = 0;
    bool _awaitingChunk = false;
    size_t _chunkRemaining = 0;
    size_t _chunkBufferIndex = 0;
    uint8_t _chunkBuffer[FLASH_PAGE_SIZE];
    unsigned long _lastHeartbeatMs = 0;

    void processLine(const String &line);
    void respond(const String &message);
    void info(const String &message);
    void maybeHeartbeat();
    bool parseBeginProgram(const String &line, uint32_t &size, uint32_t &address);
    void handleVerify();
    void handleReadSignature();
    void handleUpdiProbe();
    void handleErase();
    void handleReset();
    void handleSerialOn();
    void handleSerialOff();
    void handlePowerOn();
    void handlePowerOff();
    void handleRunTest(const String &line);
    void startNextChunk();
    void readBinaryChunk();
};
