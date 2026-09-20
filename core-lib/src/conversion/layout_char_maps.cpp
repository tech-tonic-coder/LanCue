#include "corelib/conversion/layout_char_maps.h"

#include <vector>

#include <QPair>
#include <QSet>

namespace lancue::conversion {

namespace {

// Each of the five tables below is ordered (not just hashed) so
// reverseMap() can replicate Switex's first-occurrence-wins tie-break
// exactly (see reverseMap()). Ported character-for-character from
// Switex's config.py — do NOT hand-edit individual entries; any
// correction belongs in Switex's own source first, then gets re-ported
// here.
//
// All five map FROM en-us, matching Switex's own MAPS registry, which
// only ever defines the `('en', X)` direction by hand and derives every
// `(X, 'en')` reverse automatically (see generate_reverse_maps() in
// config.py) — reverseMap() below is that same derivation, ported.

// Ported from Switex's `_EN_FA_STD` (config.py) — the "Persian Standard"
// layout, the only Persian layout Switex itself treats as the
// default/primary one. `_EN_FA_LEG` (Legacy Persian, differing only in
// the backslash key) was NOT ported — out of this project's requested
// scope (§1: Persian/English), not an oversight; see the Phase 6
// Learnings entry.
//
// One entry was corrected against Switex's own source (Mahdi, 2026-08-23,
// after confirming against his real keyboard and the ISIRI 9147 national
// standard): Shift+' now produces U+061B (Arabic Semicolon) instead of
// Switex's U+FF1B (Fullwidth Semicolon) — the rest of the table (96 of
// 97 entries) was verified character-for-character against ISIRI 9147
// and left untouched. See the Phase 6 Learnings entry.
const std::vector<QPair<QChar, QChar>>& enUsToFaIrEntries() {
    static const std::vector<QPair<QChar, QChar>> entries = {
        {u'`', QChar(0x200d)}, {u'1', QChar(0x06f1)}, {u'2', QChar(0x06f2)}, {u'3', QChar(0x06f3)},
        {u'4', QChar(0x06f4)}, {u'5', QChar(0x06f5)}, {u'6', QChar(0x06f6)}, {u'7', QChar(0x06f7)},
        {u'8', QChar(0x06f8)}, {u'9', QChar(0x06f9)}, {u'0', QChar(0x06f0)}, {u'-', u'-'},
        {u'=', u'='},          {u'~', QChar(0x00f7)}, {u'!', u'!'},          {u'@', QChar(0x066c)},
        {u'#', QChar(0x066b)}, {u'$', QChar(0xfdfc)}, {u'%', QChar(0x066a)}, {u'^', QChar(0x00d7)},
        {u'&', QChar(0x060c)}, {u'*', u'*'},          {u'(', u')'},          {u')', u'('},
        {u'_', QChar(0x0640)}, {u'+', u'+'},          {u'q', QChar(0x0636)}, {u'w', QChar(0x0635)},
        {u'e', QChar(0x062b)}, {u'r', QChar(0x0642)}, {u't', QChar(0x0641)}, {u'y', QChar(0x063a)},
        {u'u', QChar(0x0639)}, {u'i', QChar(0x0647)}, {u'o', QChar(0x062e)}, {u'p', QChar(0x062d)},
        {u'[', QChar(0x062c)}, {u']', QChar(0x0686)}, {u'Q', QChar(0x0652)}, {u'W', QChar(0x064c)},
        {u'E', QChar(0x064d)}, {u'R', QChar(0x064b)}, {u'T', QChar(0x064f)}, {u'Y', QChar(0x0650)},
        {u'U', QChar(0x064e)}, {u'I', QChar(0x0651)}, {u'O', u']'},          {u'P', u'['},
        {u'{', u'}'},          {u'}', u'{'},          {u'a', QChar(0x0634)}, {u's', QChar(0x0633)},
        {u'd', QChar(0x06cc)}, {u'f', QChar(0x0628)}, {u'g', QChar(0x0644)}, {u'h', QChar(0x0627)},
        {u'j', QChar(0x062a)}, {u'k', QChar(0x0646)}, {u'l', QChar(0x0645)}, {u';', QChar(0x06a9)},
        {u'\'', QChar(0x06af)}, {u'A', QChar(0x0624)}, {u'S', QChar(0x0626)}, {u'D', QChar(0x064a)},
        {u'F', QChar(0x0625)}, {u'G', QChar(0x0623)}, {u'H', QChar(0x0622)}, {u'J', QChar(0x0629)},
        {u'K', QChar(0x00bb)}, {u'L', QChar(0x00ab)}, {u':', u':'},          {u'"', QChar(0x061b)},
        {u'z', QChar(0x0638)}, {u'x', QChar(0x0637)}, {u'c', QChar(0x0632)}, {u'v', QChar(0x0631)},
        {u'b', QChar(0x0630)}, {u'n', QChar(0x062f)}, {u'm', QChar(0x067e)}, {u',', QChar(0x0648)},
        {u'.', u'.'},          {u'/', u'/'},          {u'Z', QChar(0x0643)}, {u'X', QChar(0x0653)},
        {u'C', QChar(0x0698)}, {u'V', QChar(0x0670)}, {u'B', QChar(0x200c)}, {u'N', QChar(0x0654)},
        {u'M', QChar(0x0621)}, {u'<', u'>'},          {u'>', u'<'},          {u'?', QChar(0x061f)},
        {QChar(0x005c), QChar(0x005c)}, {u'|', u'|'}, {u' ', u' '},          {u'\n', u'\n'},
        {u'\t', u'\t'},
    };
    return entries;
}

// Ported from Switex's `_EN_AR` (config.py).
const std::vector<QPair<QChar, QChar>>& enUsToArSaEntries() {
    static const std::vector<QPair<QChar, QChar>> entries = {
        {u'1', QChar(0x0661)}, {u'2', QChar(0x0662)}, {u'3', QChar(0x0663)}, {u'4', QChar(0x0664)},
        {u'5', QChar(0x0665)}, {u'6', QChar(0x0666)}, {u'7', QChar(0x0667)}, {u'8', QChar(0x0668)},
        {u'9', QChar(0x0669)}, {u'0', QChar(0x0660)}, {u'-', u'-'},          {u'=', u'='},
        {u'q', QChar(0x0636)}, {u'w', QChar(0x0635)}, {u'e', QChar(0x062b)}, {u'r', QChar(0x0642)},
        {u't', QChar(0x0641)}, {u'y', QChar(0x063a)}, {u'u', QChar(0x0639)}, {u'i', QChar(0x0647)},
        {u'o', QChar(0x062e)}, {u'p', QChar(0x062d)}, {u'[', QChar(0x062c)}, {u']', QChar(0x062f)},
        {QChar(0x005c), QChar(0x005c)},
        {u'a', QChar(0x0634)}, {u's', QChar(0x0633)}, {u'd', QChar(0x064a)},
        {u'f', QChar(0x0628)}, {u'g', QChar(0x0644)}, {u'h', QChar(0x0627)}, {u'j', QChar(0x062a)},
        {u'k', QChar(0x0646)}, {u'l', QChar(0x0645)}, {u';', QChar(0x0643)}, {u'\'', QChar(0x0637)},
        {u'z', QChar(0x0626)}, {u'x', QChar(0x0621)}, {u'c', QChar(0x0624)}, {u'v', QChar(0x0631)},
        {u'b', QChar(0x0649)}, {u'n', QChar(0x0629)}, {u'm', QChar(0x0648)}, {u',', QChar(0x0632)},
        {u'.', u','},          {u'/', u'.'},          {u'`', QChar(0x0630)}, {u'~', QChar(0x0651)},
        {u' ', u' '},          {u'\n', u'\n'},         {u'\t', u'\t'},
    };
    return entries;
}

// Ported from Switex's `_EN_RU` (config.py).
const std::vector<QPair<QChar, QChar>>& enUsToRuRuEntries() {
    static const std::vector<QPair<QChar, QChar>> entries = {
        {u'q', QChar(0x0439)}, {u'w', QChar(0x0446)}, {u'e', QChar(0x0443)}, {u'r', QChar(0x043a)},
        {u't', QChar(0x0435)}, {u'y', QChar(0x043d)}, {u'u', QChar(0x0433)}, {u'i', QChar(0x0448)},
        {u'o', QChar(0x0449)}, {u'p', QChar(0x0437)}, {u'[', QChar(0x0445)}, {u']', QChar(0x044a)},
        {QChar(0x005c), QChar(0x005c)},
        {u'a', QChar(0x0444)}, {u's', QChar(0x044b)}, {u'd', QChar(0x0432)},
        {u'f', QChar(0x0430)}, {u'g', QChar(0x043f)}, {u'h', QChar(0x0440)}, {u'j', QChar(0x043e)},
        {u'k', QChar(0x043b)}, {u'l', QChar(0x0434)}, {u';', QChar(0x0436)}, {u'\'', QChar(0x044d)},
        {u'z', QChar(0x044f)}, {u'x', QChar(0x0447)}, {u'c', QChar(0x0441)}, {u'v', QChar(0x043c)},
        {u'b', QChar(0x0438)}, {u'n', QChar(0x0442)}, {u'm', QChar(0x044c)}, {u',', QChar(0x0431)},
        {u'.', QChar(0x044e)}, {u'/', u'.'},
        {u'Q', QChar(0x0419)}, {u'W', QChar(0x0426)}, {u'E', QChar(0x0423)}, {u'R', QChar(0x041a)},
        {u'T', QChar(0x0415)}, {u'Y', QChar(0x041d)}, {u'U', QChar(0x0413)}, {u'I', QChar(0x0428)},
        {u'O', QChar(0x0429)}, {u'P', QChar(0x0417)}, {u'{', QChar(0x0425)}, {u'}', QChar(0x042a)},
        {u'|', u'|'},
        {u'A', QChar(0x0424)}, {u'S', QChar(0x042b)}, {u'D', QChar(0x0412)}, {u'F', QChar(0x0410)},
        {u'G', QChar(0x041f)}, {u'H', QChar(0x0420)}, {u'J', QChar(0x041e)}, {u'K', QChar(0x041b)},
        {u'L', QChar(0x0414)}, {u':', QChar(0x0416)}, {u'"', QChar(0x042d)},
        {u'Z', QChar(0x042f)}, {u'X', QChar(0x0427)}, {u'C', QChar(0x0421)}, {u'V', QChar(0x041c)},
        {u'B', QChar(0x0418)}, {u'N', QChar(0x0422)}, {u'M', QChar(0x042c)}, {u'<', QChar(0x0411)},
        {u'>', QChar(0x042e)}, {u'?', u','},
        {u'1', u'1'}, {u'2', u'2'}, {u'3', u'3'}, {u'4', u'4'}, {u'5', u'5'},
        {u'6', u'6'}, {u'7', u'7'}, {u'8', u'8'}, {u'9', u'9'}, {u'0', u'0'},
        {u'-', u'-'}, {u'=', u'='}, {u' ', u' '}, {u'\n', u'\n'}, {u'\t', u'\t'},
    };
    return entries;
}

// Turkish physical-key-position table, replacing an earlier, incorrect
// port of Switex's `_EN_TR` (config.py). Switex's own table there is NOT
// a physical keyboard layout at all — it's built from
// `_zip('abc...xyz', 'abcçdefgğhıi...')`, an alphabetical-position
// substitution (English alphabet's Nth letter -> Turkish alphabet's Nth
// letter) that produces entries with no relationship to any real
// keyboard (e.g. 'e' -> 'd', 'd' -> 'ç'). Per this phase's own
// instruction — Persian (fa-ir) is the one language pinned to Switex's
// exact tables since it's already tested; every other language should
// be corrected against real data where Switex's own map is wrong — this
// table instead reflects the actual Windows "Turkish Q" keyboard layout
// (KBDTUQ.DLL), sourced from Microsoft's own layout data as published at
// http://kbdlayout.info/KBDTUQ/download/klc (retrieved 2026-08-22).
// Covers the 26 letter keys plus the six accented-letter OEM key
// positions (ş, ğ, ü, ö, ç and their capitals) using Windows' standard,
// layout-independent VK_OEM_* key identities (VK_OEM_1=';', VK_OEM_2=
// '/', VK_OEM_4='[', VK_OEM_5='\', VK_OEM_6=']', VK_OEM_7='\'' on
// en-us) — deliberately not attempting the digit-row AltGr symbols
// (Turkish Lira sign, €, dead-key accents) that would need per-modifier
// state this project's flat single-layer char map can't represent
// anyway. Digits/-/=/space are kept as plain identity, same as before.
const std::vector<QPair<QChar, QChar>>& enUsToTrTrEntries() {
    static const std::vector<QPair<QChar, QChar>> entries = {
        {u'q', u'q'}, {u'w', u'w'}, {u'e', u'e'}, {u'r', u'r'}, {u't', u't'},
        {u'y', u'y'}, {u'u', u'u'}, {u'i', QChar(0x0131)}, {u'o', u'o'}, {u'p', u'p'},
        {u'a', u'a'}, {u's', u's'}, {u'd', u'd'}, {u'f', u'f'}, {u'g', u'g'},
        {u'h', u'h'}, {u'j', u'j'}, {u'k', u'k'}, {u'l', u'l'},
        {u'z', u'z'}, {u'x', u'x'}, {u'c', u'c'}, {u'v', u'v'}, {u'b', u'b'},
        {u'n', u'n'}, {u'm', u'm'},
        {u'Q', u'Q'}, {u'W', u'W'}, {u'E', u'E'}, {u'R', u'R'}, {u'T', u'T'},
        {u'Y', u'Y'}, {u'U', u'U'}, {u'I', u'I'}, {u'O', u'O'}, {u'P', u'P'},
        {u'A', u'A'}, {u'S', u'S'}, {u'D', u'D'}, {u'F', u'F'}, {u'G', u'G'},
        {u'H', u'H'}, {u'J', u'J'}, {u'K', u'K'}, {u'L', u'L'},
        {u'Z', u'Z'}, {u'X', u'X'}, {u'C', u'C'}, {u'V', u'V'}, {u'B', u'B'},
        {u'N', u'N'}, {u'M', u'M'},
        // Accented-letter OEM positions (VK_OEM_1/2/4/5/6/7 on en-us):
        {u';', QChar(0x015f)}, {u':', QChar(0x015e)},               // ş / Ş
        {u'[', QChar(0x011f)}, {u'{', QChar(0x011e)},               // ğ / Ğ
        {u']', QChar(0x00fc)}, {u'}', QChar(0x00dc)},               // ü / Ü
        {u'\\', QChar(0x00e7)}, {u'|', QChar(0x00c7)},              // ç / Ç
        {u'/', QChar(0x00f6)}, {u'?', QChar(0x00d6)},               // ö / Ö
        {u'\'', u'i'}, {u'"', QChar(0x0130)},                       // dotted i / İ
        {u'1', u'1'}, {u'2', u'2'}, {u'3', u'3'}, {u'4', u'4'}, {u'5', u'5'},
        {u'6', u'6'}, {u'7', u'7'}, {u'8', u'8'}, {u'9', u'9'}, {u'0', u'0'},
        {u'-', u'-'}, {u'=', u'='}, {u' ', u' '}, {u'\n', u'\n'}, {u'\t', u'\t'},
    };
    return entries;
}

// Standard Israeli "Hebrew" keyboard layout, replacing an earlier,
// incorrect port of Switex's `_EN_HE` (config.py). Switex's own table
// was not a real keyboard layout either: several of its entries mapped
// to Arabic-script characters instead of Hebrew ones, it was missing the
// 'p' key entirely (only 25 of 26 English letters had any entry), and
// even the entries that *were* Hebrew letters didn't match any real
// physical Hebrew keyboard layout. This table instead reflects the
// actual Windows "Hebrew" keyboard layout (KBDHEB.DLL), sourced from
// Microsoft's own layout data as published at
// http://kbdlayout.info/KBDHEB/ (retrieved 2026-08-22), which is also
// consistent with the standard Israeli layout described independently
// by multiple Hebrew-keyboard references. No uppercase (Shift+letter)
// entries are defined — matching Switex's own original scope decision
// here, and correct for Hebrew, which has no letter case; Shift instead
// produces dagesh/vowel-point diacritics this project's flat char map
// isn't attempting to model.
const std::vector<QPair<QChar, QChar>>& enUsToHeIlEntries() {
    static const std::vector<QPair<QChar, QChar>> entries = {
        {u'q', u'/'}, {u'w', u'\''}, {u'e', QChar(0x05e7)}, {u'r', QChar(0x05e8)}, {u't', QChar(0x05d0)},
        {u'y', QChar(0x05d8)}, {u'u', QChar(0x05d5)}, {u'i', QChar(0x05df)}, {u'o', QChar(0x05dd)},
        {u'p', QChar(0x05e4)},
        {u'a', QChar(0x05e9)}, {u's', QChar(0x05d3)}, {u'd', QChar(0x05d2)}, {u'f', QChar(0x05db)},
        {u'g', QChar(0x05e2)}, {u'h', QChar(0x05d9)}, {u'j', QChar(0x05d7)}, {u'k', QChar(0x05dc)},
        {u'l', QChar(0x05da)}, {u';', QChar(0x05e3)},
        {u'z', QChar(0x05d6)}, {u'x', QChar(0x05e1)}, {u'c', QChar(0x05d1)}, {u'v', QChar(0x05d4)},
        {u'b', QChar(0x05e0)}, {u'n', QChar(0x05de)}, {u'm', QChar(0x05e6)},
        {u',', QChar(0x05ea)}, {u'.', QChar(0x05e5)}, {u'/', u'.'},
        {u'1', u'1'}, {u'2', u'2'}, {u'3', u'3'}, {u'4', u'4'}, {u'5', u'5'},
        {u'6', u'6'}, {u'7', u'7'}, {u'8', u'8'}, {u'9', u'9'}, {u'0', u'0'},
        {u' ', u' '}, {u'\n', u'\n'}, {u'\t', u'\t'},
    };
    return entries;
}

CharMap toCharMap(const std::vector<QPair<QChar, QChar>>& entries) {
    CharMap map;
    map.reserve(static_cast<int>(entries.size()));
    for (const auto& entry : entries) {
        map.insert(entry.first, entry.second);
    }
    return map;
}

// Mirrors Switex's own `_rev()` (config.py) exactly: walk the forward
// table in its original order and keep only the *first* key seen for
// each value. Several of the five tables above aren't 1:1 (e.g.
// enUsToHeIlEntries() maps both 'h' and 'y' to U+05D9), so a naive
// QHash-to-QHash reversal would leave the winner undefined; replicating
// the same iteration order and tie-break Switex uses guarantees this
// port behaves identically to it.
CharMap reverseMap(const std::vector<QPair<QChar, QChar>>& entries) {
    CharMap reversed;
    QSet<QChar> seenValues;
    for (const auto& entry : entries) {
        if (seenValues.contains(entry.second)) {
            continue;
        }
        seenValues.insert(entry.second);
        reversed.insert(entry.second, entry.first);
    }
    return reversed;
}

struct LayoutPairKey {
    QString from;
    QString to;

    bool operator==(const LayoutPairKey& other) const { return from == other.from && to == other.to; }
};

size_t qHash(const LayoutPairKey& key, size_t seed = 0) {
    return qHashMulti(seed, key.from, key.to);
}

const QHash<LayoutPairKey, CharMap>& registry() {
    static const QHash<LayoutPairKey, CharMap> maps = [] {
        QHash<LayoutPairKey, CharMap> result;
        // One entry per Switex-ported language pair (§5 Phase 6, `en`
        // being the common pivot on both sides exactly as Switex's own
        // MAPS registry only ever defines `('en', X)` by hand): each
        // adds its forward table plus the automatically-derived reverse
        // — a third language later means one more line here, not new
        // conversion code (§4.4).
        const struct {
            const char* targetLayout;
            const std::vector<QPair<QChar, QChar>>& (*entriesFn)();
        } pairs[] = {
            {"fa-ir", enUsToFaIrEntries}, {"ar-sa", enUsToArSaEntries}, {"ru-ru", enUsToRuRuEntries},
            {"tr-tr", enUsToTrTrEntries}, {"he-il", enUsToHeIlEntries},
        };
        for (const auto& pair : pairs) {
            const QString target = QString::fromLatin1(pair.targetLayout);
            const auto& entries = pair.entriesFn();
            result.insert(LayoutPairKey{QStringLiteral("en-us"), target}, toCharMap(entries));
            result.insert(LayoutPairKey{target, QStringLiteral("en-us")}, reverseMap(entries));
        }
        return result;
    }();
    return maps;
}

} // namespace

const CharMap* findCharMap(const QString& fromLayout, const QString& toLayout) {
    const auto& maps = registry();
    const auto it = maps.constFind(LayoutPairKey{fromLayout, toLayout});
    if (it == maps.constEnd()) {
        return nullptr;
    }
    return &it.value();
}

QStringList supportedLayoutIds() {
    QSet<QString> ids;
    for (auto it = registry().constBegin(); it != registry().constEnd(); ++it) {
        ids.insert(it.key().from);
        ids.insert(it.key().to);
    }
    QStringList result(ids.constBegin(), ids.constEnd());
    result.sort();
    return result;
}

} // namespace lancue::conversion
