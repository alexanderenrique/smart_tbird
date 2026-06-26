#pragma once

#include <Arduino.h>

class UartBridge {
public:
    void begin();
    void enable();
    void disable();
    void poll();

private:
    bool _enabled = false;
    HardwareSerial *_targetSerial = nullptr;
};
