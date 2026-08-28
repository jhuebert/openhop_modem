// =============================================================
// fallback_repeat.cpp — see header.
//
// Packet handling mirrors MeshCore's Mesh::routeRecvPacket /
// Mesh::onRecvPacket flood behavior:
//   - only ROUTE_TYPE_TRANSPORT_FLOOD (0x00) / ROUTE_TYPE_FLOOD (0x01);
//   - header == 0xFF ("do not retransmit") is skipped;
//   - payload types 0x00–0x08 only — MeshCore flood-routes exactly those
//     (REQ, RESPONSE, TXT_MSG, ACK, ADVERT, GRP_TXT, GRP_DATA, ANON_REQ,
//     PATH) and never repeats unknown/control/raw-custom types;
//   - one own path hash appended, path count bounded by MAX_PATH_SIZE;
//   - duplicate suppression on a payload hash (type + payload, path
//     ignored, so copies arriving via different repeaters collapse);
//   - retransmit delay = (airtime × 52/50)/2 × random(0..5) ms, the
//     stock Mesh::getRetransmitDelay scheme.
// =============================================================

#include "fallback_repeat.h"

#include <Arduino.h>
#include <string.h>

#include "compat.h"

namespace FallbackRepeat {
namespace {

constexpr uint8_t  SEEN_TABLE_SIZE  = 128;   // × 4 B payload hashes
constexpr uint8_t  MAX_PATH_SIZE    = 64;    // MeshCore flood path bound
constexpr uint8_t  QUEUE_SLOTS      = 2;     // like the repeater dispatcher
constexpr uint8_t  MAX_BUSY_RETRIES = 8;     // bound, unlike MeshCore, so a
                                             // wedged radio cannot hold a slot forever

struct RepeatSlot {
    bool     used;
    uint8_t  len;
    uint8_t  retries;
    uint32_t dueMs;
    uint8_t  pkt[255];   // MAX_LORA_PAYLOAD
};

RepeatSlot slots[QUEUE_SLOTS];
uint32_t  seen[SEEN_TABLE_SIZE];
uint8_t   seenNext = 0;

bool     enabledFlag       = false;
bool     standbyFlag       = false;
bool     radioReadyFlag    = false;
bool     wasActive         = false;
uint32_t lastHostFrameMs   = 0;
uint32_t sentCount         = 0;
uint32_t droppedCount      = 0;
uint8_t  pathHash[4]       = {0};
TxFn        txFn        = nullptr;
AirtimeMsFn airtimeMsFn = nullptr;

// ─── CRC-32 (same polynomial/shape as rak4631_config.cpp) ───
uint32_t crc32Raw(uint32_t crc, const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            const uint32_t mask = 0U - (crc & 1U);
            crc = (crc >> 1) ^ (0xedb88320U & mask);
        }
    }
    return crc;
}

// Payload hash over (payload type | payload), deliberately ignoring
// the path so copies repeated by different repeaters hash identically.
uint32_t payloadHash(uint8_t payloadType, const uint8_t* payload, uint8_t len) {
    uint8_t t = payloadType;
    uint32_t crc = crc32Raw(0xffffffffU, &t, 1);
    return ~crc32Raw(crc, payload, len);
}

bool wasSeen(uint32_t hash) {
    for (uint8_t i = 0; i < SEEN_TABLE_SIZE; ++i) {
        if (seen[i] == hash) return true;
    }
    return false;
}

void markSeen(uint32_t hash) {
    seen[seenNext] = hash;
    seenNext = (seenNext + 1) % SEEN_TABLE_SIZE;
}

// Arduino random() is unseeded and identical on the nRF52 targets;
// micros() jitter is this firmware's usual entropy source (see the
// auto-CAD backoff in main.cpp).
uint32_t jitter(uint32_t bound) {
    return micros() % bound;
}

// Deterministic per-device path hash: low bytes of a fixed digest of the
// permanent MAC. Stable across reboots, never random per packet.
void initPathHash() {
    uint8_t mac[6];
    compatGetMac(mac);
    uint32_t x = ~crc32Raw(0xffffffffU, mac, sizeof(mac));
    for (uint8_t i = 0; i < 4; ++i) {
        pathHash[i] = (uint8_t)(x >> (8 * i));
    }
}

// MeshCore wire format: header(1) [transport codes(4) when route is
// TRANSPORT_*] path_len(1) path(count × hash_size) payload(rest).
struct FloodPacket {
    uint8_t  payloadType;
    const uint8_t* payload;
    uint8_t  payloadLen;
    uint8_t  pathOffset;   // start of path bytes within the raw packet
    uint8_t  hashSize;     // bytes per path hash entry (1..4)
    uint8_t  hashCount;    // entries already in the path
};

bool parseFloodPacket(const uint8_t* data, uint8_t len, FloodPacket& out) {
    if (len < 3) return false;
    const uint8_t header = data[0];
    if (header == 0xFF) return false;                 // do-not-retransmit
    const uint8_t route = header & 0x03;
    if (route != 0x00 && route != 0x01) return false; // floods only
    out.payloadType = (header >> 2) & 0x0F;
    if (out.payloadType > 0x08) return false;         // MeshCore's flood whitelist

    uint8_t idx = 1;
    if (route == 0x00) idx += 4;                      // transport codes
    if (idx >= len) return false;
    const uint8_t pathLenByte = data[idx++];
    out.hashSize  = (pathLenByte >> 6) + 1;
    out.hashCount = pathLenByte & 0x3F;
    out.pathOffset = idx;
    const uint8_t pathBytes = out.hashCount * out.hashSize;
    if ((uint16_t)idx + pathBytes >= len) return false;  // empty payload = bad encoding
    out.payload = &data[idx + pathBytes];
    out.payloadLen = len - idx - pathBytes;
    return true;
}

void clearQueue() {
    for (uint8_t i = 0; i < QUEUE_SLOTS; ++i) slots[i].used = false;
}

bool engaged() {
    return enabledFlag && radioReadyFlag && !standbyFlag &&
           (millis() - lastHostFrameMs >
            (uint32_t)FALLBACK_REPEAT_ENGAGE_TIMEOUT_SEC * 1000U);
}

}  // namespace

void begin(bool enabled, TxFn tx, AirtimeMsFn airtimeMs) {
    enabledFlag = enabled;
    txFn = tx;
    airtimeMsFn = airtimeMs;
    lastHostFrameMs = 0;
    standbyFlag = false;
    radioReadyFlag = false;
    wasActive = false;
    clearQueue();
    memset(seen, 0xFF, sizeof(seen));   // 0xFFFFFFFF is the CRC-32 "empty" value
    seenNext = 0;
    initPathHash();
    Serial.printf("[FBREP] %s (engage window %us)\n",
                  enabled ? "enabled" : "disabled",
                  (unsigned)FALLBACK_REPEAT_ENGAGE_TIMEOUT_SEC);
}

void setEnabled(bool enabled) {
    if (enabledFlag == enabled) return;
    enabledFlag = enabled;
    clearQueue();
    Serial.printf("[FBREP] %s\n", enabled ? "enabled" : "disabled");
}

bool enabled() {
    return enabledFlag;
}

bool active() {
    return engaged();
}

void noteHostFrame() {
    if (wasActive) {
        Serial.println("[FBREP] host frame seen — disengaging");
    }
    lastHostFrameMs = millis();
    clearQueue();
}

void onRadioStandbyChanged(bool standby) {
    standbyFlag = standby;
    if (standby) clearQueue();   // radio was shut down deliberately
}

bool onRxPacket(const uint8_t* data, uint8_t len) {
    if (!engaged()) return false;

    FloodPacket pkt;
    if (!parseFloodPacket(data, len, pkt)) return false;

    const uint32_t hash = payloadHash(pkt.payloadType, pkt.payload, pkt.payloadLen);
    if (wasSeen(hash)) return false;   // duplicate (incl. our own echo)

    // Path capacity gate, same as Mesh::routeRecvPacket: (count+1) hashes
    // must fit MAX_PATH_SIZE, and the extended frame must fit the radio.
    const uint8_t hashSize = pkt.hashSize;
    if ((uint8_t)(pkt.hashCount + 1) * hashSize > MAX_PATH_SIZE) return false;
    const uint8_t newLen = len + hashSize;
    if (newLen < len) return false;    // would wrap past 255
    const uint8_t payloadOffset = pkt.pathOffset + (uint8_t)(pkt.hashCount * hashSize);

    int8_t slotIdx = -1;
    for (uint8_t i = 0; i < QUEUE_SLOTS; ++i) {
        if (!slots[i].used) { slotIdx = i; break; }
    }
    if (slotIdx < 0) {
        droppedCount++;   // dispatcher pool exhausted, same as the repeater
        return false;
    }

    // Build the extended frame: path prefix + our hash + payload.
    RepeatSlot& slot = slots[slotIdx];
    memcpy(slot.pkt, data, payloadOffset);
    memcpy(&slot.pkt[payloadOffset], pathHash, hashSize);
    memcpy(&slot.pkt[payloadOffset + hashSize], pkt.payload, pkt.payloadLen);
    slot.pkt[pkt.pathOffset - 1] = ((hashSize - 1) << 6) | (uint8_t)(pkt.hashCount + 1);
    slot.len = newLen;
    slot.retries = 0;

    const uint32_t airtime = airtimeMsFn ? airtimeMsFn(newLen) : 20;
    const uint32_t delay = ((airtime * 52) / 50) / 2 * jitter(6);
    slot.dueMs = millis() + delay;
    slot.used = true;

    markSeen(hash);   // marked before TX, so our own echo is dropped
    return true;
}

void loop(bool radioReady, bool standby, bool txActive) {
    radioReadyFlag = radioReady;
    standbyFlag = standby;

    const bool now = engaged();
    if (now != wasActive) {
        Serial.printf("[FBREP] %s\n",
                      now ? "engaged (host silent) — repeating flood traffic"
                          : "disengaged — host is alive");
        clearQueue();
        wasActive = now;
    }
    if (!now || txActive) return;

    const uint32_t nowMs = millis();
    for (uint8_t i = 0; i < QUEUE_SLOTS; ++i) {
        RepeatSlot& slot = slots[i];
        if (!slot.used || (int32_t)(nowMs - slot.dueMs) < 0) continue;

        if (!txFn) { slot.used = false; continue; }
        if (txFn(slot.pkt, slot.len)) {
            slot.used = false;
            sentCount++;
            Serial.printf("[FBREP] repeated %u bytes (%lu total)\n",
                          (unsigned)slot.len, (unsigned long)sentCount);
            continue;
        }
        // Channel busy (LBT) or radio-side failure: back off and retry,
        // bounded so a persistently wedged radio cannot hold the slot.
        if (++slot.retries > MAX_BUSY_RETRIES) {
            slot.used = false;
            droppedCount++;
            Serial.println("[FBREP] giving up on packet after retries");
            continue;
        }
        slot.dueMs = nowMs + (1 + jitter(4)) * 120;
    }
}

uint32_t repeatsSent()    { return sentCount; }
uint32_t repeatsDropped() { return droppedCount; }

}  // namespace FallbackRepeat
