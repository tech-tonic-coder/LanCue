#include "corelib/ui/layout_color_palette.h"

namespace lancue::ui {

namespace {

// Matches the roadmap's palette table, 1..8 in order.
constexpr const char* kSwatchHex[kLayoutColorSwatchCount] = {
    "#534AB7", // 1
    "#185FA5", // 2
    "#0F6E56", // 3
    "#3B6D11", // 4
    "#854F0B", // 5
    "#993C1D", // 6
    "#A32D2D", // 7
    "#993556", // 8
};

} // namespace

bool isValidLayoutColorSwatch(int swatchIndex) {
    return swatchIndex >= kLayoutColorSwatchMin && swatchIndex <= kLayoutColorSwatchMax;
}

QString layoutColorSwatchHex(int swatchIndex) {
    int index = swatchIndex;
    if (!isValidLayoutColorSwatch(index)) {
        // Wrap into range instead of asserting.
        const int zeroBased = index - kLayoutColorSwatchMin;
        const int wrapped = ((zeroBased % kLayoutColorSwatchCount) + kLayoutColorSwatchCount) % kLayoutColorSwatchCount;
        index = wrapped + kLayoutColorSwatchMin;
    }
    return QString::fromUtf8(kSwatchHex[index - kLayoutColorSwatchMin]);
}

} // namespace lancue::ui
