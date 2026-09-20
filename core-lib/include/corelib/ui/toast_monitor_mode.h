#pragma once

#include <QString>

namespace lancue::ui {

// Which monitor(s) the toast shows on, when more than one is connected.
// Independent of ToastPosition (toast_position.h): position is *where on
// a screen*, this is *which screen(s)* — orthogonal settings, so e.g.
// "top-left" + "all monitors" means every connected screen gets its own
// top-left-anchored toast, simultaneously.
enum class ToastMonitorMode {
    // The OS's own designated primary display, regardless of where the
    // cursor or any window currently is.
    Primary,
    // Every currently-connected screen gets its own toast, shown at the
    // same time. On a single-monitor machine this is indistinguishable
    // from any other mode — QGuiApplication::screens() only ever has the
    // one entry — so nothing here needs a separate single-monitor case.
    All,
    // One specific screen, chosen by its index into
    // QGuiApplication::screens() (Settings::toastMonitorIndex) — see that
    // field's own comment in settings_schema.h for the caveat about
    // index stability across monitor reconnects.
    Specific,
    // Whichever screen the mouse cursor is currently over
    // (QGuiApplication::screenAt(QCursor::pos())) — this was this
    // feature's only behavior before per-monitor targeting existed
    // (Phase 7's original implementation), kept as the default so an
    // existing settings.json that predates this field behaves exactly as
    // before.
    CursorScreen,
};

// "primary"/"all"/"specific"/"cursor" — the exact string persisted in
// Settings::toastMonitorMode and round-tripped through settings.json,
// same plain-kebab-case-on-disk reasoning as ui::ToastPosition's own
// toCode()/fromCode().
QString toCode(ToastMonitorMode mode);

// Parses toCode()'s own output, falling back to `fallback` (CursorScreen
// by default, matching this feature's original single-monitor-targeting
// behavior) for an unrecognized/empty code — same lenient-per-field
// philosophy as every other settings_schema.cpp field.
ToastMonitorMode fromCode(const QString& code, ToastMonitorMode fallback = ToastMonitorMode::CursorScreen);

} // namespace lancue::ui
