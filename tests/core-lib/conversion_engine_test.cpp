#include <catch2/catch_test_macros.hpp>

#include "corelib/conversion/conversion_engine.h"
#include "corelib/conversion/layout_char_maps.h"

using namespace lancue::conversion;

TEST_CASE("findCharMap returns tables for both directions of en-us/fa-ir", "[conversion]") {
    CHECK(findCharMap(QStringLiteral("en-us"), QStringLiteral("fa-ir")) != nullptr);
    CHECK(findCharMap(QStringLiteral("fa-ir"), QStringLiteral("en-us")) != nullptr);
}

TEST_CASE("findCharMap returns tables for both directions of every ported language pair", "[conversion]") {
    for (const QString& target : {QStringLiteral("fa-ir"), QStringLiteral("ar-sa"), QStringLiteral("ru-ru"),
                                   QStringLiteral("tr-tr"), QStringLiteral("he-il")}) {
        CAPTURE(target);
        CHECK(findCharMap(QStringLiteral("en-us"), target) != nullptr);
        CHECK(findCharMap(target, QStringLiteral("en-us")) != nullptr);
    }
}

TEST_CASE("findCharMap returns nullptr for a pair that doesn't pivot through en-us", "[conversion]") {
    // Matches Switex's own MAPS registry: every non-English pair only
    // ever converts through English, so e.g. fa-ir -> ar-sa directly has
    // no table, same as Switex has no ('fa', 'ar') entry.
    CHECK(findCharMap(QStringLiteral("fa-ir"), QStringLiteral("ar-sa")) == nullptr);
    CHECK(findCharMap(QStringLiteral("ru-ru"), QStringLiteral("he-il")) == nullptr);
}

TEST_CASE("findCharMap returns nullptr for an unconfigured pair", "[conversion]") {
    CHECK(findCharMap(QStringLiteral("en-us"), QStringLiteral("ur-pk")) == nullptr);
    CHECK(findCharMap(QStringLiteral("de-de"), QStringLiteral("fr-fr")) == nullptr);
}

TEST_CASE("supportedLayoutIds contains en-us plus all five ported target layouts", "[conversion]") {
    const QStringList ids = supportedLayoutIds();
    CHECK(ids.size() == 6);
    for (const QString& expected : {QStringLiteral("en-us"), QStringLiteral("fa-ir"), QStringLiteral("ar-sa"),
                                     QStringLiteral("ru-ru"), QStringLiteral("tr-tr"), QStringLiteral("he-il")}) {
        CAPTURE(expected);
        CHECK(ids.contains(expected));
    }
}

TEST_CASE("convert() maps a known English-layout string to its Persian equivalent", "[conversion]") {
    // "salam" typed on an English keyboard while the Persian Standard
    // layout was active reads as this Persian word — a real, checkable
    // round trip rather than an arbitrary single-character spot check.
    const ConversionResult result = convert(QStringLiteral("sghl"), QStringLiteral("en-us"), QStringLiteral("fa-ir"));
    REQUIRE(result.ok);
    CHECK(result.text == QString::fromUtf8("سلام"));
}

TEST_CASE("convert() round-trips Persian back to the original English-layout keys", "[conversion]") {
    const ConversionResult toFa = convert(QStringLiteral("sghl"), QStringLiteral("en-us"), QStringLiteral("fa-ir"));
    REQUIRE(toFa.ok);
    const ConversionResult backToEn = convert(toFa.text, QStringLiteral("fa-ir"), QStringLiteral("en-us"));
    REQUIRE(backToEn.ok);
    CHECK(backToEn.text == QStringLiteral("sghl"));
}

TEST_CASE("convert() passes an unmapped character through unchanged", "[conversion]") {
    // An emoji (and, for the filler on either side, Chinese characters)
    // have no entry in either direction's table — the documented
    // ambiguous/unmapped-character policy (§5 Phase 6) is passthrough,
    // not dropping the character or raising an error. Deliberately NOT
    // using plain ASCII letters as filler here: nearly every QWERTY key
    // (including all of a-z) has an explicit entry in the ported table,
    // so plain letters would exercise the mapped path instead of the
    // unmapped one this test is actually about. Written as UTF-16
    // surrogate pair literals since QChar can't hold a full astral
    // codepoint on its own; convert() iterates QString by QChar, so this
    // also exercises that a surrogate pair survives untouched as two
    // separate passthrough characters, not merged or corrupted.
    const QString filler = QString::fromUtf8("\xe4\xbd\xa0\xe5\xa5\xbd"); // "你好" — not in either table
    const QString withEmoji = filler + QChar(0xD83D) + QChar(0xDE00) + filler;
    const ConversionResult result = convert(withEmoji, QStringLiteral("en-us"), QStringLiteral("fa-ir"));
    REQUIRE(result.ok);
    CHECK(result.text == withEmoji);
}

TEST_CASE("convert() treats identity-mapped punctuation as documented, not arbitrarily", "[conversion]") {
    // '.', '-', '=', space, etc. have explicit identity entries in the
    // ported table (present in both layouts) — this is this phase's
    // documented answer for shared/ambiguous characters, distinct from
    // "no entry at all" (the emoji case above), even though the visible
    // result is the same passthrough behavior either way.
    const ConversionResult result =
        convert(QStringLiteral("a.b-c=d e"), QStringLiteral("en-us"), QStringLiteral("fa-ir"));
    REQUIRE(result.ok);
    CHECK(result.text.contains(QLatin1Char('.')));
    CHECK(result.text.contains(QLatin1Char('-')));
    CHECK(result.text.contains(QLatin1Char('=')));
    CHECK(result.text.contains(QLatin1Char(' ')));
}

TEST_CASE("convert() is a no-op when fromLayout equals toLayout", "[conversion]") {
    const ConversionResult result = convert(QStringLiteral("sghl"), QStringLiteral("en-us"), QStringLiteral("en-us"));
    REQUIRE(result.ok);
    CHECK(result.text == QStringLiteral("sghl"));
}

TEST_CASE("convert() reports failure (not a silent passthrough) for an unconfigured pair", "[conversion]") {
    const ConversionResult result = convert(QStringLiteral("hello"), QStringLiteral("en-us"), QStringLiteral("ur-pk"));
    CHECK_FALSE(result.ok);
    CHECK(result.text == QStringLiteral("hello"));
}

TEST_CASE("convert() pivots through en-us for a pair with no direct table", "[conversion]") {
    // fa-ir -> ar-sa has no direct table (only en-us pairs exist), but
    // Switex's own convert() recovers via fromLayout -> en-us -> toLayout
    // rather than failing — ported here, not invented. "salam" typed on
    // an English keyboard reads as Persian سلام; converting that Persian
    // text directly to Arabic-layout keys should match manually pivoting
    // it through en-us by hand.
    const ConversionResult viaFa = convert(QStringLiteral("sghl"), QStringLiteral("en-us"), QStringLiteral("fa-ir"));
    REQUIRE(viaFa.ok);

    const ConversionResult pivoted = convert(viaFa.text, QStringLiteral("fa-ir"), QStringLiteral("ar-sa"));
    REQUIRE(pivoted.ok);

    const ConversionResult manualStep1 = convert(viaFa.text, QStringLiteral("fa-ir"), QStringLiteral("en-us"));
    REQUIRE(manualStep1.ok);
    const ConversionResult manualStep2 = convert(manualStep1.text, QStringLiteral("en-us"), QStringLiteral("ar-sa"));
    REQUIRE(manualStep2.ok);

    CHECK(pivoted.text == manualStep2.text);
}

TEST_CASE("convert() maps known strings for the untouched (verified-correct) AR/RU tables", "[conversion]") {
    // Expected strings computed directly from Switex's own config.py
    // tables (not hand-guessed) — ar-sa and ru-ru were left as originally
    // ported since they already matched the real Windows Arabic (101)
    // and Russian ЙЦУКЕН keyboard layouts (verified against Microsoft's
    // own layout data; see the Phase 6 Learnings entry), unlike tr-tr and
    // he-il below.
    CHECK(convert(QStringLiteral("salam"), QStringLiteral("en-us"), QStringLiteral("ar-sa")).text ==
          QString::fromUtf8("سشمشو"));
    CHECK(convert(QStringLiteral("hello"), QStringLiteral("en-us"), QStringLiteral("ru-ru")).text ==
          QString::fromUtf8("руддщ"));
}

TEST_CASE("convert() maps known strings for the corrected tr-tr/he-il tables", "[conversion]") {
    // Switex's own tr-tr and he-il tables were factually wrong (see the
    // Phase 6 Learnings entry) and were replaced with the real Windows
    // "Turkish Q" and "Hebrew" keyboard layouts — these expected strings
    // are computed from that corrected data, not from Switex.
    //
    // "hello" happens to be identity-mapped on the real Turkish Q layout
    // (h, e, l, o are all unaccented keys) — still a meaningful check
    // that the table wasn't left as Switex's old alphabetical-shift
    // scheme, which would have produced "gdiil" instead.
    CHECK(convert(QStringLiteral("hello"), QStringLiteral("en-us"), QStringLiteral("tr-tr")).text ==
          QStringLiteral("hello"));
    // "shalom" on the real Hebrew layout deliberately includes the 'l'
    // key, which maps to final-kaf (ך) rather than lamed (the 'k' key
    // does that instead) — an easy pair to accidentally swap, so this
    // catches that specific mistake, not just any Hebrew output.
    CHECK(convert(QStringLiteral("shalom"), QStringLiteral("en-us"), QStringLiteral("he-il")).text ==
          QString::fromUtf8("דישךםצ"));
}

TEST_CASE("detectLayout recognizes Persian text", "[conversion]") {
    CHECK(detectLayout(QString::fromUtf8("سلام")) == QStringLiteral("fa-ir"));
}

TEST_CASE("detectLayout recognizes English text", "[conversion]") {
    CHECK(detectLayout(QStringLiteral("hello world")) == QStringLiteral("en-us"));
}

TEST_CASE("detectLayout defaults to en-us for text with no letters at all", "[conversion]") {
    CHECK(detectLayout(QStringLiteral("12345 !@#$%")) == QStringLiteral("en-us"));
    CHECK(detectLayout(QStringLiteral("")) == QStringLiteral("en-us"));
}

TEST_CASE("detectLayout favors fa-ir on a genuine nonzero tie, matching Switex's own tie-break", "[conversion]") {
    // One Persian letter, one Latin letter: equal nonzero counts.
    const QString tied = QString::fromUtf8("ا") + QStringLiteral("a");
    CHECK(detectLayout(tied) == QStringLiteral("fa-ir"));
}
