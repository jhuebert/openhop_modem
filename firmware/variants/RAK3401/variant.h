// =============================================================
// variants/RAK3401/variant.h — Adafruit nRF52 BSP pin map for the
// RAKwireless RAK3401 (nRF52840 WisBlock core + RAK13302 SX1262 /
// SKY66122-11 1 W front end on the WisBlock SPI slot).
//
// Ported from the MeshCore `rak3401` variant. Unlike the RAK4631
// targets in this repo the SX1262 is NOT the in-module radio on
// P1.10..P1.15 — it sits on the WisBlock SPI slot, so the default
// SPI bus (PIN_SPI_*) is the LoRa bus here and main.cpp's generic
// `SPI.begin()` path applies.
//
// WisBlock slot ↔ raw nRF52840 GPIO ↔ RAK13302 signal
//   WB_SPI_CLK  = P0.03 = 3   → SX1262 SCK
//   WB_IO4      = P0.04 = 4   → SX1262 NRESET
//   WB_IO5      = P0.09 = 9   → SX1262 BUSY
//   WB_IO6      = P0.10 = 10  → SX1262 DIO1 (IRQ)
//   WB_SPI_CS   = P0.26 = 26  → SX1262 NSS
//   WB_SPI_MISO = P0.29 = 29  → SX1262 MISO
//   WB_SPI_MOSI = P0.30 = 30  → SX1262 MOSI
//   WB_IO3      = P0.21 = 21  → SKY66122 CSD+CPS (tied, FEM enable)
//   WB_IO2      = P0.34 = 34  → 3V3_S rail + 5 V PA boost enable
//
// Identity pin map in variant.cpp: Arduino pin N == raw nRF GPIO N
// (P1.x encoded as 32 + x), same scheme as the other nRF52 boards
// in this tree.
// =============================================================
#ifndef _VARIANT_RAK3401_OPENHOP_
#define _VARIANT_RAK3401_OPENHOP_

#define RAK4630
#define VARIANT_MCK (64000000ul)
#define USE_LFXO  // 32.768 kHz crystal on the WisBlock core

#include "WVariant.h"

#ifdef __cplusplus
extern "C" {
#endif

static const uint8_t WB_IO1 = 17;
static const uint8_t WB_IO2 = 34;
static const uint8_t WB_IO3 = 21;
static const uint8_t WB_IO4 = 4;
static const uint8_t WB_IO5 = 9;
static const uint8_t WB_IO6 = 10;
static const uint8_t WB_SW1 = 33;
static const uint8_t WB_I2C1_SDA = 13;
static const uint8_t WB_I2C1_SCL = 14;
static const uint8_t WB_I2C2_SDA = 24;
static const uint8_t WB_I2C2_SCL = 25;
static const uint8_t WB_SPI_CS = 26;
static const uint8_t WB_SPI_CLK = 3;
static const uint8_t WB_SPI_MISO = 29;
static const uint8_t WB_SPI_MOSI = 30;

#define PINS_COUNT (48)
#define NUM_DIGITAL_PINS (48)
#define NUM_ANALOG_INPUTS (6)
#define NUM_ANALOG_OUTPUTS (0)

#define PIN_LED1 (35)
#define PIN_LED2 (36)
#define LED_BUILTIN PIN_LED1
#define LED_CONN PIN_LED2
#define LED_GREEN PIN_LED1
#define LED_BLUE PIN_LED2
#define LED_STATE_ON 1

// Analog inputs. A0 (P0.05 = AIN3) is the battery divider tap.
#define PIN_A0 (5)
#define PIN_A1 (31)
#define PIN_A2 (28)
#define PIN_A3 (29)
#define PIN_A4 (30)
#define PIN_A5 (31)
static const uint8_t A0 = PIN_A0;
static const uint8_t A1 = PIN_A1;
static const uint8_t A2 = PIN_A2;
static const uint8_t A3 = PIN_A3;
static const uint8_t A4 = PIN_A4;
static const uint8_t A5 = PIN_A5;
#define ADC_RESOLUTION 14

#define PIN_AREF (2)
static const uint8_t AREF = PIN_AREF;

#define PIN_NFC1 (9)
#define PIN_NFC2 (10)

// UART1 = WisBlock base TXD1/RXD1 (RAK1910 GNSS lands here).
#define PIN_SERIAL1_RX (15)
#define PIN_SERIAL1_TX (16)
// UART2 exists only so main.cpp's PROTO_UART symbol links; the
// board config leaves the protocol UART disabled, so it is never
// .begin()'d. Pins mirror the core module's J-Link CDC pads.
#define PIN_SERIAL2_RX (8)
#define PIN_SERIAL2_TX (6)

// Default SPI bus == RAK13302 SX1262 on the WisBlock SPI slot.
#define SPI_INTERFACES_COUNT 1
#define PIN_SPI_MISO (29)
#define PIN_SPI_MOSI (30)
#define PIN_SPI_SCK (3)
#define PIN_SPI_NSS (26)
static const uint8_t SS = 26;
static const uint8_t MOSI = PIN_SPI_MOSI;
static const uint8_t MISO = PIN_SPI_MISO;
static const uint8_t SCK = PIN_SPI_SCK;

#define WIRE_INTERFACES_COUNT 2
#define PIN_WIRE_SDA (13)
#define PIN_WIRE_SCL (14)
#define PIN_WIRE1_SDA (24)
#define PIN_WIRE1_SCL (25)

// Battery sense: LiPo divider tap on P0.05 (A0), 1.73:1 ratio.
#define PIN_VBAT_READ (PIN_A0)
#define BATTERY_PIN PIN_VBAT_READ
#define ADC_MULTIPLIER (1.73)

// 3V3_S switched peripheral rail AND the RAK13302's 5 V boost that
// feeds the SKY66122 PA. Must stay HIGH for the whole radio session —
// do NOT toggle this for power saving.
#define PIN_3V3_EN (34)
#define WB_IO2_3V3_EN PIN_3V3_EN

// SKY66122-11 FEM enable. CSD and CPS are tied together on the
// RAK13302 PCB and routed to WisBlock IO3 (P0.21). HIGH = FEM active
// (LNA on RX, PA path available for TX). CTX is wired to SX1262 DIO2,
// so TX/RX switching is hardware-timed by the radio itself.
#define SX126X_POWER_EN (21)

#define USE_SX1262
#define SX126X_CS (26)
#define SX126X_DIO1 (10)
#define SX126X_BUSY (9)
#define SX126X_RESET (4)
#define SX126X_DIO2_AS_RF_SWITCH
#define SX126X_DIO3_TCXO_VOLTAGE 1.8

// RAK1910 GNSS on the WisBlock base (optional add-on module).
#define PIN_GPS_PPS (17)
#define PIN_GPS_RX PIN_SERIAL1_RX
#define PIN_GPS_TX PIN_SERIAL1_TX

#ifdef __cplusplus
}
#endif

#endif
