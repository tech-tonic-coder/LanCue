#pragma once

#include "corelib/platform/IInputSimulator.h"

namespace lancue::platform::windows {

// SendInput-based implementation — the documented, current mechanism for
// synthesizing keyboard input on Windows (the older keybd_event is itself
// documented as superseded by SendInput), and what every real Windows
// automation/hotkey utility uses for this exact purpose, so no §4.9
// research detour was needed here the way Phase 2's layout watcher
// needed one.
class InputSimulatorWindows final : public IInputSimulator {
public:
    void simulateCopy() override;
    void simulatePaste() override;
};

} // namespace lancue::platform::windows
