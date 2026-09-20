#pragma once

#include "corelib/platform/IClipboardController.h"

namespace lancue::platform::macos {

// Not yet implemented (§4.5). The real mechanism, when this platform is
// actually being built and tested, is NSPasteboard for read/write, with
// change notification via polling NSPasteboard's changeCount property —
// macOS genuinely has no push notification for "the general pasteboard
// changed" (confirmed by every real macOS clipboard-manager utility's own
// documented approach: they all poll changeCount, typically on a short
// interval timer while actively watching). That makes this one of the
// rare cases §4.7 itself already anticipates ("a short-interval repeating
// timer is a red flag... in almost every case for this project [an OS
// callback exists]" — implying the rare exception where it genuinely
// doesn't), not a violation to fix later; the real implementation should
// document this explicitly at the point it polls, the same way this
// comment documents it here ahead of time. Left as a stub for now since
// there's no macOS machine in this project's current dev loop to verify
// against.
class ClipboardControllerMacos final : public IClipboardController {
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

} // namespace lancue::platform::macos
