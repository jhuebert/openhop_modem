#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>

#define USB_VID 0x303a
#define USB_PID 0x1001

static const uint8_t TX = 43;
static const uint8_t RX = 44;

// Sensor/OLED I2C bus (default Wire). The PMU/RTC live on a second bus
// (Wire1, SDA=42/SCL=41) — see BOARD.pmu in boards/lilygo_tbeam_s3_supreme.h.
static const uint8_t SDA = 17;
static const uint8_t SCL = 18;

// SX1262 SPI bus — firmware calls SPI.begin() explicitly with these pins
// via BOARD.pin_lora_*, but declare the board defaults too.
static const uint8_t SS   = 10;
static const uint8_t MOSI = 11;
static const uint8_t MISO = 13;
static const uint8_t SCK  = 12;

#endif /* Pins_Arduino_h */
