#pragma once

// Pin map for ESP32 ATtiny UPDI programmer hardware.
// See ../README.md for wiring details.
//
// ESP32 DevKit layout (Phase 1):
//   GPIO 16 - UPDI RX2 (tie to GPIO 17, then 4.7k to target UPDI)
//   GPIO 17 - UPDI TX2 (tie to GPIO 16, then 4.7k to target UPDI)
//   GPIO 18 - target RESET (active low)
//   3.3V    - target VCC (direct from ESP32, no GPIO)
//
// Phase 2: UART bridge to target application serial (TBD GPIOs, e.g. 4/5 or 21/22).
//
// GPIO 1/3 (primary UART header) are reserved for USB serial to the PC.

// USB serial to PC (must match platformio.ini monitor_speed and attiny-uploader --baud).
static const uint32_t USB_SERIAL_BAUD = 9600;

// UPDI on Serial2: hardware-tie RX2 and TX2, then 4.7k series resistor to target UPDI.
static const int PIN_UPDI_RX = 16;
static const int PIN_UPDI_TX = 17;

// Active-low target reset.
static const int PIN_TARGET_RESET = 18;

// Phase 2: UART bridge to target application serial (Serial1, pins TBD).
// static const int PIN_BRIDGE_RX = 4;
// static const int PIN_BRIDGE_TX = 5;

// UPDI line baud rate (serialupdi / pymcuprog default; SYNCH retrains after BREAK).
static const uint32_t UPDI_BAUD = 115200;

// Verbose hex-level UPDI logging on USB serial.
static const bool UPDI_DEBUG = true;

// BREAK baud rate and framing (pymcuprog serialupdi convention).
static const uint32_t UPDI_BREAK_BAUD = 300;

// Gap between the two BREAKs in a double-BREAK sequence (ms).
static const uint32_t UPDI_DOUBLE_BREAK_GAP_MS = 100;

// RX timeout for UPDI response bytes (ms).
static const uint32_t UPDI_RX_TIMEOUT_MS = 300;

// Echo drain timeout after TX (ms).
static const uint32_t UPDI_ECHO_TIMEOUT_MS = 100;

// ATtiny3216 flash page size.
static const size_t FLASH_PAGE_SIZE = 128;

// Flash base address in UPDI data space.
static const uint32_t FLASH_START = 0x8000;
