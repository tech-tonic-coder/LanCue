#pragma once

#include "corelib/platform/IClipboardController.h"

namespace lancue::platform::linux_ {

// Not yet implemented (§4.5: handle unsupported/failed gracefully rather
// than silently no-op). The real mechanism, when this platform is
// actually being built and tested, differs by display server the same
// way Phase 2/3's watcher/hotkey manager do: X11 exposes clipboard
// contents via the CLIPBOARD selection (XGetSelectionOwner/
// XConvertSelection to read, ICCCM SetSelectionOwner to write) with
// change notification via the XFixes extension's
// XFixesSelectionNotifyEvent — the actual X11 analog of Windows'
// AddClipboardFormatListener, not a polling loop. Wayland brokers
// clipboard access through the compositor (wl_data_device_manager /
// the data-control protocol for a non-interactive daemon like this one),
// a fundamentally different integration than X11's selection model, same
// caveat Phase 2's Learnings entries already documented for layout-change
// notification on Wayland. Left as a stub for now since there's no Linux
// desktop machine in this project's current dev loop to verify either
// path against.
class ClipboardControllerLinux final : public IClipboardController {
public:
    void setCallback(ClipboardChangedCallback callback) override;
    bool start() override;
    void stop() override;
    bool hasText() const override;
    std::optional<QString> readText() const override;
    bool writeText(const QString& text) override;

private:
    ClipboardChangedCallback m_callback;
};

} // namespace lancue::platform::linux_
