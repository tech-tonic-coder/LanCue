#pragma once

#include "corelib/platform/IInputSimulator.h"

namespace lancue::platform::macos {

// Not yet implemented (§4.5). The real mechanism, per the roadmap's own
// Phase 5 plan, is CGEventCreateKeyboardEvent + CGEventPost — the current
// documented Quartz Event Services approach real macOS automation tools
// use for synthetic input, superseding the older deprecated Carbon
// key-event APIs. Requires the process to have Accessibility permission
// granted in System Settings, the same category of permission
// GlobalHotkeyManagerMacos's own (also-stub) comment already flags for
// CGEventTap — worth noting explicitly in the real implementation's
// failure path so a denied-permission failure doesn't look identical to
// some other kind of failure. Left as a stub for now since there's no
// macOS machine in this project's current dev loop to verify against.
class InputSimulatorMacos final : public IInputSimulator {
public:
    void simulateCopy() override;
    void simulatePaste() override;
};

} // namespace lancue::platform::macos
