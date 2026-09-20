#include "corelib/ui/toast_position.h"

namespace lancue::ui {

QString toCode(ToastPosition position) {
    switch (position) {
        case ToastPosition::TopLeft:
            return QStringLiteral("top-left");
        case ToastPosition::TopCenter:
            return QStringLiteral("top-center");
        case ToastPosition::TopRight:
            return QStringLiteral("top-right");
        case ToastPosition::CenterLeft:
            return QStringLiteral("center-left");
        case ToastPosition::Center:
            return QStringLiteral("center");
        case ToastPosition::CenterRight:
            return QStringLiteral("center-right");
        case ToastPosition::BottomLeft:
            return QStringLiteral("bottom-left");
        case ToastPosition::BottomCenter:
            return QStringLiteral("bottom-center");
        case ToastPosition::BottomRight:
            return QStringLiteral("bottom-right");
    }
    return QStringLiteral("bottom-right");
}

ToastPosition fromCode(const QString& code, ToastPosition fallback) {
    if (code.compare(QStringLiteral("top-left"), Qt::CaseInsensitive) == 0) {
        return ToastPosition::TopLeft;
    }
    if (code.compare(QStringLiteral("top-center"), Qt::CaseInsensitive) == 0) {
        return ToastPosition::TopCenter;
    }
    if (code.compare(QStringLiteral("top-right"), Qt::CaseInsensitive) == 0) {
        return ToastPosition::TopRight;
    }
    if (code.compare(QStringLiteral("center-left"), Qt::CaseInsensitive) == 0) {
        return ToastPosition::CenterLeft;
    }
    if (code.compare(QStringLiteral("center"), Qt::CaseInsensitive) == 0) {
        return ToastPosition::Center;
    }
    if (code.compare(QStringLiteral("center-right"), Qt::CaseInsensitive) == 0) {
        return ToastPosition::CenterRight;
    }
    if (code.compare(QStringLiteral("bottom-left"), Qt::CaseInsensitive) == 0) {
        return ToastPosition::BottomLeft;
    }
    if (code.compare(QStringLiteral("bottom-center"), Qt::CaseInsensitive) == 0) {
        return ToastPosition::BottomCenter;
    }
    if (code.compare(QStringLiteral("bottom-right"), Qt::CaseInsensitive) == 0) {
        return ToastPosition::BottomRight;
    }
    return fallback;
}

} // namespace lancue::ui
