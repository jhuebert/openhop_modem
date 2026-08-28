// =============================================================
// node_state.h — non-volatile per-node state on T114 (nRF52840).
//
// Persists between reboots so the modem boots into the same state
// the controller last assigned, even if the controller takes
// seconds to come up and re-push.
//
// Two fields:
//   - displayName: ASCII name shown big on the TFT
//   - radioStandby: hard standby flag (skip startReceive on boot)
//
// Backed by Adafruit_LittleFS on the internal flash. The controller
// still pushes both fields on boot / OFFLINE→ONLINE — this just
// gives the modem something sensible to show during the gap.
// =============================================================
#pragma once

#if defined(BOARD_HELTEC_T114)

#include <stdint.h>
#include <stddef.h>

namespace NodeState {

void begin();                                   // load from LittleFS at boot

bool        getStandby();
const char* getDisplayName();                   // returns "" if never set
bool        getAutoCad();
bool        getFallbackRepeat();

void setDisplayName(const char* name);          // saves to LittleFS
void setStandby(bool on);                       // saves to LittleFS
void setAutoCad(bool on);                       // saves to LittleFS
void setFallbackRepeat(bool on);                // saves to LittleFS

// Last host-pushed radio config (CMD_SET_CONFIG payload, sizeof bytes).
// Restored at boot so the fallback repeater survives a modem power
// cycle without re-deriving the radio preset. Returns false when none
// was ever persisted.
bool loadRadioConfig(uint8_t* out, size_t len);
void saveRadioConfig(const uint8_t* data, size_t len);

}   // namespace NodeState

#endif   // BOARD_HELTEC_T114
