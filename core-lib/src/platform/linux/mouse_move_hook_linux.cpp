#include "mouse_move_hook_linux.h"

// Qt headers must come before Xlib.h, not after: Xlib.h #defines bare
// macros (None, Bool, Status, True, False, Success, Complex...) that
// collide with real identifiers Qt's own headers declare (e.g.
// Qt::None, QMetaObject-related "Status" spellings) once expanded in
// the wrong order — confirmed by actually hitting this while building
// this file (a real, well-known X11/Qt header-ordering issue, not
// specific to this project). Keeping Qt's includes first sidesteps it
// entirely, no #undef juggling needed.
#include <QCoreApplication>
#include <QSemaphore>
#include <QString>
#include <QtGlobal>

#include <poll.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>

#include "corelib/logging/logger.h"

namespace lancue::platform::linux_ {

MouseMoveHookLinux::MouseMoveHookLinux() = default;

MouseMoveHookLinux::~MouseMoveHookLinux() {
    stop();
}

void MouseMoveHookLinux::setCallback(MouseMovedCallback callback) {
    m_callback = std::move(callback);
}

bool MouseMoveHookLinux::start() {
    if (pipe(m_wakeupPipe) != 0) {
        lancue::logError(QStringLiteral("MouseMoveHookLinux: pipe() failed for the shutdown wakeup pipe."));
        return false;
    }

    QSemaphore ready(0);
    bool succeeded = false;

    m_thread = std::thread([this, &ready, &succeeded]() {
        // A dedicated Xlib connection, opened and used only from this
        // thread — Qt's own XCB connection (the QPA platform plugin) is
        // a completely separate connection to the same X server; mixing
        // Xlib event handling into Qt's own XCB event loop would need
        // XInitThreads()-level coordination for no real benefit here, so
        // this class simply owns its own, isolated connection instead
        // (same "each platform owns whatever it privately needs"
        // reasoning as the Windows/macOS implementations' own dedicated
        // threads).
        Display* display = XOpenDisplay(nullptr);
        if (!display) {
            const bool looksLikeWayland = qEnvironmentVariableIsSet("WAYLAND_DISPLAY");
            lancue::logError(
                QStringLiteral("MouseMoveHookLinux: XOpenDisplay() failed — no X11 (or XWayland compatibility) "
                                "display available.%1")
                    .arg(looksLikeWayland ? QStringLiteral(" This looks like a pure-Wayland session with no "
                                                            "XWayland — see this class's own header comment.")
                                          : QString()));
            succeeded = false;
            ready.release();
            return;
        }

        if (qEnvironmentVariableIsSet("WAYLAND_DISPLAY")) {
            lancue::logWarning(QStringLiteral(
                "MouseMoveHookLinux: running under Wayland via XWayland — mouse tracking may only see "
                "XWayland-mapped windows/cursor, not the full native-Wayland desktop. See this class's own header "
                "comment."));
        }

        int xiOpcode = 0;
        int firstEvent = 0;
        int firstError = 0;
        if (!XQueryExtension(display, "XInputExtension", &xiOpcode, &firstEvent, &firstError)) {
            lancue::logError(QStringLiteral("MouseMoveHookLinux: the X server has no XInput extension."));
            XCloseDisplay(display);
            succeeded = false;
            ready.release();
            return;
        }

        int major = 2;
        int minor = 0;
        if (XIQueryVersion(display, &major, &minor) != Success) {
            lancue::logError(
                QStringLiteral("MouseMoveHookLinux: XIQueryVersion failed — server doesn't support XInput2."));
            XCloseDisplay(display);
            succeeded = false;
            ready.release();
            return;
        }

        unsigned char mask[XIMaskLen(XI_RawMotion)] = {};
        XISetMask(mask, XI_RawMotion);
        XIEventMask eventMask;
        eventMask.deviceid = XIAllMasterDevices;
        eventMask.mask_len = sizeof(mask);
        eventMask.mask = mask;
        XISelectEvents(display, DefaultRootWindow(display), &eventMask, 1);
        XFlush(display);

        succeeded = true;
        ready.release();

        const int xFd = ConnectionNumber(display);
        struct pollfd fds[2];
        fds[0].fd = xFd;
        fds[0].events = POLLIN;
        fds[0].revents = 0;
        fds[1].fd = m_wakeupPipe[0];
        fds[1].events = POLLIN;
        fds[1].revents = 0;

        // poll() blocked on the X connection's own fd (plus the shutdown
        // self-pipe) is an OS wait on a real event source, not a §4.7
        // polling loop — the loop only ever wakes when the X server
        // actually has something for us or stop() has signaled
        // shutdown, exactly like QThread::exec()/CFRunLoopRun() waiting
        // on their own platform's event source do for the other two
        // implementations. There's no fixed-interval timeout driving
        // this call (-1: blocks indefinitely).
        while (true) {
            const int rc = poll(fds, 2, -1);
            if (rc < 0) {
                break;
            }
            if (fds[1].revents & POLLIN) {
                break; // stop() wrote to the wakeup pipe.
            }
            if (fds[0].revents & POLLIN) {
                while (XPending(display) > 0) {
                    XEvent event;
                    XNextEvent(display, &event);
                    XGenericEventCookie* cookie = &event.xcookie;
                    if (cookie->type == GenericEvent && cookie->extension == xiOpcode &&
                        XGetEventData(display, cookie)) {
                        if (cookie->evtype == XI_RawMotion) {
                            // Marshal back onto the main/Qt thread before
                            // touching the registered callback at all —
                            // same reasoning as the Windows/macOS
                            // implementations' own hand-off (see those
                            // files' comments): IMouseMoveHook.h's own
                            // contract promises callers their callback
                            // always runs on the thread that called
                            // start().
                            QMetaObject::invokeMethod(
                                qApp,
                                [this]() {
                                    if (m_callback) {
                                        m_callback();
                                    }
                                },
                                Qt::QueuedConnection);
                        }
                        XFreeEventData(display, cookie);
                    }
                }
            }
        }

        XCloseDisplay(display);
    });

    ready.acquire();
    if (!succeeded) {
        m_thread.join();
        close(m_wakeupPipe[0]);
        close(m_wakeupPipe[1]);
        m_wakeupPipe[0] = -1;
        m_wakeupPipe[1] = -1;
        return false;
    }

    lancue::logInfo(QStringLiteral("MouseMoveHookLinux: XInput2 raw-motion selection installed."));
    return true;
}

void MouseMoveHookLinux::stop() {
    if (m_thread.joinable()) {
        const char byte = 0;
        if (m_wakeupPipe[1] >= 0) {
            [[maybe_unused]] const ssize_t written = write(m_wakeupPipe[1], &byte, 1);
        }
        m_thread.join();
    }
    if (m_wakeupPipe[0] >= 0) {
        close(m_wakeupPipe[0]);
        close(m_wakeupPipe[1]);
        m_wakeupPipe[0] = -1;
        m_wakeupPipe[1] = -1;
    }
}

} // namespace lancue::platform::linux_

namespace lancue::platform {

std::unique_ptr<IMouseMoveHook> createMouseMoveHook() {
    return std::make_unique<linux_::MouseMoveHookLinux>();
}

} // namespace lancue::platform
