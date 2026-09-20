#pragma once

#include <QString>

namespace lancue::i18n {

// LanCue's own UI language — separate from a keyboard layout id
// (Phase 2's "en-us"/"fa-ir"). Defaults to English on a fresh install.
enum class AppLanguage {
    English,
    Persian,
};

// "en"/"fa" — persisted as Settings::appLanguage.
QString toCode(AppLanguage language);

// Parses toCode()'s output; falls back to `fallback` if unrecognized.
AppLanguage fromCode(const QString& code, AppLanguage fallback = AppLanguage::English);

enum class LayoutDirection {
    LeftToRight,
    RightToLeft,
};

// Which direction LanCue's own UI should render in for `language`.
LayoutDirection directionFor(AppLanguage language);

// Human-readable name of a keyboard layout id ("en-us" -> "English
// (US)"), localized into `language`. An empty id (layout genuinely
// couldn't be determined) becomes "Unknown"/"نامشخص"; an id we don't
// have a name for falls back to the id itself, title-cased.
QString layoutDisplayName(const QString& normalizedLayoutId, AppLanguage language);

// The toast badge's 2-letter code, e.g. "en-us" -> "EN". Just the id's
// own first two letters uppercased — every layout id already starts
// with the code you'd expect, so no lookup table needed. Empty/unknown
// id falls back to "LC".
QString layoutBadgeCode(const QString& normalizedLayoutId);

// The toast's fixed subtitle ("Keyboard layout changed"), in the app's
// own UI language — not the layout that just became active.
QString toastLayoutChangedText(AppLanguage language);

} // namespace lancue::i18n
