#include "corelib/platform/IGlobalHotkeyManager.h"

#include <QChar>
#include <QStringList>

namespace lancue::platform {

namespace {

// Deliberately hand-rolled rather than QKeySequence::toString(): that
// class lives in QtGui, and core-lib only links Qt6::Core/Network for
// lancue-core's headless daemon (§2.3) — pulling in all of QtGui just for
// a log/debug string isn't worth the extra dependency surface. This only
// needs to cover the keys GlobalHotkeyFeature/settings will realistically
// let a user pick, not be a complete Qt::Key enumerator.
QString keyName(Qt::Key key) {
    // QChar(int) was removed in Qt6 (ambiguity with implicit conversions
    // — Qt::Key implicitly converts to int/uint, not directly to
    // QChar), so the code point must be narrowed to char16_t explicitly
    // rather than relying on a QChar(Qt::Key) conversion that no longer
    // exists.
    if (key >= Qt::Key_A && key <= Qt::Key_Z) {
        return QString(QChar(static_cast<char16_t>(key)));
    }
    if (key >= Qt::Key_0 && key <= Qt::Key_9) {
        return QString(QChar(static_cast<char16_t>(key)));
    }
    if (key >= Qt::Key_F1 && key <= Qt::Key_F35) {
        return QStringLiteral("F%1").arg(key - Qt::Key_F1 + 1);
    }
    switch (key) {
        case Qt::Key_Space:
            return QStringLiteral("Space");
        case Qt::Key_Escape:
            return QStringLiteral("Esc");
        case Qt::Key_Tab:
            return QStringLiteral("Tab");
        case Qt::Key_Return:
        case Qt::Key_Enter:
            return QStringLiteral("Enter");
        case Qt::Key_QuoteLeft:
            return QStringLiteral("`");
        default:
            return QStringLiteral("Key(0x%1)").arg(static_cast<uint>(key), 0, 16);
    }
}

} // namespace

QString HotkeyCombo::toString() const {
    QStringList parts;
    if (modifiers & Qt::ControlModifier) {
        parts << QStringLiteral("Ctrl");
    }
    if (modifiers & Qt::AltModifier) {
        parts << QStringLiteral("Alt");
    }
    if (modifiers & Qt::ShiftModifier) {
        parts << QStringLiteral("Shift");
    }
    if (modifiers & Qt::MetaModifier) {
        parts << QStringLiteral("Win");
    }
    parts << keyName(key);
    return parts.join(QStringLiteral("+"));
}

} // namespace lancue::platform
