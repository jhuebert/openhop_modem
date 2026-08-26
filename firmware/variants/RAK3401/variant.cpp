// =============================================================
// variants/RAK3401/variant.cpp — identity pin map for the
// RAKwireless RAK3401 (nRF52840 + RAK13302 SX1262/SKY66122).
//
// Arduino pin N maps to raw nRF52 GPIO N (P1.x == 32 + x).
// P0.00 / P0.01 carry the 32.768 kHz crystal and are unmapped
// (0xFF) so digitalWrite() silently no-ops on them.
// =============================================================
#include "variant.h"
#include "wiring_constants.h"
#include "wiring_digital.h"
#include "nrf.h"

const uint32_t g_ADigitalPinMap[] = {
  0xff, 0xff, 2 , 3 , 4 , 5 , 6 , 7,
  8   , 9   , 10, 11, 12, 13, 14, 15,
  16  , 17  , 18, 19, 20, 21, 22, 23,
  24  , 25  , 26, 27, 28, 29, 30, 31,
  32  , 33  , 34, 35, 36, 37, 38, 39,
  40  , 41  , 42, 43, 44, 45, 46, 47
};

void initVariant() {
  pinMode(PIN_LED1, OUTPUT);
  ledOff(PIN_LED1);
  pinMode(PIN_LED2, OUTPUT);
  ledOff(PIN_LED2);

  // Bring up the 3V3_S rail and the RAK13302's 5 V PA boost as early as
  // possible so both are settled before configureStaticGpios() enables
  // the SKY66122 front end.
  pinMode(PIN_3V3_EN, OUTPUT);
  digitalWrite(PIN_3V3_EN, HIGH);
}
