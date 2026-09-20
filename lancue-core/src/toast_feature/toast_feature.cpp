#include "toast_feature/toast_feature.h"

#include <algorithm>

#include <QColor>
#include <QCursor>
#include <QGuiApplication>
#include <QScreen>

#include "corelib/eventbus/event.h"
#include "corelib/eventbus/event_bus.h"
#include "corelib/i18n/language.h"
#include "corelib/logging/logger.h"
#include "corelib/platform/IGlobalHotkeyManager.h"
#include "corelib/ui/app_theme_mode.h"
#include "corelib/ui/layout_color_palette.h"
#include "corelib/ui/toast_monitor_mode.h"
#include "corelib/ui/toast_position.h"
#include "toast_feature/toast_window.h"

namespace lancue {

ToastFeature::ToastFeature(EventBus& eventBus, settings::SettingsManager& settingsManager)
    : m_eventBus(eventBus), m_settingsManager(settingsManager) {}

ToastFeature::~ToastFeature() = default;

bool ToastFeature::start() {
    // How many windows we need depends on toastMonitorMode and how many
    // screens exist right now — that's resolved in showForCurrentLayout(),
    // not here.

    m_eventBus.subscribe(EventType::LayoutChanged, [this](const Event& event) {
        m_currentLayout = event.data.toString();

        if (!m_hasReceivedFirstLayoutEvent) {
            // Just the startup seed — don't show a toast for it.
            m_hasReceivedFirstLayoutEvent = true;
            return;
        }

        if (m_settingsManager.current().toastEnabled) {
            showForCurrentLayout();
        }
    });

    m_eventBus.subscribe(EventType::HotkeyPressed, [this](const Event& event) {
        const QString id = event.data.toString();
        if (id == platform::kToastDismissHotkeyId) {
            for (const auto& window : m_toastWindows) {
                window->dismiss();
            }
        } else if (id == platform::kToastShowHotkeyId) {
            // Don't stack a second toast on an already-showing one.
            const bool anyShowing =
                std::any_of(m_toastWindows.begin(), m_toastWindows.end(),
                            [](const auto& window) { return window->isShowingToast(); });
            if (!anyShowing) {
                showForCurrentLayout();
            }
        }
    });

    // toastDurationMs live-updates a toast already on screen (a duration
    // change should be visible immediately, e.g. in a live settings
    // preview); toastEnabled=false immediately dismisses one too — see
    // the reasoning inline below, added after Mahdi's own question about
    // exactly this interaction. Every other setting here only affects
    // the next toast shown.
    m_eventBus.subscribe(EventType::SettingsChanged, [this](const Event& event) {
        if (event.data.toString() == QString::fromUtf8(settings::kFieldToastDuration)) {
            for (const auto& window : m_toastWindows) {
                window->updateDuration(m_settingsManager.current().toastDurationMs);
            }
        } else if (event.data.toString() == QString::fromUtf8(settings::kFieldToastEnabled)) {
            if (!m_settingsManager.current().toastEnabled) {
                // Immediately hides an already-visible toast the moment
                // the user turns notifications off, rather than only
                // gating the *next* one (which the toastEnabled check in
                // the LayoutChanged handler above already does on its
                // own). This matters most for a permanent-duration toast
                // (toastDurationMs == kToastDurationPermanentMs): without
                // this, disabling toasts would do nothing visible until
                // the daemon restarts, since a permanent toast never
                // times out on its own. Turning toastEnabled back on
                // doesn't show anything immediately — same as any other
                // toggle, it only lifts the gate for the next real
                // trigger.
                for (const auto& window : m_toastWindows) {
                    window->dismiss();
                }
            }
        }
    });

    return true;
}

void ToastFeature::stop() {
    for (const auto& window : m_toastWindows) {
        window->dismiss();
    }
}

void ToastFeature::onEvent(const Event& /*event*/) {
    // Subscribes directly in start() instead — see IFeature::onEvent().
}

std::vector<QScreen*> ToastFeature::resolveTargetScreens() const {
    const auto& settings = m_settingsManager.current();
    // Passing the fallback explicitly: with both toast_position.h and
    // toast_monitor_mode.h included here, a bare fromCode() call is
    // ambiguous between their two overloads.
    const ui::ToastMonitorMode mode = ui::fromCode(settings.toastMonitorMode, ui::ToastMonitorMode::CursorScreen);

    QScreen* const primary = QGuiApplication::primaryScreen();

    switch (mode) {
        case ui::ToastMonitorMode::All: {
            const QList<QScreen*> screens = QGuiApplication::screens();
            std::vector<QScreen*> result(screens.begin(), screens.end());
            if (result.empty() && primary) {
                result.push_back(primary);
            }
            return result;
        }
        case ui::ToastMonitorMode::Primary:
            return primary ? std::vector<QScreen*>{primary} : std::vector<QScreen*>{};
        case ui::ToastMonitorMode::Specific: {
            const QList<QScreen*> screens = QGuiApplication::screens();
            if (settings.toastMonitorIndex >= 0 && settings.toastMonitorIndex < screens.size()) {
                return {screens.at(settings.toastMonitorIndex)};
            }
            // Index doesn't match a connected screen anymore — fall back
            // to primary instead of showing nothing.
            logInfo(QStringLiteral("ToastFeature: toastMonitorIndex %1 is out of range for %2 connected "
                                    "screen(s); falling back to the primary screen.")
                        .arg(settings.toastMonitorIndex)
                        .arg(screens.size()));
            return primary ? std::vector<QScreen*>{primary} : std::vector<QScreen*>{};
        }
        case ui::ToastMonitorMode::CursorScreen:
        default: {
            QScreen* screen = QGuiApplication::screenAt(QCursor::pos());
            if (!screen) {
                screen = primary;
            }
            return screen ? std::vector<QScreen*>{screen} : std::vector<QScreen*>{};
        }
    }
}

void ToastFeature::ensureWindowCount(size_t count) {
    while (m_toastWindows.size() < count) {
        m_toastWindows.push_back(std::make_unique<ui::ToastWindow>());
    }
    while (m_toastWindows.size() > count) {
        // dismiss() starts a fade-out, but destroying it right after
        // skips that — acceptable since this only happens between shows
        // (e.g. a monitor got unplugged), never mid-show.
        m_toastWindows.back()->dismiss();
        m_toastWindows.pop_back();
    }
}

void ToastFeature::showForCurrentLayout() {
    const auto& settings = m_settingsManager.current();
    const i18n::AppLanguage language = i18n::fromCode(settings.appLanguage);
    const QString title = i18n::layoutDisplayName(m_currentLayout, language);
    const ui::ToastPosition position = ui::fromCode(settings.toastPosition, ui::ToastPosition::BottomRight);

    // An explicit swatch for this layout wins; otherwise leave
    // accentOverride invalid so ToastWindow falls back to the theme's
    // default accent.
    QColor accentOverride;
    const auto colorAssignmentIt = settings.layoutColorAssignments.constFind(m_currentLayout);
    if (colorAssignmentIt != settings.layoutColorAssignments.constEnd()) {
        accentOverride = QColor(ui::layoutColorSwatchHex(colorAssignmentIt.value()));
    }

    const std::vector<QScreen*> screens = resolveTargetScreens();
    ensureWindowCount(screens.size());

    const qreal cardOpacity = settings.toastOpacityPercent / 100.0;
    const ui::AppThemeMode themeMode = ui::fromCode(settings.appThemeMode, ui::AppThemeMode::Auto);

    for (size_t i = 0; i < screens.size(); ++i) {
        m_toastWindows[i]->showToast(title, m_currentLayout, language, settings.toastDurationMs, screens[i], position,
                                      accentOverride, cardOpacity, themeMode);
    }
}

} // namespace lancue
