#pragma once

#include <QString>

namespace lancue::conversion {

// Result of a convert() call. `ok` is false only when no character table
// exists at all for the requested (fromLayout, toLayout) pair — a
// per-character miss inside an existing table is not a failure (see
// convert()'s doc comment below).
struct ConversionResult {
    QString text;
    bool ok = false;
};

// Converts `text` from `fromLayout` to `toLayout` one character at a
// time using layout_char_maps::findCharMap(). This is the same
// character-by-character, "unmapped passes through unchanged" behavior
// Switex itself uses (`mapping.get(c, c)` in its `convert()`) — kept
// deliberately identical rather than inventing a different policy, since
// it's already a proven, shipped answer to the ambiguous-character
// question this phase asks (see the Phase 6 Learnings entry):
//   - A character with an explicit entry in the table (including
//     identity entries like '.' -> '.') converts to that entry.
//   - A character with no entry at all (e.g. an emoji, or any symbol
//     outside both layouts) passes through unchanged rather than being
//     dropped or guessed at.
// `fromLayout == toLayout` is a deliberate no-op that returns `text`
// unchanged with `ok = true` (also matching Switex's convert()) rather
// than looking for a nonexistent self-pair table — relevant when a
// caller's source and target happen to resolve to the same layout (e.g.
// a per-language dedicated hotkey pressed on text already in that
// layout).
//
// When neither `fromLayout` nor `toLayout` is "en-us" and no direct
// table exists (e.g. fa-ir -> ar-sa: every ported table only pivots
// through en-us, see layout_char_maps.cpp), this automatically converts
// in two steps, fromLayout -> en-us -> toLayout — ported from Switex's
// own convert(), which does exactly this rather than treating it as a
// failure. Only a genuinely missing table even after that pivot attempt
// (e.g. either side isn't a supported layout at all) leaves `ok = false`.
ConversionResult convert(const QString& text, const QString& fromLayout, const QString& toLayout);

// Content-based best-effort guess of which supported layout `text` was
// actually typed in. Ported from Switex's own `detect_lang()`
// (switex.py): counts each layout's characteristic characters
// (fa/ru/tr/he alphabets, checked in that priority order, with plain
// a-z Latin letters counted toward "en-us") and returns whichever has
// the strictly highest count; a genuine tie resolves in that same
// fa > ru > tr > he > en-us priority order (matching the insertion order
// of Switex's own `counts` dict, which is what its `max()` call
// effectively breaks ties by), and an all-zero count (e.g. text that's
// only digits/punctuation) resolves to "en-us" — all exactly matching
// Switex's own behavior, not a new policy invented here.
//
// Three known quirks inherited as-is from Switex rather than "fixed"
// here (see the Phase 6 Learnings entry for the reasoning):
//   (1) There is no "ar-sa" bucket at all — Switex's own detect_lang()
//       never returns 'ar' under any input, so the dedicated Arabic
//       hotkey's forced target is always reached, but its *detected
//       source* for genuinely Arabic-script text will usually resolve to
//       "fa-ir" instead (most Arabic letters are also in the Persian
//       alphabet this function checks first) rather than ever being
//       "ar-sa" itself. convert()'s en-us pivot (see its own doc
//       comment) still makes the actual character substitution correct
//       in the common case, since fa-ir and ar-sa both ultimately route
//       through en-us — this is a real gap only for the minority of
//       Arabic-only characters absent from the Persian alphabet.
//   (2) Switex checks its Turkish-specific character set *after* the
//       plain a-z check, so ordinary Turkish words typed with unaccented
//       Latin letters mostly count toward "en-us" instead of "tr-tr" —
//       only the accented Turkish letters (ç, ğ, ı, ö, ş, ü, İ) are ever
//       counted as Turkish.
//   (3) Switex's own Hebrew character set includes two Arabic letters
//       (matching the same anomaly already noted on the ported ar/he
//       tables).
//
// Used only by the per-language dedicated hotkeys (§5 Phase 6), which
// need *some* way to pick a source layout despite explicitly ignoring
// the current OS layout — the generic auto-detect hotkey never calls
// this, since its source/target come from matching the current OS
// layout against a configured settings::ConversionPair instead (see
// ConversionFeature).
QString detectLayout(const QString& text);

} // namespace lancue::conversion
