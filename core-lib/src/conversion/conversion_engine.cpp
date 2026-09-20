#include "corelib/conversion/conversion_engine.h"

#include "corelib/conversion/layout_char_maps.h"

namespace lancue::conversion {

ConversionResult convert(const QString& text, const QString& fromLayout, const QString& toLayout) {
    // Matches Switex's own convert() (switex.py: "if from_lang ==
    // to_lang: return text, from_lang, to_lang") — most relevant to the
    // per-language dedicated hotkeys, whose detected source can
    // legitimately equal the forced target (e.g. pressing "convert to
    // Persian" on text detectLayout() already calls Persian). That's a
    // deliberate no-op, not a missing-table failure, so it must not go
    // through findCharMap() at all (no self-pair table exists, nor
    // should one).
    if (fromLayout == toLayout) {
        return ConversionResult{text, true};
    }

    const CharMap* map = findCharMap(fromLayout, toLayout);
    if (map) {
        QString result;
        result.reserve(text.size());
        for (const QChar ch : text) {
            result.append(map->value(ch, ch));
        }
        return ConversionResult{result, true};
    }

    // No direct table for this pair — e.g. fa-ir -> ar-sa, since every
    // ported table only ever pivots through en-us (see
    // layout_char_maps.cpp). Ported from Switex's own convert()
    // (switex.py), which hits this same "mapping not found" case and
    // recovers by converting fromLayout -> en-us -> toLayout as two
    // separate table lookups, rather than treating it as a hard failure.
    // Only attempted when neither side is already en-us, matching
    // Switex's own guard exactly (this also bounds the recursion to one
    // extra level, since both recursive calls below always have en-us on
    // one side and therefore hit the direct-table branch above, never
    // this one again).
    static const QString kPivotLayout = QStringLiteral("en-us");
    if (fromLayout != kPivotLayout && toLayout != kPivotLayout) {
        const ConversionResult toPivot = convert(text, fromLayout, kPivotLayout);
        if (!toPivot.ok) {
            return ConversionResult{text, false};
        }
        return convert(toPivot.text, kPivotLayout, toLayout);
    }

    return ConversionResult{text, false};
}

QString detectLayout(const QString& text) {
    // Character sets ported verbatim from Switex's `detect_lang()`
    // (switex.py). he_chars deliberately includes two characters
    // (U+0645, U+0646) that are actually Arabic, not Hebrew — the same
    // upstream anomaly already noted on the ported ar-sa/he-il tables,
    // kept as-is rather than corrected.
    static const QString kFaChars = QString::fromUtf8(
        "\u0627\u0628\u067e\u062a\u062b\u062c\u0686\u062d\u062e\u062f\u0630\u0631\u0632\u0698\u0633\u0634\u0635"
        "\u0636\u0637\u0638\u0639\u063a\u0641\u0642\u06a9\u06af\u0644\u0645\u0646\u0648\u0647\u06cc");
    static const QString kRuChars = QString::fromUtf8(
        "\u0430\u0431\u0432\u0433\u0434\u0435\u0451\u0436\u0437\u0438\u0439\u043a\u043b\u043c\u043d\u043e\u043f"
        "\u0440\u0441\u0442\u0443\u0444\u0445\u0446\u0447\u0448\u0449\u044a\u044b\u044c\u044d\u044e\u044f");
    static const QString kTrChars = QString::fromUtf8("\u00e7\u011f\u0131\u00f6\u015f\u00fc\u0130");
    static const QString kHeChars = QString::fromUtf8(
        "\u05d0\u05d1\u05d2\u05d3\u05d4\u05d5\u05d6\u05d7\u05d8\u05d9\u05db\u05dc\u0645\u0646\u05e1\u05e2\u05e4"
        "\u05e6\u05e7\u05e8\u05e9\u05ea");

    int faCount = 0;
    int ruCount = 0;
    int trCount = 0;
    int heCount = 0;
    int enCount = 0;
    // Priority order matches Switex's own if/elif chain exactly: fa,
    // then ru, then he, then plain a-z Latin (en), then tr last — which
    // is why Turkish's own accented letters are checked only after the
    // a-z case has already claimed anything in that range (see this
    // function's header doc comment for what that means in practice).
    for (const QChar ch : text.toLower()) {
        if (kFaChars.contains(ch)) {
            ++faCount;
        } else if (kRuChars.contains(ch)) {
            ++ruCount;
        } else if (kHeChars.contains(ch)) {
            ++heCount;
        } else if (ch >= u'a' && ch <= u'z') {
            ++enCount;
        } else if (kTrChars.contains(ch)) {
            ++trCount;
        }
    }

    // Replicates Python's `max(counts, key=counts.get)` over a dict
    // whose insertion order is fa, ru, tr, he, en — since Python's max()
    // only replaces the current best on a *strictly* greater value, the
    // first key reached with the maximum count wins any tie, which is
    // exactly what this loop does too.
    struct Candidate {
        QString layoutId;
        int count;
    };
    const Candidate candidates[] = {
        {QStringLiteral("fa-ir"), faCount},
        {QStringLiteral("ru-ru"), ruCount},
        {QStringLiteral("tr-tr"), trCount},
        {QStringLiteral("he-il"), heCount},
        {QStringLiteral("en-us"), enCount},
    };
    const Candidate* best = &candidates[0];
    for (const Candidate& candidate : candidates) {
        if (candidate.count > best->count) {
            best = &candidate;
        }
    }
    return best->count > 0 ? best->layoutId : QStringLiteral("en-us");
}

} // namespace lancue::conversion
