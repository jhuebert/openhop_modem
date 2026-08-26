#include "rak4631_config_store.h"

#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

using namespace Rak4631Config;
using namespace Rak4631ConfigStore;

namespace {

class FakeFlash final : public Flash {
public:
    FakeFlash() {
        a.fill(0xff);
        b.fill(0xff);
    }

    bool erasePage(uint32_t address) override {
        operations.push_back(address);
        auto* page = pageFor(address);
        if (!page || failErase) return false;
        page->fill(0xff);
        return true;
    }

    bool write(uint32_t address, const uint8_t* data, size_t length) override {
        operations.push_back(address);
        auto* page = pageFor(address);
        if (!page || !data || length > page->size()) return false;
        const size_t actual = length < writeLimit ? length : writeLimit;
        for (size_t i = 0; i < actual; ++i) (*page)[i] &= data[i];
        return actual == length;
    }

    bool read(uint32_t address, uint8_t* data, size_t length) override {
        const uint32_t base = address >= SLOT_B_ADDRESS ? SLOT_B_ADDRESS : SLOT_A_ADDRESS;
        auto* page = pageFor(base);
        if (!page || address < base || length > page->size() - (address - base))
            return false;
        memcpy(data, page->data() + (address - base), length);
        return true;
    }

    std::array<uint8_t, PAGE_SIZE> a{};
    std::array<uint8_t, PAGE_SIZE> b{};
    std::vector<uint32_t> operations;
    size_t writeLimit = std::numeric_limits<size_t>::max();
    bool failErase = false;

private:
    std::array<uint8_t, PAGE_SIZE>* pageFor(uint32_t address) {
        if (address == SLOT_A_ADDRESS) return &a;
        if (address == SLOT_B_ADDRESS) return &b;
        return nullptr;
    }
};

Config configNamed(const char* name, uint8_t lastOctet) {
    Config config = makeDefaults(name, 5055, "");
    config.useStaticIP = true;
    config.staticIP = IPv4Address{{192, 168, 0, lastOctet}};
    config.subnet = IPv4Address{{255, 255, 255, 0}};
    config.gateway = IPv4Address{{192, 168, 0, 1}};
    assert(validateAndNormalize(config) == ValidationStatus::OK);
    return config;
}

void testLayoutUsesOnlyBootloaderPreservedConfigPages() {
    static_assert(SLOT_A_ADDRESS == 0xED000u, "slot A moved");
    static_assert(SLOT_B_ADDRESS == 0xEE000u, "slot B moved");
    static_assert(SLOT_B_ADDRESS + PAGE_SIZE <= CONFIG_REGION_END,
                  "config slots overlap bootloader");
    static_assert(SLOT_WIRE_SIZE <= PAGE_SIZE, "slot record exceeds erase page");
}

void testEmptySaveAndReload() {
    FakeFlash flash;
    Store store(flash);
    Config loaded{};
    assert(store.load(loaded) == LoadResult::EMPTY);

    const Config first = configNamed("rack-one", 28);
    assert(store.save(first));

    Store rebooted(flash);
    assert(rebooted.load(loaded) == LoadResult::OK);
    assert(loaded == first);
    assert(flash.operations.size() == 2);
    assert(flash.operations[0] == SLOT_A_ADDRESS);
    assert(flash.operations[1] == SLOT_A_ADDRESS);
}

void testNewestSlotWinsAndCorruptionFallsBack() {
    FakeFlash flash;
    Store store(flash);
    const Config first = configNamed("rack-one", 28);
    const Config second = configNamed("rack-two", 29);
    assert(store.save(first));
    assert(store.save(second));

    Config loaded{};
    Store rebooted(flash);
    assert(rebooted.load(loaded) == LoadResult::OK);
    assert(loaded == second);

    flash.b[20] ^= 0x01;
    Store afterCorruption(flash);
    assert(afterCorruption.load(loaded) == LoadResult::OK);
    assert(loaded == first);
}

void testInterruptedWritePreservesPreviousSlot() {
    FakeFlash flash;
    Store store(flash);
    const Config first = configNamed("rack-stable", 28);
    const Config second = configNamed("rack-interrupted", 30);
    assert(store.save(first));

    flash.writeLimit = 40;
    assert(!store.save(second));
    flash.writeLimit = std::numeric_limits<size_t>::max();

    Config loaded{};
    Store rebooted(flash);
    assert(rebooted.load(loaded) == LoadResult::OK);
    assert(loaded == first);
}

void testFactoryEraseRemovesBothSlots() {
    FakeFlash flash;
    Store store(flash);
    assert(store.save(configNamed("rack-one", 28)));
    assert(store.save(configNamed("rack-two", 29)));
    assert(store.erase());

    Config loaded{};
    Store rebooted(flash);
    assert(rebooted.load(loaded) == LoadResult::EMPTY);
}

void testInterruptedFactoryResetPreservesPreviousConfig() {
    FakeFlash flash;
    Store store(flash);
    const Config original = configNamed("rack-stable", 28);
    assert(store.save(original));

    flash.writeLimit = 8;
    assert(!store.erase());
    flash.writeLimit = std::numeric_limits<size_t>::max();

    Config loaded{};
    Store rebooted(flash);
    assert(rebooted.load(loaded) == LoadResult::OK);
    assert(loaded == original);
}

}  // namespace

int main() {
    testLayoutUsesOnlyBootloaderPreservedConfigPages();
    testEmptySaveAndReload();
    testNewestSlotWinsAndCorruptionFallsBack();
    testInterruptedWritePreservesPreviousSlot();
    testFactoryEraseRemovesBothSlots();
    testInterruptedFactoryResetPreservesPreviousConfig();
    std::cout << "rak4631 config store tests passed\n";
    return 0;
}
