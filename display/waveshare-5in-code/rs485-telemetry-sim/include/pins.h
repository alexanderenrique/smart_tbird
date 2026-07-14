#pragma once

// MAX485 / SP3485 breakout defaults for a generic ESP32 DevKit.
// Adjust to match your wiring before flashing.

namespace Pins {

constexpr int MODBUS_RX = 16;    // RO → ESP32 RX
constexpr int MODBUS_TX = 17;    // DI → ESP32 TX
constexpr int MODBUS_DE_RE = 4;  // DE+RE tied together

}  // namespace Pins
