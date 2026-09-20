#include "corelib/ui/app_theme_mode.h"

namespace lancue::ui {

QString toCode(AppThemeMode mode) {
    switch (mode) {
        case AppThemeMode::Auto:
            return QStringLiteral("auto");
        case AppThemeMode::Light:
            return QStringLiteral("light");
        case AppThemeMode::Dark:
            return QStringLiteral("dark");
    }
    return QStringLiteral("auto");
}

AppThemeMode fromCode(const QString& code, AppThemeMode fallback) {
    if (code.compare(QStringLiteral("auto"), Qt::CaseInsensitive) == 0) {
        return AppThemeMode::Auto;
    }
    if (code.compare(QStringLiteral("light"), Qt::CaseInsensitive) == 0) {
        return AppThemeMode::Light;
    }
    if (code.compare(QStringLiteral("dark"), Qt::CaseInsensitive) == 0) {
        return AppThemeMode::Dark;
    }
    return fallback;
}

} // namespace lancue::ui
