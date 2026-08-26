#pragma once

#include "rak4631_config.h"

#include <stddef.h>
#include <stdint.h>

namespace Rak4631ConfigStore {

constexpr uint32_t CONFIG_REGION_START = 0xED000u;
constexpr uint32_t SLOT_A_ADDRESS = CONFIG_REGION_START;
constexpr uint32_t SLOT_B_ADDRESS = CONFIG_REGION_START + 0x1000u;
constexpr uint32_t CONFIG_REGION_END = 0xF4000u;
constexpr uint32_t PAGE_SIZE = 0x1000u;
constexpr size_t SLOT_WIRE_SIZE = 272u;

class Flash {
public:
    virtual ~Flash() = default;
    virtual bool erasePage(uint32_t address) = 0;
    virtual bool write(uint32_t address, const uint8_t* data, size_t length) = 0;
    virtual bool read(uint32_t address, uint8_t* data, size_t length) = 0;
};

enum class LoadResult : uint8_t {
    OK = 0,
    EMPTY,
    IO_ERROR,
};

class Store {
public:
    explicit Store(Flash& flash);

    LoadResult load(Rak4631Config::Config& config);
    bool save(const Rak4631Config::Config& config);
    bool erase();

private:
    struct Slot {
        bool ioOk = false;
        bool valid = false;
        bool tombstone = false;
        uint32_t generation = 0;
        Rak4631Config::Config config{};
    };

    Slot readSlot(uint32_t address);
    bool writeSlot(uint32_t address, uint32_t generation,
                   const Rak4631Config::Config* config);
    static bool newer(uint32_t candidate, uint32_t reference);

    Flash& flash_;
    bool scanned_ = false;
    bool hasActive_ = false;
    uint32_t activeAddress_ = SLOT_A_ADDRESS;
    uint32_t activeGeneration_ = 0;
};

}  // namespace Rak4631ConfigStore
