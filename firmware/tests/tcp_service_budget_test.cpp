#include "bounded_protocol_service.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <initializer_list>

struct FakeClient {
    std::deque<uint8_t> bytes;
    bool is_connected = true;
    size_t connected_calls = 0;
    size_t available_calls = 0;

    FakeClient(std::initializer_list<uint8_t> initial) : bytes(initial) {}

    bool connected() {
        ++connected_calls;
        return is_connected;
    }

    int available() {
        ++available_calls;
        return static_cast<int>(bytes.size());
    }

    int read() {
        if (bytes.empty()) return -1;
        const uint8_t value = bytes.front();
        bytes.pop_front();
        return value;
    }
};

static FakeClient clientWithBytes(size_t count) {
    FakeClient client({});
    for (size_t i = 0; i < count; ++i) {
        client.bytes.push_back(static_cast<uint8_t>(i));
    }
    return client;
}

static void testStopsAfterOneCompleteFrame() {
    FakeClient client = clientWithBytes(18);
    size_t parser_bytes = 0;

    const auto result = serviceBoundedProtocolInput(
        client, 256, 1,
        [&parser_bytes](uint8_t) {
            ++parser_bytes;
            return parser_bytes % 6 == 0;
        });

    assert(result.bytes_processed == 6);
    assert(result.frames_processed == 1);
    assert(client.bytes.size() == 12);
}

static void testStopsAtByteBudgetWithoutAFrame() {
    FakeClient client = clientWithBytes(300);

    const auto result = serviceBoundedProtocolInput(
        client, 256, 1, [](uint8_t) { return false; });

    assert(result.bytes_processed == 256);
    assert(result.frames_processed == 0);
    assert(client.bytes.size() == 44);
}

static void testLargeFrameContinuesAcrossLoopPasses() {
    FakeClient client = clientWithBytes(267);
    size_t parser_bytes = 0;
    auto feed = [&parser_bytes](uint8_t) {
        ++parser_bytes;
        return parser_bytes == 267;
    };

    const auto first = serviceBoundedProtocolInput(client, 256, 1, feed);
    assert(first.bytes_processed == 256);
    assert(first.frames_processed == 0);
    assert(client.bytes.size() == 11);

    const auto second = serviceBoundedProtocolInput(client, 256, 1, feed);
    assert(second.bytes_processed == 11);
    assert(second.frames_processed == 1);
    assert(client.bytes.empty());
}

static void testDisconnectDuringCallbackStopsThePass() {
    FakeClient client = clientWithBytes(20);
    size_t parser_bytes = 0;

    const auto result = serviceBoundedProtocolInput(
        client, 256, 2,
        [&client, &parser_bytes](uint8_t) {
            ++parser_bytes;
            if (parser_bytes == 6) {
                client.is_connected = false;
                return true;
            }
            return false;
        });

    assert(result.bytes_processed == 6);
    assert(result.frames_processed == 1);
    assert(client.bytes.size() == 14);
}

static void testZeroBudgetDoesNotTouchTheClient() {
    FakeClient client = clientWithBytes(10);

    const auto result = serviceBoundedProtocolInput(
        client, 0, 1, [](uint8_t) { return false; });

    assert(result.bytes_processed == 0);
    assert(result.frames_processed == 0);
    assert(client.connected_calls == 0);
    assert(client.available_calls == 0);
    assert(client.bytes.size() == 10);
}

int main() {
    testStopsAfterOneCompleteFrame();
    testStopsAtByteBudgetWithoutAFrame();
    testLargeFrameContinuesAcrossLoopPasses();
    testDisconnectDuringCallbackStopsThePass();
    testZeroBudgetDoesNotTouchTheClient();
    return 0;
}
