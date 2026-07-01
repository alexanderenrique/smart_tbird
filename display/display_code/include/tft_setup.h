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

// Touch disabled for display bring-up; GPIO 3 is a strapping pin on S3
// #define TOUCH_CS 3
#ifndef TFT_MISO 
#define TFT_MISO 9 //Touch_DO
#endif
#ifndef TFT_SCLK
#define TFT_SCLK 10
#endif
#ifndef TFT_MOSI
#define TFT_MOSI 11 //Touch_DIN
#endif
#ifndef TFT_DC
#define TFT_DC 12  //Data/Command (DC/RS)
#endif
#ifndef TFT_RST
#define TFT_RST 13
#endif
#ifndef TFT_CS
#define TFT_CS 14
#endif






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

// ESP32-S3: use HSPI if default FSPI pins conflict with your wiring
// #define USE_HSPI_PORT
