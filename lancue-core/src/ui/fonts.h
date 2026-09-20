#pragma once

#include <QString>

#include "corelib/i18n/language.h"

namespace lancue::ui {

// Loads every embedded font file under the ":/fonts" resource prefix
// (resources/fonts/fonts.qrc) into the application's font database via
// QFontDatabase::addApplicationFont(). Must be called once, early in
// main() (before any widget that uses titleFontFamily()/bodyFontFamily()
// below is constructed) — this is what makes LanCue's typography correct
// on a fresh machine regardless of whether Manrope/Vazirmatn happen to be
// installed system-wide (Mahdi's own requirement, 2026-08-28), so a
// failed load is logged loudly rather than silently falling through to
// whatever generic sans-serif the OS substitutes.
//
// Lives in lancue-core (not core-lib) because QFontDatabase is a QtGui
// type and core-lib deliberately stays off QtGui (see hotkey_combo.cpp's
// own comment) — this is lancue-core's own concern until a second
// executable (lancue-settings, Phase 9) also needs it, at which point
// promoting this file into core-lib is the right move (§4.3 — don't
// generalize for a hypothetical second caller that doesn't exist yet).
void loadApplicationFonts();

// The exact font-family string to hand to QFont for LanCue's title-weight
// text (SemiBold) in `language`. Hardcoded to the specific family names
// these bundled .ttf files actually expose in their own name tables
// (confirmed against the real files under resources/fonts/ — Google
// Fonts' static distribution of a variable family gives each non-
// regular/non-bold weight its own family name, e.g. "Manrope SemiBold"
// rather than a "Manrope" family with a SemiBold *style* selectable via
// QFont::setWeight() — verified 2026-08-28, see the Phase 7 Learnings
// entry). If these exact files are ever swapped for a differently-named
// build of the same fonts, Qt's own font matching falls back to the
// nearest installed substitute rather than failing outright — graceful
// degradation, not a hard dependency on the string matching exactly.
QString titleFontFamily(i18n::AppLanguage language);

// The regular-weight family name for `language`'s body/subtitle text —
// see titleFontFamily()'s own comment for why this is a literal string
// rather than a family+weight pair.
QString bodyFontFamily(i18n::AppLanguage language);

} // namespace lancue::ui
