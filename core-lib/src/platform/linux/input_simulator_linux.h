#pragma once

#include "corelib/platform/IInputSimulator.h"

namespace lancue::platform::linux_ {

// Not yet implemented (§4.5). The real mechanism, per the roadmap's own
// Phase 5 plan, is XTestFakeKeyEvent (the XTest extension) for X11 —
// exactly what real X11 automation tools (xdotool and similar) use for
// synthetic Ctrl+C/Ctrl+V injection. Wayland's tighter security model
// generally blocks synthetic input from an arbitrary client outside a
// compositor-specific portal/protocol, the same class of limitation
// already documented for Wayland hotkeys/clipboard elsewhere in this
// file's sibling stubs — needs its own investigation, not a fallback path
// inside this class. Left as a stub for now since there's no Linux
// desktop machine in this project's current dev loop to verify either
// path against.
class InputSimulatorLinux final : public IInputSimulator {
public:
    void simulateCopy() override;
    void simulatePaste() override;
};

} // namespace lancue::platform::linux_
