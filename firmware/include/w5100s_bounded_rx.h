#pragma once

#include <stdint.h>

#if defined(OPENHOP_ETHERNET_W5100S)

namespace W5100sBoundedRx {

enum class PollResult : uint8_t {
    NONE,
    BYTE,
    CLOSED,
    STALLED,
};

class Reader {
public:
    void reset(uint8_t socket);
    void clear();
    PollResult poll(uint32_t nowMs, uint8_t& byte);
    bool settled(uint32_t nowMs, bool& stalled);
    bool connected() const;

private:
    bool serviceCommand(uint32_t nowMs, bool& stalled);
    void beginReceiveCommand(uint32_t nowMs);

    uint8_t socket_ = 0xff;
    uint16_t pointer_ = 0;
    uint16_t remaining_ = 0;
    uint16_t unacknowledged_ = 0;
    bool commandPending_ = false;
    uint32_t commandStartedMs_ = 0;
};

}  // namespace W5100sBoundedRx

#endif
