#include "clipboard_controller_windows.h"

#include <QElapsedTimer>

#include "corelib/logging/logger.h"

namespace lancue::platform::windows {

namespace {

// One dedicated window class for the hidden listener window, registered
// lazily on first use (mirrors GlobalHotkeyManagerWindows's lazy
// native-event-filter install — no reason to touch the Win32 window
// subsystem before this controller is actually started). RegisterClassW
// itself is idempotent-safe to call more than once per process only if
// each call uses a distinct name or is guaranteed to run once; guarded
// here with a static local bool so a second ClipboardControllerWindows
// instance (there is only ever one in this project today, but nothing
// stops a future caller from constructing a second) doesn't attempt a
// duplicate registration.
const wchar_t* const kWindowClassName = L"LanCueClipboardListenerWindow";

void ensureWindowClassRegistered() {
    static bool registered = false;
    if (registered) {
        return;
    }

    WNDCLASSW wc = {};
    wc.lpfnWndProc = ClipboardControllerWindows::wndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = kWindowClassName;
    RegisterClassW(&wc);
    registered = true;
}

// OpenClipboard can transiently fail with ERROR_ACCESS_DENIED (5) whenever
// another process is briefly holding the clipboard open — a clipboard
// manager, Windows' own Clipboard History (Win+V), cloud clipboard sync,
// an antivirus/DLP hook that watches the clipboard, or even the very
// target application this project just told to copy, if its own
// OpenClipboard/SetClipboardData/CloseClipboard sequence in response to
// the simulated Ctrl+C hasn't fully finished yet by the time this runs.
// This is expected, routine behavior on a real Windows machine, not an
// edge case — it's the single most commonly documented gotcha for any
// Win32 clipboard code, and a bounded retry loop is the standard,
// well-established mitigation every serious clipboard utility uses for
// exactly this reason. A previous version of this code called
// OpenClipboard exactly once with no retry at all, which is what caused
// real, repeated conversion failures on Mahdi's own machine (2026-08-27).
//
// Parameters (500ms total window, 10ms between attempts) are not
// arbitrary — they match pyperclip's own Windows clipboard backend
// exactly (`init_windows_clipboard()`'s `clipboard()` context manager in
// pyperclip/__init__.py), the library Switex itself relies on for every
// clipboard read/write. Switex was never immune to this failure mode;
// it solved it the identical way, just through that dependency instead
// of writing the retry loop itself. Matching pyperclip's own
// field-tested window (rather than inventing a shorter one) is a
// deliberate choice, confirmed against pyperclip 1.11.0's actual source
// after this exact question came up (2026-08-27) — a materially shorter
// window is exactly the kind of change that would silently reintroduce
// this bug under slightly worse contention than whatever prompted the
// original fix.
constexpr int kOpenClipboardRetryWindowMs = 500;
constexpr int kOpenClipboardRetryDelayMs = 10;

bool openClipboardWithRetry() {
    QElapsedTimer deadline;
    deadline.start();
    while (deadline.elapsed() < kOpenClipboardRetryWindowMs) {
        if (OpenClipboard(nullptr)) {
            return true;
        }
        Sleep(kOpenClipboardRetryDelayMs);
    }
    return false;
}

} // namespace

ClipboardControllerWindows::ClipboardControllerWindows() = default;

ClipboardControllerWindows::~ClipboardControllerWindows() {
    stop();
}

void ClipboardControllerWindows::setCallback(ClipboardChangedCallback callback) {
    m_callback = std::move(callback);
}

bool ClipboardControllerWindows::start() {
    ensureWindowClassRegistered();

    if (!m_hwnd) {
        // HWND_MESSAGE: a message-only window, invisible and never
        // enumerated by FindWindow/EnumWindows — exactly what's needed
        // here, since this window exists purely to receive
        // WM_CLIPBOARDUPDATE, not to be shown or interacted with.
        m_hwnd = CreateWindowExW(0, kWindowClassName, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr,
                                  GetModuleHandleW(nullptr), nullptr);
        if (!m_hwnd) {
            lancue::logError(QStringLiteral("ClipboardControllerWindows: failed to create the listener window "
                                             "(error=%1).")
                                  .arg(GetLastError()));
            return false;
        }
        // Lets the static wndProc() below recover `this` from the HWND
        // Windows hands it — the standard pattern for routing a C-style
        // WndProc callback back to a specific instance.
        SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    }

    if (!m_listenerInstalled) {
        if (!AddClipboardFormatListener(m_hwnd)) {
            lancue::logError(QStringLiteral("ClipboardControllerWindows: AddClipboardFormatListener failed "
                                             "(error=%1).")
                                  .arg(GetLastError()));
            return false;
        }
        m_listenerInstalled = true;
    }

    return true;
}

void ClipboardControllerWindows::stop() {
    if (m_listenerInstalled && m_hwnd) {
        RemoveClipboardFormatListener(m_hwnd);
        m_listenerInstalled = false;
    }
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

bool ClipboardControllerWindows::hasText() const {
    return IsClipboardFormatAvailable(CF_UNICODETEXT) != 0;
}

std::optional<QString> ClipboardControllerWindows::readText() const {
    if (!openClipboardWithRetry()) {
        lancue::logError(QStringLiteral("ClipboardControllerWindows: OpenClipboard failed while reading after "
                                         "%1ms of retrying (error=%2).")
                              .arg(kOpenClipboardRetryWindowMs)
                              .arg(GetLastError()));
        return std::nullopt;
    }

    QString text;
    HANDLE data = GetClipboardData(CF_UNICODETEXT);
    if (data) {
        // GlobalLock's returned pointer is only valid while locked and
        // only guaranteed null-terminated because CF_UNICODETEXT's own
        // contract requires it — QString::fromWCharArray with an
        // explicit length would also work, but the contract makes the
        // simpler null-terminated constructor safe here.
        const wchar_t* rawText = static_cast<const wchar_t*>(GlobalLock(data));
        if (rawText) {
            text = QString::fromWCharArray(rawText);
            GlobalUnlock(data);
        }
    }

    CloseClipboard();
    return text;
}

bool ClipboardControllerWindows::writeText(const QString& text) {
    if (!openClipboardWithRetry()) {
        lancue::logError(QStringLiteral("ClipboardControllerWindows: OpenClipboard failed while writing after "
                                         "%1ms of retrying (error=%2).")
                              .arg(kOpenClipboardRetryWindowMs)
                              .arg(GetLastError()));
        return false;
    }

    EmptyClipboard();

    const std::wstring wideText = text.toStdWString();
    const size_t byteSize = (wideText.size() + 1) * sizeof(wchar_t);

    HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, byteSize);
    if (!handle) {
        lancue::logError(QStringLiteral("ClipboardControllerWindows: GlobalAlloc failed while writing "
                                         "(error=%1).")
                              .arg(GetLastError()));
        CloseClipboard();
        return false;
    }

    void* dest = GlobalLock(handle);
    if (dest) {
        memcpy(dest, wideText.c_str(), byteSize);
        GlobalUnlock(handle);
        // Ownership of `handle` transfers to the system on a successful
        // SetClipboardData — it must not be freed here regardless of the
        // call's outcome (freeing it on failure would double-free
        // whatever the system already claimed, per the documented Win32
        // contract for this call).
        SetClipboardData(CF_UNICODETEXT, handle);
    } else {
        lancue::logError(QStringLiteral("ClipboardControllerWindows: GlobalLock failed while writing (error=%1).")
                              .arg(GetLastError()));
        GlobalFree(handle);
        CloseClipboard();
        return false;
    }

    CloseClipboard();
    return true;
}

LRESULT CALLBACK ClipboardControllerWindows::wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_CLIPBOARDUPDATE) {
        auto* self = reinterpret_cast<ClipboardControllerWindows*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (self && self->m_callback) {
            self->m_callback();
        }
        return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

} // namespace lancue::platform::windows

namespace lancue::platform {

std::unique_ptr<IClipboardController> createClipboardController() {
    return std::make_unique<windows::ClipboardControllerWindows>();
}

} // namespace lancue::platform
