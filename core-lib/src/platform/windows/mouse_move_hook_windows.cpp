#include "mouse_move_hook_windows.h"

#include <functional>
#include <utility>

#include <QCoreApplication>
#include <QSemaphore>
#include <QString>
#include <QThread>

#include "corelib/logging/logger.h"

namespace lancue::platform::windows {

namespace {

// Runs the install/pump/uninstall sequence WH_MOUSE_LL needs on its own
// dedicated thread — mirrors layout_watcher_feature.cpp's
// LayoutHookThread almost exactly (same reasoning: WH_MOUSE_LL's
// callback runs nested inside whatever thread's own message retrieval
// installed it, so a small dedicated thread whose only job is pumping
// that hook keeps it from ever nesting inside the *main* thread's own
// dispatcher). Owned entirely inside this .cpp (MouseMoveHookWindows
// itself only ever touches it through the plain QThread* start()/stop()
// lifecycle in the header) — see IMouseMoveHook.h's own comment on why
// this is encapsulated in the platform implementation rather than
// pushed up into a Feature the way Phase 2's LayoutHookThread is: this
// exact wrapper only makes sense on Windows, since macOS/Linux need
// their own native run-loop pumping mechanism, not a Qt QThread::exec()
// one (see those platforms' own implementation files).
//
// Takes the actual SetWindowsHookExW/UnhookWindowsHookEx calls as
// functors supplied by MouseMoveHookWindows::start() rather than making
// them itself: this class lives in this .cpp's own anonymous namespace,
// not as a member of MouseMoveHookWindows, so it has no access to that
// class's private lowLevelMouseProc (a real link error caught by this
// project's own MinGW sandbox cross-compile check — see the roadmap's
// Phase 8 entry). The functors themselves are ordinary lambdas written
// inside MouseMoveHookWindows::start(), a member function, so *they*
// have the access this class doesn't need to.
class MouseHookThread : public QThread {
public:
    MouseHookThread(std::function<HHOOK()> installHook, std::function<void(HHOOK)> uninstallHook,
                     QSemaphore& readySemaphore, bool& outStartSucceeded)
        : m_installHook(std::move(installHook)),
          m_uninstallHook(std::move(uninstallHook)),
          m_readySemaphore(readySemaphore),
          m_outStartSucceeded(outStartSucceeded) {}

protected:
    void run() override {
        m_hook = m_installHook();
        m_outStartSucceeded = m_hook != nullptr;
        m_readySemaphore.release();

        if (!m_outStartSucceeded) {
            return;
        }

        exec();

        m_uninstallHook(m_hook);
        m_hook = nullptr;
    }

private:
    std::function<HHOOK()> m_installHook;
    std::function<void(HHOOK)> m_uninstallHook;
    QSemaphore& m_readySemaphore;
    bool& m_outStartSucceeded;
    HHOOK m_hook = nullptr;
};

} // namespace

MouseMoveHookWindows* MouseMoveHookWindows::s_activeInstance = nullptr;

MouseMoveHookWindows::MouseMoveHookWindows() = default;

MouseMoveHookWindows::~MouseMoveHookWindows() {
    stop();
}

void MouseMoveHookWindows::setCallback(MouseMovedCallback callback) {
    m_callback = std::move(callback);
}

bool MouseMoveHookWindows::start() {
    if (s_activeInstance) {
        lancue::logError(
            QStringLiteral("MouseMoveHookWindows: start() called while another instance's hook is already active."));
        return false;
    }

    QSemaphore hookReady(0);
    bool hookStartSucceeded = false;
    // Both lambdas are written here, inside this member function, so
    // they carry this scope's access to the private lowLevelMouseProc
    // below — see MouseHookThread's own comment on why it can't reach
    // that member itself.
    auto* thread = new MouseHookThread(
        []() -> HHOOK {
            HHOOK hook = SetWindowsHookExW(WH_MOUSE_LL, &MouseMoveHookWindows::lowLevelMouseProc,
                                            GetModuleHandleW(nullptr), 0);
            if (!hook) {
                lancue::logError(QStringLiteral(
                                      "MouseMoveHookWindows: SetWindowsHookExW(WH_MOUSE_LL) failed (error=%1).")
                                      .arg(GetLastError()));
            }
            return hook;
        },
        [](HHOOK hook) { UnhookWindowsHookEx(hook); }, hookReady, hookStartSucceeded);
    m_hookThread = thread;
    thread->start();
    // Sub-millisecond in practice — a single SetWindowsHookExW call, same
    // reasoning as LayoutWatcherFeature::start()'s own identical wait.
    hookReady.acquire();

    if (!hookStartSucceeded) {
        thread->quit();
        thread->wait();
        delete m_hookThread;
        m_hookThread = nullptr;
        return false;
    }

    s_activeInstance = this;
    lancue::logInfo(QStringLiteral("MouseMoveHookWindows: low-level mouse hook installed."));
    return true;
}

void MouseMoveHookWindows::stop() {
    if (m_hookThread) {
        m_hookThread->quit();
        m_hookThread->wait();
        delete m_hookThread;
        m_hookThread = nullptr;
    }
    if (s_activeInstance == this) {
        s_activeInstance = nullptr;
    }
}

LRESULT CALLBACK MouseMoveHookWindows::lowLevelMouseProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION && s_activeInstance && wParam == WM_MOUSEMOVE) {
        // Marshal back onto the main/Qt thread before touching the
        // registered callback at all — mirrors
        // LayoutWatcherFeature::start()'s own hand-off exactly, and for
        // the same reason: this hook callback runs nested inside the
        // dedicated hook thread's own message dispatch (never the main
        // thread — see MouseHookThread's own comment), and
        // IMouseMoveHook.h's own contract promises callers their
        // callback always runs on the thread that called start() (the
        // main thread, per main.cpp), regardless of which thread the OS
        // itself delivers this hook on.
        auto* instance = s_activeInstance;
        QMetaObject::invokeMethod(
            qApp,
            [instance]() {
                if (instance->m_callback) {
                    instance->m_callback();
                }
            },
            Qt::QueuedConnection);
    }
    return CallNextHookEx(nullptr, code, wParam, lParam);
}

} // namespace lancue::platform::windows

namespace lancue::platform {

std::unique_ptr<IMouseMoveHook> createMouseMoveHook() {
    return std::make_unique<windows::MouseMoveHookWindows>();
}

} // namespace lancue::platform

