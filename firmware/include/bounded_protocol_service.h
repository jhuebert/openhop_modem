#pragma once

#include <stddef.h>
#include <stdint.h>

struct BoundedProtocolServiceResult {
    size_t bytes_processed = 0;
    uint8_t frames_processed = 0;
};

template <typename Client, typename FeedByte>
BoundedProtocolServiceResult serviceBoundedProtocolInput(
    Client& client,
    size_t max_bytes,
    uint8_t max_frames,
    FeedByte feed_byte) {
    BoundedProtocolServiceResult result;

    // Check the local budgets before touching the client. This keeps the
    // fairness guarantee independent of the network implementation's
    // connected()/available() behavior once a pass has exhausted its quota.
    while (result.bytes_processed < max_bytes &&
           result.frames_processed < max_frames &&
           client.connected() && client.available()) {
        const int value = client.read();
        if (value < 0) break;

        ++result.bytes_processed;
        if (feed_byte(static_cast<uint8_t>(value))) {
            ++result.frames_processed;
        }
    }

    return result;
}
