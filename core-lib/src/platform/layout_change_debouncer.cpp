#include "corelib/platform/layout_change_debouncer.h"

namespace lancue::platform {

LayoutChangeDebouncer::LayoutChangeDebouncer(int debounceMs) {
    m_timer.setSingleShot(true);
    m_timer.setInterval(debounceMs);
    QObject::connect(&m_timer, &QTimer::timeout, &m_timer, [this] {
        if (m_pendingLayoutId == m_lastSettledLayoutId) {
            return;
        }
        m_lastSettledLayoutId = m_pendingLayoutId;
        if (m_callback) {
            m_callback(m_lastSettledLayoutId);
        }
    });
}

void LayoutChangeDebouncer::setSettledCallback(SettledCallback callback) {
    m_callback = std::move(callback);
}

void LayoutChangeDebouncer::onRawChange(const QString& layoutId) {
    m_pendingLayoutId = layoutId;
    m_timer.start(); // restarts the window if it's already running
}

} // namespace lancue::platform
