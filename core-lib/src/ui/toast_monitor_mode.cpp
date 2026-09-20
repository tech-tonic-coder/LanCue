#include "corelib/ui/toast_monitor_mode.h"

namespace lancue::ui {

QString toCode(ToastMonitorMode mode) {
    switch (mode) {
        case ToastMonitorMode::Primary:
            return QStringLiteral("primary");
        case ToastMonitorMode::All:
            return QStringLiteral("all");
        case ToastMonitorMode::Specific:
            return QStringLiteral("specific");
        case ToastMonitorMode::CursorScreen:
            return QStringLiteral("cursor");
    }
    return QStringLiteral("cursor");
}

ToastMonitorMode fromCode(const QString& code, ToastMonitorMode fallback) {
    if (code.compare(QStringLiteral("primary"), Qt::CaseInsensitive) == 0) {
        return ToastMonitorMode::Primary;
    }
    if (code.compare(QStringLiteral("all"), Qt::CaseInsensitive) == 0) {
        return ToastMonitorMode::All;
    }
    if (code.compare(QStringLiteral("specific"), Qt::CaseInsensitive) == 0) {
        return ToastMonitorMode::Specific;
    }
    if (code.compare(QStringLiteral("cursor"), Qt::CaseInsensitive) == 0) {
        return ToastMonitorMode::CursorScreen;
    }
    return fallback;
}

} // namespace lancue::ui
