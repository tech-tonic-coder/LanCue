#pragma once

#include <QString>

namespace lancue::ui {

// Fixed 8-color palette for per-layout badge/stripe coding. Fixed
// rather than a free color picker because every swatch here is already
// checked for contrast with white text; a user-picked color can't be.
//
// 1-indexed (1-8), matching the numbering in the roadmap's palette
// table, so a swatch number means the same thing in settings.json, an
// IPC payload, and the docs.
//
// Returns plain hex strings, not QColor — core-lib doesn't link Qt::Gui.
inline constexpr int kLayoutColorSwatchMin = 1;
inline constexpr int kLayoutColorSwatchMax = 8;
inline constexpr int kLayoutColorSwatchCount = kLayoutColorSwatchMax - kLayoutColorSwatchMin + 1;

// True for a real swatch number (1-8).
bool isValidLayoutColorSwatch(int swatchIndex);

// Hex for `swatchIndex`. Out-of-range wraps into range instead of
// asserting — real validation belongs at the IPC boundary
// (isValidLayoutColorSwatch()), not here.
QString layoutColorSwatchHex(int swatchIndex);

} // namespace lancue::ui
