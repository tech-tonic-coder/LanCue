#include "ui/fonts.h"

#include <QFontDatabase>
#include <QStringList>

#include "corelib/logging/logger.h"

namespace lancue::ui {

void loadApplicationFonts() {
    // One addApplicationFont() call per embedded file — the four weights
    // per family bundled in fonts.qrc (Regular/Medium/SemiBold/Bold).
    // Medium isn't used by the toast today (only title=SemiBold,
    // subtitle=Regular — see toast_window.cpp) but is still loaded now:
    // Phase 9's settings UI is the next consumer of these same fonts, and
    // loading is a one-time startup cost paid once here regardless of
    // which weights any given window ends up using.
    const QStringList files{
        QStringLiteral(":/fonts/manrope/Manrope-Regular.ttf"),
        QStringLiteral(":/fonts/manrope/Manrope-Medium.ttf"),
        QStringLiteral(":/fonts/manrope/Manrope-SemiBold.ttf"),
        QStringLiteral(":/fonts/manrope/Manrope-Bold.ttf"),
        QStringLiteral(":/fonts/vazirmatn/Vazirmatn-Regular.ttf"),
        QStringLiteral(":/fonts/vazirmatn/Vazirmatn-Medium.ttf"),
        QStringLiteral(":/fonts/vazirmatn/Vazirmatn-SemiBold.ttf"),
        QStringLiteral(":/fonts/vazirmatn/Vazirmatn-Bold.ttf"),
    };

    for (const auto& path : files) {
        const int id = QFontDatabase::addApplicationFont(path);
        if (id == -1) {
            // Not fatal: Qt substitutes the nearest installed font for
            // any family this fails to register, so the toast still
            // renders text, just not in the intended typeface — worth a
            // loud log (a resource that should always be present failed
            // to load, likely a packaging/qrc mistake) but not worth
            // aborting startup over.
            logError(QStringLiteral("ui::loadApplicationFonts: failed to load bundled font '%1'.").arg(path));
        }
    }
}

QString titleFontFamily(i18n::AppLanguage language) {
    switch (language) {
        case i18n::AppLanguage::Persian:
            return QStringLiteral("Vazirmatn SemiBold");
        case i18n::AppLanguage::English:
        default:
            return QStringLiteral("Manrope SemiBold");
    }
}

QString bodyFontFamily(i18n::AppLanguage language) {
    switch (language) {
        case i18n::AppLanguage::Persian:
            return QStringLiteral("Vazirmatn");
        case i18n::AppLanguage::English:
        default:
            return QStringLiteral("Manrope");
    }
}

} // namespace lancue::ui
