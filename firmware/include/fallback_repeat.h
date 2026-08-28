// =============================================================
// fallback_repeat.h — host-loss fallback flood-repeat mode
// ("repeater down" survival mode) for openHop Modem firmware.
//
// While the host repeater (openHop Core) is alive, this module is
// inert: it engages only after no valid host frame has been seen on
// any transport for FALLBACK_REPEAT_ENGAGE_TIMEOUT_SEC. Repeats are
// flood-routed MeshCore packets only, deduplicated by a payload-hash
// seen-table, retransmitted after a MeshCore-style random delay via
// the firmware's shared TX path (reception guard + auto-CAD).
//
// This is deliberately NOT a mini repeater: no identity, no ACKs,
// no adverts, no policy engine, no storage.
// =============================================================
#pragma once

#include <stdint.h>

namespace FallbackRepeat {

// Type of the transmit hook registered by main.cpp at begin(). The
// hook must run the firmware's shared TX path (reception guard,
// auto-CAD, busy refusal) and returns true when the frame went on air.
typedef bool (*TxFn)(const uint8_t* data, uint8_t len);

// Time-on-air hook (milliseconds) used for the MeshCore retransmit delay.
typedef uint32_t (*AirtimeMsFn)(uint8_t len);

// Build-time host-silence window before the fallback engages. 120 s
// comfortably exceeds the host's 30 s noise-floor poll cadence, and
// the window is intentionally not a runtime setting.
#ifndef FALLBACK_REPEAT_ENGAGE_TIMEOUT_SEC
#define FALLBACK_REPEAT_ENGAGE_TIMEOUT_SEC 120
#endif

// Register hooks + initial enable state. Persistence is the caller's
// job (WifiManager / NodeState / Rak4631Config).
void begin(bool enabled, TxFn tx, AirtimeMsFn airtimeMs);

void setEnabled(bool enabled);
bool enabled();

// Engaged = enabled && host silent && radio up && not in standby.
bool active();

// Called from processHostCommand() entry: any valid host frame
// disengages the fallback and clears any pending repeats.
void noteHostFrame();

void onRadioStandbyChanged(bool standby);

// Call from handleLoRaRx() with the raw LoRa bytes (no RSSI header).
// Returns true when the packet was accepted into the repeat queue.
bool onRxPacket(const uint8_t* data, uint8_t len);

// Call every loop() iteration; transmits due repeats via the TX hook.
void loop(bool radioReady, bool standby, bool txActive);

// Serial-log counters (bench debugging only; not exposed to UI/API).
uint32_t repeatsSent();
uint32_t repeatsDropped();

}  // namespace FallbackRepeat
