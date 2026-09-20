#include <catch2/catch_test_macros.hpp>

#include <QStringList>

#include "corelib/i18n/language.h"

using namespace lancue::i18n;

TEST_CASE("toCode()/fromCode() round-trip both known languages", "[i18n]") {
    CHECK(toCode(AppLanguage::English) == QStringLiteral("en"));
    CHECK(toCode(AppLanguage::Persian) == QStringLiteral("fa"));

    CHECK(fromCode(QStringLiteral("en")) == AppLanguage::English);
    CHECK(fromCode(QStringLiteral("fa")) == AppLanguage::Persian);
    // Case-insensitive: a hand-edited settings.json ("FA", "En") must
    // still parse rather than silently falling back (see fromJson()'s
    // own per-field leniency philosophy in settings_schema.cpp).
    CHECK(fromCode(QStringLiteral("FA")) == AppLanguage::Persian);
}

TEST_CASE("fromCode() falls back for an unknown code", "[i18n]") {
    CHECK(fromCode(QStringLiteral("de")) == AppLanguage::English);
    CHECK(fromCode(QStringLiteral("")) == AppLanguage::English);
    CHECK(fromCode(QStringLiteral("xx"), AppLanguage::Persian) == AppLanguage::Persian);
}

TEST_CASE("directionFor() is RTL only for Persian", "[i18n]") {
    CHECK(directionFor(AppLanguage::English) == LayoutDirection::LeftToRight);
    CHECK(directionFor(AppLanguage::Persian) == LayoutDirection::RightToLeft);
}

TEST_CASE("layoutDisplayName() covers every conversion-engine layout in both languages", "[i18n]") {
    const QStringList ids{QStringLiteral("en-us"), QStringLiteral("fa-ir"), QStringLiteral("ar-sa"),
                           QStringLiteral("ru-ru"), QStringLiteral("tr-tr"), QStringLiteral("he-il")};

    for (const auto& id : ids) {
        const QString english = layoutDisplayName(id, AppLanguage::English);
        const QString persian = layoutDisplayName(id, AppLanguage::Persian);
        CHECK_FALSE(english.isEmpty());
        CHECK_FALSE(persian.isEmpty());
        // The two languages must actually differ for a real layout id —
        // guards against a copy-paste mistake in the lookup tables that
        // would otherwise pass a plain "isEmpty()" check.
        CHECK(english != persian);
    }
}

TEST_CASE("layoutDisplayName() falls back to a legible name for an unmapped id", "[i18n]") {
    const QString result = layoutDisplayName(QStringLiteral("de-de"), AppLanguage::English);
    CHECK(result == QStringLiteral("De-de"));
}

TEST_CASE("layoutDisplayName() returns a deliberate 'unknown' message for an empty id, not a blank string",
          "[i18n]") {
    // The empty-string sentinel IKeyboardLayoutWatcher::currentLayout()
    // returns when the layout genuinely couldn't be determined (Phase 7,
    // 2026-08-30) must never silently produce an empty toast title.
    const QString english = layoutDisplayName(QString(), AppLanguage::English);
    const QString persian = layoutDisplayName(QString(), AppLanguage::Persian);
    CHECK_FALSE(english.isEmpty());
    CHECK_FALSE(persian.isEmpty());
    CHECK(english != persian);
}

TEST_CASE("toastLayoutChangedText() differs by language and is never empty", "[i18n]") {
    const QString english = toastLayoutChangedText(AppLanguage::English);
    const QString persian = toastLayoutChangedText(AppLanguage::Persian);
    CHECK_FALSE(english.isEmpty());
    CHECK_FALSE(persian.isEmpty());
    CHECK(english != persian);
}

TEST_CASE("layoutBadgeCode() derives the expected 2-letter code for every conversion-engine layout", "[i18n]") {
    CHECK(layoutBadgeCode(QStringLiteral("en-us")) == QStringLiteral("EN"));
    CHECK(layoutBadgeCode(QStringLiteral("fa-ir")) == QStringLiteral("FA"));
    CHECK(layoutBadgeCode(QStringLiteral("ar-sa")) == QStringLiteral("AR"));
    CHECK(layoutBadgeCode(QStringLiteral("ru-ru")) == QStringLiteral("RU"));
    CHECK(layoutBadgeCode(QStringLiteral("tr-tr")) == QStringLiteral("TR"));
    CHECK(layoutBadgeCode(QStringLiteral("he-il")) == QStringLiteral("HE"));
}

TEST_CASE("layoutBadgeCode() falls back to 'LC' for the empty/unknown-layout sentinel", "[i18n]") {
    CHECK(layoutBadgeCode(QString()) == QStringLiteral("LC"));
    CHECK(layoutBadgeCode(QStringLiteral("x")) == QStringLiteral("LC"));
}

TEST_CASE("layoutBadgeCode() derives a code for an unmapped future layout id too", "[i18n]") {
    CHECK(layoutBadgeCode(QStringLiteral("de-de")) == QStringLiteral("DE"));
}
