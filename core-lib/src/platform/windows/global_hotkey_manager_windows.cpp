#include "global_hotkey_manager_windows.h"

#include <QCoreApplication>

#include "corelib/logging/logger.h"

namespace lancue::platform::windows {

namespace {

UINT mapModifiersToWinFlags(Qt::KeyboardModifiers modifiers) {
    UINT flags = 0;
    if (modifiers & Qt::ControlModifier) {
        flags |= MOD_CONTROL;
    }
    if (modifiers & Qt::AltModifier) {
        flags |= MOD_ALT;
    }
    if (modifiers & Qt::ShiftModifier) {
        flags |= MOD_SHIFT;
    }
    if (modifiers & Qt::MetaModifier) {
        flags |= MOD_WIN;
    }
    return flags;
}

// Covers the keys a user could realistically be offered for this feature
// (letters, digits, function keys, a handful of named keys) rather than
// every Qt::Key value — same scope decision as HotkeyCombo::keyName()'s
// own comment. Returns 0 (an unused/invalid Windows virtual-key value)
// for anything outside that set, which registerHotkey() treats as
// "unsupported key" and refuses rather than passing 0 through to
// RegisterHotKey.
UINT mapKeyToVirtualKey(Qt::Key key) {
    // Qt::Key_A..Z and Qt::Key_0..9 are defined as their ASCII codes,
    // which are numerically identical to the Windows virtual-key codes
    // for the same characters (VK_A..VK_Z, VK_0..VK_9) — no lookup table
    // needed for these two ranges.
    if ((key >= Qt::Key_A && key <= Qt::Key_Z) || (key >= Qt::Key_0 && key <= Qt::Key_9)) {
        return static_cast<UINT>(key);
    }
    if (key >= Qt::Key_F1 && key <= Qt::Key_F24) {
        return VK_F1 + static_cast<UINT>(key - Qt::Key_F1);
    }
    switch (key) {
        case Qt::Key_Space:
            return VK_SPACE;
        case Qt::Key_Escape:
            return VK_ESCAPE;
        case Qt::Key_Tab:
            return VK_TAB;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            return VK_RETURN;
        case Qt::Key_QuoteLeft:
            return VK_OEM_3;
        default:
            return 0;
    }
}

} // namespace

GlobalHotkeyManagerWindows::GlobalHotkeyManagerWindows() = default;

GlobalHotkeyManagerWindows::~GlobalHotkeyManagerWindows() {
    stop();
}

void GlobalHotkeyManagerWindows::setCallback(HotkeyPressedCallback callback) {
    m_callback = std::move(callback);
}

bool GlobalHotkeyManagerWindows::registerHotkey(const HotkeyId& id, const HotkeyCombo& combo) {
    if (!m_filterInstalled) {
        // Installed lazily on first use rather than in the constructor:
        // QCoreApplication::instance() doesn't exist yet if this class is
        // ever constructed before the app object (it isn't today, but
        // this makes that assumption load-bearing nowhere).
        QCoreApplication::instance()->installNativeEventFilter(this);
        m_filterInstalled = true;
    }

    const UINT vk = mapKeyToVirtualKey(combo.key);
    if (vk == 0) {
        lancue::logError(
            QStringLiteral("GlobalHotkeyManagerWindows: '%1' (id='%2') uses a key this mapping doesn't support.")
                .arg(combo.toString(), id));
        return false;
    }

    // MOD_NOREPEAT: without it, holding the combo down re-fires WM_HOTKEY
    // at the OS key-repeat rate, which is never what "press this hotkey"
    // should mean for a one-shot trigger.
    const UINT modFlags = mapModifiersToWinFlags(combo.modifiers) | MOD_NOREPEAT;

    // Re-registering this same id: allocate a *new* Windows-side id for
    // the new combo and only unregister+drop the old one once the new
    // RegisterHotKey call has actually succeeded. Registering first (with
    // a fresh id, so it can't collide with the one about to be replaced)
    // and only then releasing the old one means a failed re-registration
    // leaves the previous combo for this id still working, per this
    // method's own contract, rather than leaving `id` briefly or
    // permanently unregistered.
    const int newWinId = m_nextWinId++;
    if (!RegisterHotKey(nullptr, newWinId, modFlags, vk)) {
        const DWORD error = GetLastError();
        // ERROR_HOTKEY_ALREADY_REGISTERED (1409) is the expected/common
        // case this phase's requirements call out explicitly; logged with
        // the raw error code regardless so an unexpected failure mode is
        // still diagnosable rather than looking identical to a conflict.
        lancue::logError(
            QStringLiteral("GlobalHotkeyManagerWindows: RegisterHotKey('%1', id='%2') failed (error=%3).")
                .arg(combo.toString(), id)
                .arg(error));
        return false;
    }

    const auto previousWinId = m_idToWinId.find(id);
    if (previousWinId != m_idToWinId.end()) {
        UnregisterHotKey(nullptr, previousWinId.value());
        m_winIdToId.remove(previousWinId.value());
    }

    m_idToWinId[id] = newWinId;
    m_winIdToId[newWinId] = id;
    return true;
}

void GlobalHotkeyManagerWindows::unregisterHotkey(const HotkeyId& id) {
    const auto it = m_idToWinId.find(id);
    if (it == m_idToWinId.end()) {
        return;
    }
    UnregisterHotKey(nullptr, it.value());
    m_winIdToId.remove(it.value());
    m_idToWinId.erase(it);
}

void GlobalHotkeyManagerWindows::stop() {
    for (auto it = m_winIdToId.constBegin(); it != m_winIdToId.constEnd(); ++it) {
        UnregisterHotKey(nullptr, it.key());
    }
    m_winIdToId.clear();
    m_idToWinId.clear();

    if (m_filterInstalled) {
        if (auto* app = QCoreApplication::instance()) {
            app->removeNativeEventFilter(this);
        }
        m_filterInstalled = false;
    }
}

bool GlobalHotkeyManagerWindows::nativeEventFilter(const QByteArray& eventType, void* message, qintptr* /*result*/) {
    if (eventType != "windows_generic_MSG") {
        return false;
    }
    const auto* msg = static_cast<const MSG*>(message);
    if (msg->message == WM_HOTKEY) {
        const auto it = m_winIdToId.constFind(static_cast<int>(msg->wParam));
        if (it != m_winIdToId.constEnd() && m_callback) {
            m_callback(it.value());
        }
    }
    // Never consumed: nothing else in this process is expected to act on
    // WM_HOTKEY, but there's no reason to stop Qt's own dispatcher from
    // also seeing it.
    return false;
}

} // namespace lancue::platform::windows

namespace lancue::platform {

std::unique_ptr<IGlobalHotkeyManager> createGlobalHotkeyManager() {
    return std::make_unique<windows::GlobalHotkeyManagerWindows>();
}

} // namespace lancue::platform
