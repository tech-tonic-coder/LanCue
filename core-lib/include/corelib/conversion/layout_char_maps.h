#pragma once

#include <QChar>
#include <QHash>
#include <QString>
#include <QStringList>

namespace lancue::conversion {

// A single character-to-character table for converting text typed in one
// keyboard layout into what it should read as if it had been typed in
// another. Keyed by QChar so it works directly against QString iteration.
using CharMap = QHash<QChar, QChar>;

// Returns the table that converts a character typed in `fromLayout` into
// its `toLayout` equivalent, or nullptr if this build has no table for
// that exact ordered pair. Layout ids use Phase 2's normalized scheme
// (lowercase "xx-yy", e.g. "en-us", "fa-ir").
//
// Five pairs exist, all pivoting through en-us exactly as Switex's own
// MAPS registry does (§5 Phase 6, ported from config.py): en-us/fa-ir,
// en-us/ar-sa, en-us/ru-ru, en-us/tr-tr, en-us/he-il — each in both
// directions. There is no direct fa-ir/ar-sa table (etc.) for the same
// reason Switex has none either: every non-English pair only ever
// converts through English. `fa_leg` (Legacy Persian) was deliberately
// not ported — see the Phase 6 Learnings entry. Adding a further
// Switex-supported layout later means adding a new table here, not new
// conversion code (§4.4: layout-conversion pairs are data, not code).
const CharMap* findCharMap(const QString& fromLayout, const QString& toLayout);

// All layout ids reachable by at least one table findCharMap() can
// return, deduplicated. Lets a future settings UI (Phase 9) populate a
// layout picker without hardcoding language names here.
QStringList supportedLayoutIds();

} // namespace lancue::conversion
