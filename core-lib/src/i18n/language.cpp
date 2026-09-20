#include "corelib/i18n/language.h"

#include <QHash>

namespace lancue::i18n {

QString toCode(AppLanguage language) {
    switch (language) {
        case AppLanguage::English:
            return QStringLiteral("en");
        case AppLanguage::Persian:
            return QStringLiteral("fa");
    }
    return QStringLiteral("en");
}

AppLanguage fromCode(const QString& code, AppLanguage fallback) {
    if (code.compare(QStringLiteral("en"), Qt::CaseInsensitive) == 0) {
        return AppLanguage::English;
    }
    if (code.compare(QStringLiteral("fa"), Qt::CaseInsensitive) == 0) {
        return AppLanguage::Persian;
    }
    return fallback;
}

LayoutDirection directionFor(AppLanguage language) {
    switch (language) {
        case AppLanguage::Persian:
            return LayoutDirection::RightToLeft;
        case AppLanguage::English:
        default:
            return LayoutDirection::LeftToRight;
    }
}

namespace {

// One entry per layout the conversion engine supports — keep in sync
// with layout_char_maps.h. Two languages doesn't justify a fancier
// lookup structure than a table per language.
QString layoutDisplayNameEnglish(const QString& id) {
    static const QHash<QString, QString> names{
        {QStringLiteral("en-us"), QStringLiteral("English (US)")},
        {QStringLiteral("fa-ir"), QStringLiteral("Persian")},
        {QStringLiteral("ar-sa"), QStringLiteral("Arabic")},
        {QStringLiteral("ru-ru"), QStringLiteral("Russian")},
        {QStringLiteral("tr-tr"), QStringLiteral("Turkish")},
        {QStringLiteral("he-il"), QStringLiteral("Hebrew")},
    };
    return names.value(id);
}

QString layoutDisplayNamePersian(const QString& id) {
    static const QHash<QString, QString> names{
        {QStringLiteral("en-us"), QStringLiteral("انگلیسی (آمریکا)")},
        {QStringLiteral("fa-ir"), QStringLiteral("فارسی")},
        {QStringLiteral("ar-sa"), QStringLiteral("عربی")},
        {QStringLiteral("ru-ru"), QStringLiteral("روسی")},
        {QStringLiteral("tr-tr"), QStringLiteral("ترکی")},
        {QStringLiteral("he-il"), QStringLiteral("عبری")},
    };
    return names.value(id);
}

// Fallback for an id we don't have a name for: "de-de" -> "De-de".
// layoutDisplayName() handles the empty-id case before this is reached.
QString titleCasedFallback(const QString& id) {
    QString result = id;
    result[0] = result[0].toUpper();
    return result;
}

// Shown when the layout genuinely couldn't be determined.
QString unknownLayoutText(AppLanguage language) {
    switch (language) {
        case AppLanguage::Persian:
            return QStringLiteral("نامشخص");
        case AppLanguage::English:
        default:
            return QStringLiteral("Unknown");
    }
}

} // namespace

QString layoutDisplayName(const QString& normalizedLayoutId, AppLanguage language) {
    if (normalizedLayoutId.isEmpty()) {
        return unknownLayoutText(language);
    }
    const QString found = (language == AppLanguage::Persian) ? layoutDisplayNamePersian(normalizedLayoutId)
                                                               : layoutDisplayNameEnglish(normalizedLayoutId);
    if (!found.isEmpty()) {
        return found;
    }
    return titleCasedFallback(normalizedLayoutId);
}

QString toastLayoutChangedText(AppLanguage language) {
    switch (language) {
        case AppLanguage::Persian:
            return QStringLiteral("زبان کیبورد تغییر کرد");
        case AppLanguage::English:
        default:
            return QStringLiteral("Keyboard layout changed");
    }
}

QString layoutBadgeCode(const QString& normalizedLayoutId) {
    if (normalizedLayoutId.size() < 2) {
        return QStringLiteral("LC");
    }
    return normalizedLayoutId.left(2).toUpper();
}

} // namespace lancue::i18n
