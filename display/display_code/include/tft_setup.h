// TFT_eSPI configuration for 320x480 portrait display (ILI9488 default).
// Edit driver and pins to match your panel after breadboarding.
//
// Included via platformio.ini: -include include/tft_setup.h

#pragma once

#define USER_SETUP_LOADED 1

// Driver — change if your panel uses a different controller
#define ILI9488_DRIVER

// ESP32-S3: Arduino FSPI=0 but SPI peripheral registers use port 2 (GPSPI2).
// Without this, TFT_eSPI writes to address 0x10 and crashes on tft.init().
#define USE_FSPI_PORT

#define TFT_WIDTH  320
#define TFT_HEIGHT 480

// TFT Pins (GPIO 9–14 are the FSPI bus on ESP32-S3)

// XPT2046 touch chip select (T_CS); shares SPI bus with TFT (MISO/MOSI/SCLK above).
#define TOUCH_CS 8 
#define TFT_SCLK 10
#define TFT_MOSI 11 //Touch_DIN
#define TFT_DC 12  //Data/Command (DC/RS)
#define TFT_RST 13
#define TFT_CS 14
// XPT2046 T_DO (MISO) — TFT_eSPI reads touch on TFT_MISO, not TOUCH_DO (NEMO: TFT_MISO 32)
#define TFT_MISO 18

// Set to 1 for verbose touch/SPI diagnostics on Serial (115200)
#define TOUCH_DEBUG 1



#define TFT_BACKLIGHT_ON HIGH

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

#define SPI_FREQUENCY       10000000
#define SPI_READ_FREQUENCY  10000000
#define SPI_TOUCH_FREQUENCY 2500000

// Pressure threshold passed to tft.getTouch() — lower if Z reads 200-400 while pressing
#define TOUCH_PRESSURE_THRESHOLD 200

// Required on ESP32 when TFT and XPT2046 share SPI (auto-set on S3; explicit for parity with NEMO)
#define SUPPORT_TRANSACTIONS

// ESP32-S3: use HSPI if default FSPI pins conflict with your wiring
// #define USE_HSPI_PORT
