#include "rak4631_config_store.h"

#include <string.h>

namespace Rak4631ConfigStore {
namespace {

constexpr uint8_t MAGIC[4] = {'O', 'H', 'C', 'S'};
constexpr uint16_t STORE_SCHEMA = 1;
constexpr size_t HEADER_SIZE = 12;

void write16(uint8_t* output, uint16_t value) {
    output[0] = static_cast<uint8_t>(value);
    output[1] = static_cast<uint8_t>(value >> 8);
}

void write32(uint8_t* output, uint32_t value) {
    output[0] = static_cast<uint8_t>(value);
    output[1] = static_cast<uint8_t>(value >> 8);
    output[2] = static_cast<uint8_t>(value >> 16);
    output[3] = static_cast<uint8_t>(value >> 24);
}

uint16_t read16(const uint8_t* input) {
    return static_cast<uint16_t>(input[0]) |
           static_cast<uint16_t>(static_cast<uint16_t>(input[1]) << 8);
}

uint32_t read32(const uint8_t* input) {
    return static_cast<uint32_t>(input[0]) |
           (static_cast<uint32_t>(input[1]) << 8) |
           (static_cast<uint32_t>(input[2]) << 16) |
           (static_cast<uint32_t>(input[3]) << 24);
}

uint32_t crc32(const uint8_t* data, size_t length) {
    uint32_t crc = 0xffffffffu;
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

}  // namespace

Store::Store(Flash& flash) : flash_(flash) {}

bool Store::newer(uint32_t candidate, uint32_t reference) {
    const uint32_t distance = candidate - reference;
    return distance != 0 && distance < 0x80000000u;
}

Store::Slot Store::readSlot(uint32_t address) {
    Slot slot{};
    uint8_t bytes[SLOT_WIRE_SIZE];
    slot.ioOk = flash_.read(address, bytes, sizeof(bytes));
    if (!slot.ioOk || memcmp(bytes, MAGIC, sizeof(MAGIC)) != 0 ||
        read16(bytes + 4) != STORE_SCHEMA) {
        return slot;
    }

    const uint16_t recordLength = read16(bytes + 6);
    if (recordLength > Rak4631Config::MAX_ENCODED_SIZE ||
        HEADER_SIZE + recordLength + sizeof(uint32_t) > sizeof(bytes)) {
        return slot;
    }

    const uint32_t storedCrc = read32(bytes + HEADER_SIZE + recordLength);
    if (storedCrc != crc32(bytes, HEADER_SIZE + recordLength)) return slot;

    slot.valid = true;
    slot.generation = read32(bytes + 8);
    if (recordLength == 0) {
        slot.tombstone = true;
        return slot;
    }

    Rak4631Config::Config decoded{};
    if (Rak4631Config::decode(bytes + HEADER_SIZE, recordLength, decoded) !=
        Rak4631Config::DecodeStatus::OK) {
        return slot;
    }

    slot.config = decoded;
    return slot;
}

__attribute__((noinline)) LoadResult Store::load(Rak4631Config::Config& config) {
    const Slot a = readSlot(SLOT_A_ADDRESS);
    const Slot b = readSlot(SLOT_B_ADDRESS);
    scanned_ = true;

    if (!a.valid && !b.valid) {
        hasActive_ = false;
        activeGeneration_ = 0;
        return (!a.ioOk && !b.ioOk) ? LoadResult::IO_ERROR : LoadResult::EMPTY;
    }

    const bool useB = b.valid && (!a.valid || newer(b.generation, a.generation));
    const Slot& selected = useB ? b : a;
    activeAddress_ = useB ? SLOT_B_ADDRESS : SLOT_A_ADDRESS;
    activeGeneration_ = selected.generation;
    hasActive_ = true;
    if (selected.tombstone) return LoadResult::EMPTY;
    config = selected.config;
    return LoadResult::OK;
}

bool Store::writeSlot(uint32_t address, uint32_t generation,
                      const Rak4631Config::Config* config) {
    uint8_t bytes[SLOT_WIRE_SIZE];
    memset(bytes, 0xff, sizeof(bytes));
    memcpy(bytes, MAGIC, sizeof(MAGIC));
    write16(bytes + 4, STORE_SCHEMA);
    write32(bytes + 8, generation);

    size_t recordLength = 0;
    if (config != nullptr) {
        if (!Rak4631Config::encode(*config, bytes + HEADER_SIZE,
                                   Rak4631Config::MAX_ENCODED_SIZE, recordLength) ||
            recordLength > UINT16_MAX) {
            return false;
        }
    }
    write16(bytes + 6, static_cast<uint16_t>(recordLength));
    write32(bytes + HEADER_SIZE + recordLength,
            crc32(bytes, HEADER_SIZE + recordLength));

    if (!flash_.erasePage(address) || !flash_.write(address, bytes, sizeof(bytes)))
        return false;

    uint8_t readback[SLOT_WIRE_SIZE];
    if (!flash_.read(address, readback, sizeof(readback)) ||
        memcmp(bytes, readback, sizeof(bytes)) != 0) {
        return false;
    }

    const Slot verified = readSlot(address);
    return verified.valid && verified.generation == generation &&
           ((config == nullptr && verified.tombstone) ||
            (config != nullptr && !verified.tombstone && verified.config == *config));
}

bool Store::save(const Rak4631Config::Config& config) {
    if (!scanned_) {
        Rak4631Config::Config ignored{};
        if (load(ignored) == LoadResult::IO_ERROR) return false;
    }

    const uint32_t target =
        hasActive_ && activeAddress_ == SLOT_A_ADDRESS ? SLOT_B_ADDRESS : SLOT_A_ADDRESS;
    const uint32_t generation = hasActive_ ? activeGeneration_ + 1u : 1u;
    if (!writeSlot(target, generation, &config)) return false;

    activeAddress_ = target;
    activeGeneration_ = generation;
    hasActive_ = true;
    return true;
}

bool Store::erase() {
    if (!scanned_) {
        Rak4631Config::Config ignored{};
        if (load(ignored) == LoadResult::IO_ERROR) return false;
    }

    const uint32_t target =
        hasActive_ && activeAddress_ == SLOT_A_ADDRESS ? SLOT_B_ADDRESS : SLOT_A_ADDRESS;
    const uint32_t generation = hasActive_ ? activeGeneration_ + 1u : 1u;
    if (!writeSlot(target, generation, nullptr)) return false;
    activeAddress_ = target;
    activeGeneration_ = generation;
    hasActive_ = true;
    return true;
}

}  // namespace Rak4631ConfigStore
