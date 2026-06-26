#pragma once

#include <Arduino.h>

class HwControl {
public:
    void begin();
    void resetTarget();
    void powerOn();
    void powerOff();
};
