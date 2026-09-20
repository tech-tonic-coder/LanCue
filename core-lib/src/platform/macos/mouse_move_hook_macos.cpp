#include "mouse_move_hook_macos.h"

#include <QCoreApplication>
#include <QSemaphore>
#include <QString>

#include "corelib/logging/logger.h"

namespace lancue::platform::macos {

MouseMoveHookMacos::MouseMoveHookMacos() = default;

MouseMoveHookMacos::~MouseMoveHookMacos() {
    stop();
}

void MouseMoveHookMacos::setCallback(MouseMovedCallback callback) {
    m_callback = std::move(callback);
}

bool MouseMoveHookMacos::start() {
    QSemaphore ready(0);
    bool succeeded = false;

    m_thread = std::thread([this, &ready, &succeeded]() {
        const CGEventMask mask = CGEventMaskBit(kCGEventMouseMoved);
        m_tap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionListenOnly, mask,
                                  &MouseMoveHookMacos::handleEvent, this);
        if (!m_tap) {
            lancue::logError(QStringLiteral(
                "MouseMoveHookMacos: CGEventTapCreate failed — Input Monitoring permission may not be granted yet "
                "(System Settings > Privacy & Security > Input Monitoring)."));
            succeeded = false;
            ready.release();
            return;
        }

        m_runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, m_tap, 0);
        m_runLoop = CFRunLoopGetCurrent();
        CFRunLoopAddSource(m_runLoop, m_runLoopSource, kCFRunLoopCommonModes);
        CGEventTapEnable(m_tap, true);

        succeeded = true;
        ready.release();

        // Blocks this thread until stop() calls CFRunLoopStop(m_runLoop)
        // below — mirrors MouseHookThread::run()'s own exec() call on
        // Windows, just via CoreFoundation's own run-loop primitive
        // instead of Qt's, since a CGEventTap's run-loop source must be
        // pumped by an actual CFRunLoopRun() on the thread that added
        // it, which QThread::exec() does not do (that pumps Qt's own
        // dispatcher, a completely different mechanism — see
        // IMouseMoveHook.h's own comment on why each platform manages
        // its own loop internally rather than sharing one external
        // wrapper).
        CFRunLoopRun();

        CFRunLoopRemoveSource(m_runLoop, m_runLoopSource, kCFRunLoopCommonModes);
        CFRelease(m_runLoopSource);
        m_runLoopSource = nullptr;
        CGEventTapEnable(m_tap, false);
        CFRelease(m_tap);
        m_tap = nullptr;
        m_runLoop = nullptr;
    });

    ready.acquire();
    if (!succeeded) {
        m_thread.join();
        return false;
    }

    lancue::logInfo(QStringLiteral("MouseMoveHookMacos: CGEventTap installed (listen-only)."));
    return true;
}

void MouseMoveHookMacos::stop() {
    if (m_thread.joinable()) {
        if (m_runLoop) {
            CFRunLoopStop(m_runLoop);
        }
        m_thread.join();
    }
}

CGEventRef MouseMoveHookMacos::handleEvent(CGEventTapProxy /*proxy*/, CGEventType type, CGEventRef event,
                                            void* refcon) {
    if (type == kCGEventMouseMoved) {
        // Marshal back onto the main/Qt thread before touching the
        // registered callback at all — same reasoning as
        // MouseMoveHookWindows::lowLevelMouseProc's own hand-off (see
        // that file's comment): this fires on the dedicated thread
        // start() spawned above, never the main thread, and
        // IMouseMoveHook.h's own contract promises callers their
        // callback always runs on the thread that called start().
        auto* instance = static_cast<MouseMoveHookMacos*>(refcon);
        QMetaObject::invokeMethod(
            qApp,
            [instance]() {
                if (instance->m_callback) {
                    instance->m_callback();
                }
            },
            Qt::QueuedConnection);
    }
    // Listen-only: the return value is ignored by the OS either way —
    // returned only for signature correctness (CGEventTapCallBack always
    // returns a CGEventRef), not because it has any effect here.
    return event;
}

} // namespace lancue::platform::macos

namespace lancue::platform {

std::unique_ptr<IMouseMoveHook> createMouseMoveHook() {
    return std::make_unique<macos::MouseMoveHookMacos>();
}

} // namespace lancue::platform
