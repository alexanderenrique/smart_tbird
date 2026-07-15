#pragma once

#include <Arduino.h>

// ATtiny3226 board packaging + MAX485 / SP3485 wiring.
//
// USART0 alternate mux (Serial.swap(1)):
//   TX = PA1  → DI
//   RX = PA2  ← RO
//   DE+RE = PA3
//
// Arduino digital pin numbers from megaTinyCore txy6 variant.

namespace Pins {

constexpr uint8_t MODBUS_TX = PIN_PA1;    // 14 — DI ← ATtiny TX
constexpr uint8_t MODBUS_RX = PIN_PA2;    // 15 — RO → ATtiny RX
constexpr uint8_t MODBUS_DE_RE = PIN_PA3; // 16 — DE+RE tied; HIGH = drive bus

}  // namespace Pins
