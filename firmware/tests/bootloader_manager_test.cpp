#include "bootloader_manager.h"

#include <cassert>
#include <iostream>

using namespace BootloaderManager;

namespace {

struct Calls {
    int count = 0;
    Mode mode = Mode::NONE;
};

void transition(Mode mode, void* context) {
    auto& calls = *static_cast<Calls*>(context);
    ++calls.count;
    calls.mode = mode;
}

void testOnlyRunsAfterSuccessfulResponseCloseAndDelay() {
    DeferredTransition deferred;
    Calls calls;
    deferred.arm(Mode::BLE_OTA, 900);
    deferred.poll(1000, transition, &calls);
    assert(calls.count == 0);
    deferred.responseClosed(1000);
    deferred.poll(1000 + TRANSITION_DELAY_MS - 1, transition, &calls);
    assert(calls.count == 0);
    deferred.poll(1000 + TRANSITION_DELAY_MS, transition, &calls);
    assert(calls.count == 1);
    assert(calls.mode == Mode::BLE_OTA);
    deferred.poll(5000, transition, &calls);
    assert(calls.count == 1);
}

void testFailedResponseCancelsAndReplacementIsExplicit() {
    DeferredTransition deferred;
    Calls calls;
    deferred.arm(Mode::BLE_OTA, 0);
    deferred.responseAborted();
    deferred.responseClosed(100);
    deferred.poll(1000, transition, &calls);
    assert(calls.count == 0);

    deferred.arm(Mode::BLE_OTA, 0xffffffe0U);
    deferred.arm(Mode::UF2_USB, 0);
    deferred.responseClosed(0xfffffff0U);
    deferred.poll(static_cast<uint32_t>(0xfffffff0U + TRANSITION_DELAY_MS), transition, &calls);
    assert(calls.count == 1);
    assert(calls.mode == Mode::BLE_OTA);
}

void testRebootSurvivesFailedResponseAndUsesBoundedFallback() {
    DeferredTransition deferred;
    Calls calls;
    deferred.arm(Mode::REBOOT, 1000);
    deferred.responseAborted();

    deferred.poll(1000 + REBOOT_FALLBACK_DELAY_MS - 1, transition, &calls);
    assert(calls.count == 0);
    deferred.poll(1000 + REBOOT_FALLBACK_DELAY_MS, transition, &calls);
    assert(calls.count == 1);
    assert(calls.mode == Mode::REBOOT);
}

void testCommittedTransitionCannotBeCancelledOrReplaced() {
    DeferredTransition deferred;
    Calls calls;
    deferred.arm(Mode::REBOOT, 0);
    deferred.responseClosed(100);
    assert(deferred.committed());
    deferred.responseAborted();
    deferred.arm(Mode::BLE_OTA, 0);
    deferred.responseClosed(200);
    deferred.poll(100 + TRANSITION_DELAY_MS, transition, &calls);
    assert(calls.count == 1);
    assert(calls.mode == Mode::REBOOT);
}

}  // namespace

int main() {
    testOnlyRunsAfterSuccessfulResponseCloseAndDelay();
    testFailedResponseCancelsAndReplacementIsExplicit();
    testRebootSurvivesFailedResponseAndUsesBoundedFallback();
    testCommittedTransitionCannotBeCancelledOrReplaced();
    std::cout << "bootloader manager scheduling tests passed\n";
}
