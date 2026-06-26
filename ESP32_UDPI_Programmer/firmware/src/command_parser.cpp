#include "command_parser.h"
#include "pins.h"
#include "updi/updi_probe.h"

static const char *DEVICE_ID = "ESP32-UPDI v1.0";

CommandParser::CommandParser(
    UpdiLink &link,
    UpdiPhy &phy,
    UpdiNvm &nvm,
    StreamReceiver &streamReceiver,
    HwControl &hwControl,
    UartBridge &uartBridge
)
    : _link(link),
      _phy(phy),
      _nvm(nvm),
      _streamReceiver(streamReceiver),
      _hwControl(hwControl),
      _uartBridge(uartBridge) {}

static const unsigned long HEARTBEAT_INTERVAL_MS = 30000;

void CommandParser::setup() {
    _lineBuffer.reserve(128);
    _sessionImage.clear();
    _lastHeartbeatMs = millis();
}

void CommandParser::respond(const String &message) {
    Serial.println(message);
}

void CommandParser::info(const String &message) {
    Serial.print("INFO ");
    Serial.println(message);
}

void CommandParser::maybeHeartbeat() {
    if (_awaitingChunk || _streamReceiver.active()) {
        return;
    }

    unsigned long now = millis();
    if (now - _lastHeartbeatMs < HEARTBEAT_INTERVAL_MS) {
        return;
    }

    _lastHeartbeatMs = now;
    info("alive - waiting for .hex file");
}

void CommandParser::loop() {
    _uartBridge.poll();
    maybeHeartbeat();

    if (_awaitingChunk) {
        readBinaryChunk();
        return;
    }

    while (Serial.available()) {
        char character = static_cast<char>(Serial.read());
        if (character == '\n' || character == '\r') {
            if (_lineBuffer.length() > 0) {
                processLine(_lineBuffer);
                _lineBuffer = "";
            }
            continue;
        }
        _lineBuffer += character;
    }
}

bool CommandParser::parseBeginProgram(const String &line, uint32_t &size, uint32_t &address) {
    int sizeIndex = line.indexOf("size=");
    if (sizeIndex < 0) {
        return false;
    }

    size = static_cast<uint32_t>(line.substring(sizeIndex + 5).toInt());

    address = FLASH_START;
    int addrIndex = line.indexOf("addr=");
    if (addrIndex >= 0) {
        String addrText = line.substring(addrIndex + 5);
        int spaceIndex = addrText.indexOf(' ');
        if (spaceIndex >= 0) {
            addrText = addrText.substring(0, spaceIndex);
        }
        addrText.trim();
        if (addrText.startsWith("0x") || addrText.startsWith("0X")) {
            address = strtoul(addrText.c_str(), nullptr, 16);
        } else {
            address = static_cast<uint32_t>(addrText.toInt());
        }
    }

    return size > 0;
}

void CommandParser::startNextChunk() {
    if (!_streamReceiver.active()) {
        _awaitingChunk = false;
        _chunkRemaining = 0;
        return;
    }

    uint32_t remaining = _streamReceiver.remainingBytes();
    if (remaining == 0) {
        _awaitingChunk = false;
        _chunkRemaining = 0;
        return;
    }

    _chunkRemaining = min(static_cast<size_t>(remaining), static_cast<size_t>(FLASH_PAGE_SIZE));
    _awaitingChunk = true;
    _chunkBufferIndex = 0;
}

void CommandParser::readBinaryChunk() {
    while (_chunkRemaining > 0 && Serial.available()) {
        _chunkBuffer[_chunkBufferIndex++] = static_cast<uint8_t>(Serial.read());
        _chunkRemaining--;
    }

    if (_chunkRemaining > 0) {
        return;
    }

    String response;
    if (!_streamReceiver.consumeChunk(_chunkBuffer, _chunkBufferIndex, response)) {
        respond(response);
        _awaitingChunk = false;
        return;
    }

    respond("OK");
    startNextChunk();
}

void CommandParser::processLine(const String &line) {
    String trimmed = line;
    trimmed.trim();
    if (trimmed.length() == 0) {
        return;
    }

    if (trimmed == "HELLO") {
        respond(String("OK ") + DEVICE_ID);
        return;
    }

    if (trimmed.startsWith("BEGIN PROGRAM")) {
        uint32_t size = 0;
        uint32_t address = FLASH_START;
        if (!parseBeginProgram(trimmed, size, address)) {
            respond("ERROR invalid BEGIN PROGRAM");
            return;
        }

        _sessionImage.assign(size, 0xFF);
        _sessionStartAddress = address;
        _streamReceiver.begin(size, address, _sessionImage);
        respond("OK");
        info(String("receiving ") + size + " bytes...");
        startNextChunk();
        return;
    }

    if (trimmed == "END PROGRAM") {
        String response;
        if (!_streamReceiver.finalize(response)) {
            respond(response);
            return;
        }
        _awaitingChunk = false;
        _chunkRemaining = 0;
        respond("OK");
        info("programming complete - success");
        return;
    }

    if (_streamReceiver.active()) {
        respond("ERROR programming in progress");
        return;
    }

    if (trimmed == "VERIFY") {
        handleVerify();
        return;
    }
    if (trimmed == "READ_SIGNATURE") {
        handleReadSignature();
        return;
    }
    if (trimmed == "UPDI_PROBE") {
        handleUpdiProbe();
        return;
    }
    if (trimmed == "ERASE") {
        handleErase();
        return;
    }
    if (trimmed == "RESET") {
        handleReset();
        return;
    }
    if (trimmed == "SERIAL ON") {
        handleSerialOn();
        return;
    }
    if (trimmed == "SERIAL OFF") {
        handleSerialOff();
        return;
    }
    if (trimmed == "POWER ON") {
        handlePowerOn();
        return;
    }
    if (trimmed == "POWER OFF") {
        handlePowerOff();
        return;
    }
    if (trimmed.startsWith("RUN TEST")) {
        handleRunTest(trimmed);
        return;
    }

    respond("ERROR unknown command");
}

void CommandParser::handleVerify() {
    if (_sessionImage.empty()) {
        respond("ERROR no programmed image");
        return;
    }

    uint32_t failAddress = 0;
    if (_nvm.verifyFlash(
            _sessionStartAddress,
            _sessionImage.data(),
            _sessionImage.size(),
            failAddress)) {
        respond("OK");
        info("verify OK");
        return;
    }

    respond(String("ERROR addr=0x") + String(failAddress, HEX));
}

void CommandParser::handleUpdiProbe() {
    UpdiProbe probe(_link, _phy);
    UpdiProbeResult result;
    probe.run(result);
    probe.printResult(Serial, result);
    if (result.success) {
        respond("OK");
        return;
    }
    respond("ERROR UPDI probe failed");
}

void CommandParser::handleReadSignature() {
    uint8_t signature[3] = {0};
    if (!_nvm.readSignature(signature)) {
        respond("ERROR read signature failed");
        return;
    }

    char buffer[32];
    snprintf(
        buffer,
        sizeof(buffer),
        "OK %02X %02X %02X",
        signature[0],
        signature[1],
        signature[2]
    );
    respond(buffer);
}

void CommandParser::handleErase() {
    if (!_nvm.chipErase()) {
        respond("ERROR erase failed");
        return;
    }
    _sessionImage.clear();
    respond("OK");
}

void CommandParser::handleReset() {
    _hwControl.resetTarget();
    respond("OK");
}

void CommandParser::handleSerialOn() {
    _uartBridge.enable();
    respond("OK");
}

void CommandParser::handleSerialOff() {
    _uartBridge.disable();
    respond("OK");
}

void CommandParser::handlePowerOn() {
    _hwControl.powerOn();
    respond("OK");
}

void CommandParser::handlePowerOff() {
    _hwControl.powerOff();
    respond("OK");
}

void CommandParser::handleRunTest(const String &line) {
    (void)line;
    respond("ERROR NOT IMPLEMENTED");
}
