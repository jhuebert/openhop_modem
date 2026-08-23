#include "w5100s_bounded_rx.h"

#if defined(OPENHOP_ETHERNET_W5100S)

#include <Arduino.h>
#include <RAK13800_W5100S.h>
#include <w5100.h>

namespace W5100sBoundedRx {
namespace {

constexpr uint8_t INVALID_SOCKET = 0xff;
constexpr uint8_t STABLE_READ_ATTEMPTS = 4;
constexpr uint16_t ACK_BATCH_BYTES = 64;
constexpr uint32_t COMMAND_DEADLINE_MS = 100;

bool socketStatusConnected(uint8_t status) {
    return status == SnSR::ESTABLISHED || status == SnSR::CLOSE_WAIT;
}

}  // namespace

void Reader::reset(uint8_t socket) {
    socket_ = socket;
    pointer_ = 0;
    remaining_ = 0;
    unacknowledged_ = 0;
    commandPending_ = false;
    commandStartedMs_ = 0;
}

void Reader::clear() {
    reset(INVALID_SOCKET);
}

bool Reader::serviceCommand(uint32_t nowMs, bool& stalled) {
    stalled = false;
    if (!commandPending_) return true;
    if (socket_ >= MAX_SOCK_NUM) {
        stalled = true;
        return false;
    }

    W5100.getSPI()->beginTransaction(SPI_ETHERNET_SETTINGS);
    const uint8_t command = W5100.readSnCR(socket_);
    W5100.getSPI()->endTransaction();
    if (command == 0) {
        commandPending_ = false;
        return true;
    }
    if (static_cast<uint32_t>(nowMs - commandStartedMs_) >= COMMAND_DEADLINE_MS)
        stalled = true;
    return false;
}

void Reader::beginReceiveCommand(uint32_t nowMs) {
    W5100.writeSnRX_RD(socket_, pointer_);
    W5100.writeSnCR(socket_, Sock_RECV);
    unacknowledged_ = 0;
    commandPending_ = true;
    commandStartedMs_ = nowMs;
}

bool Reader::settled(uint32_t nowMs, bool& stalled) {
    if (!serviceCommand(nowMs, stalled)) return false;
    if (unacknowledged_ == 0) return true;

    W5100.getSPI()->beginTransaction(SPI_ETHERNET_SETTINGS);
    beginReceiveCommand(nowMs);
    W5100.getSPI()->endTransaction();
    return false;
}

bool Reader::connected() const {
    if (socket_ >= MAX_SOCK_NUM) return false;
    W5100.getSPI()->beginTransaction(SPI_ETHERNET_SETTINGS);
    const uint8_t status = W5100.readSnSR(socket_);
    W5100.getSPI()->endTransaction();
    return socketStatusConnected(status);
}

PollResult Reader::poll(uint32_t nowMs, uint8_t& byte) {
    bool stalled = false;
    if (!serviceCommand(nowMs, stalled))
        return stalled ? PollResult::STALLED : PollResult::NONE;
    if (socket_ >= MAX_SOCK_NUM) return PollResult::CLOSED;

    W5100.getSPI()->beginTransaction(SPI_ETHERNET_SETTINGS);
    if (remaining_ == 0) {
        uint16_t previous = W5100.readSnRX_RSR(socket_);
        bool stable = false;
        for (uint8_t attempt = 0; attempt < STABLE_READ_ATTEMPTS; ++attempt) {
            const uint16_t current = W5100.readSnRX_RSR(socket_);
            if (current == previous) {
                stable = true;
                break;
            }
            previous = current;
        }
        if (!stable) {
            W5100.getSPI()->endTransaction();
            return PollResult::NONE;
        }
        if (previous > W5100.SSIZE) {
            W5100.getSPI()->endTransaction();
            return PollResult::STALLED;
        }
        if (previous == 0) {
            const uint8_t status = W5100.readSnSR(socket_);
            W5100.getSPI()->endTransaction();
            return socketStatusConnected(status) ? PollResult::NONE : PollResult::CLOSED;
        }
        pointer_ = W5100.readSnRX_RD(socket_);
        remaining_ = previous;
    }

    const uint16_t offset = pointer_ & W5100.SMASK;
    W5100.read(W5100.RBASE(socket_) + offset, &byte, 1);
    ++pointer_;
    --remaining_;
    ++unacknowledged_;
    if (unacknowledged_ >= ACK_BATCH_BYTES || remaining_ == 0)
        beginReceiveCommand(nowMs);
    W5100.getSPI()->endTransaction();
    return PollResult::BYTE;
}

}  // namespace W5100sBoundedRx

#endif
