#include "follower_feature/follower_feature.h"

#include <QColor>
#include <QCursor>

#include "corelib/eventbus/event.h"
#include "corelib/eventbus/event_bus.h"
#include "corelib/i18n/language.h"
#include "corelib/logging/logger.h"
#include "corelib/settings/settings_schema.h"
#include "corelib/ui/app_theme_mode.h"
#include "corelib/ui/layout_color_palette.h"
#include "follower_feature/follower_window.h"

namespace lancue {

FollowerFeature::FollowerFeature(EventBus& eventBus, settings::SettingsManager& settingsManager)
    : m_eventBus(eventBus), m_settingsManager(settingsManager) {}

FollowerFeature::~FollowerFeature() = default;

bool FollowerFeature::start() {
    m_followerWindow = std::make_unique<ui::FollowerWindow>();

    m_eventBus.subscribe(EventType::LayoutChanged, [this](const Event& event) {
        m_currentLayout = event.data.toString();

        // The very first LayoutChanged is LayoutWatcherFeature's own
        // startup seed (today's active layout, not a real switch) — see
        // ToastFeature's own identical guard.
        if (!m_hasReceivedFirstLayoutEvent) {
            m_hasReceivedFirstLayoutEvent = true;
            return;
        }

        const auto& settings = m_settingsManager.current();
        if (!settings.followerEnabled) {
            return;
        }
        const i18n::AppLanguage language = i18n::fromCode(settings.appLanguage);
        // Both windows read this same field — there's no separate
        // "follower theme" setting — so a dark/light/auto choice always
        // applies to the toast and the follower together, never one
        // dark while the other stays light (Mahdi's own round-2
        // request).
        const ui::AppThemeMode themeMode = ui::fromCode(settings.appThemeMode);
        // Two-letter code (e.g. "EN", "FA"), the same one ToastWindow's
        // own badge already shows — not the full localized display
        // name: always exactly two characters regardless of layout or
        // app language, so FollowerWindow's fixed-width label never has
        // to change size with what it's currently showing (Mahdi's own
        // round-2 request) — see FollowerWindow.h's own comment.
        const QString badgeCode = i18n::layoutBadgeCode(m_currentLayout);

        // A separate field from toastOpacityPercent (kDefaultFollowerOpacityPercent
        // just happens to share the same default *value*, 85) — see
        // that constant's own comment in settings_schema.h.
        const qreal cardOpacity = settings.followerOpacityPercent / 100.0;

        // Same per-layout accent-color lookup ToastFeature::showForCurrentLayout()
        // already does — an explicit swatch for this layout wins, otherwise
        // leave accentOverride invalid so FollowerWindow falls back to the
        // theme's neutral border. Used as the pill's border (not a fill or
        // a stripe, both of which ToastWindow's larger card has room for
        // but this small a pill doesn't) — a colored ring around the
        // whole badge stays legible at any size, doesn't compete with the
        // two-letter text for space, and (unlike a fill) doesn't force a
        // text-contrast decision per accent color.
        QColor accentOverride;
        const auto colorAssignmentIt = settings.layoutColorAssignments.constFind(m_currentLayout);
        if (colorAssignmentIt != settings.layoutColorAssignments.constEnd()) {
            accentOverride = QColor(ui::layoutColorSwatchHex(colorAssignmentIt.value()));
        }

        m_followerWindow->showFollowing(badgeCode, language, QCursor::pos(), settings.followerDurationMs, themeMode,
                                         cardOpacity, accentOverride);
    });

    m_eventBus.subscribe(EventType::MouseMoved, [this](const Event& event) {
        if (!m_followerWindow->isShowingFollower()) {
            return;
        }
        // event.data is already QCursor::pos(), resolved once by
        // MouseWatcherFeature (see that class's own comment) — reused
        // here instead of querying again, so there's exactly one
        // authoritative read of the cursor position per real move.
        m_followerWindow->updatePosition(event.data.toPoint());
    });

    // followerDurationMs live-updates a follower already on screen (a
    // duration change should be visible immediately); followerEnabled=false
    // immediately hides one too — mirrors ToastFeature's own identical
    // SettingsChanged subscription exactly (see that file's own comment,
    // added together after Mahdi's own question about exactly this
    // interaction with a permanent-duration window). Every other setting
    // here (opacity, accent color, theme) only affects the next one
    // shown — re-deriving the whole rendering on an already-visible
    // window isn't worth the complexity for a window this short-lived,
    // whereas restarting a timer or hiding outright are both trivial.
    m_eventBus.subscribe(EventType::SettingsChanged, [this](const Event& event) {
        if (event.data.toString() == QString::fromUtf8(settings::kFieldFollowerDuration)) {
            m_followerWindow->updateDuration(m_settingsManager.current().followerDurationMs);
        } else if (event.data.toString() == QString::fromUtf8(settings::kFieldFollowerEnabled)) {
            if (!m_settingsManager.current().followerEnabled) {
                // Matters most for a permanent-duration follower
                // (followerDurationMs == kFollowerDurationPermanentMs):
                // without this, disabling the follower would do nothing
                // visible until the daemon restarts, since a permanent
                // follower never times out on its own. Turning
                // followerEnabled back on doesn't show anything
                // immediately — same as any other toggle, it only lifts
                // the gate the LayoutChanged handler above already
                // checks for the next real trigger.
                m_followerWindow->hideFollower();
            }
        }
    });

    logInfo(QStringLiteral("FollowerFeature: started."));
    return true;
}

void FollowerFeature::stop() {
    if (m_followerWindow) {
        m_followerWindow->hideFollower();
    }
}

void FollowerFeature::onEvent(const Event& /*event*/) {
    // Subscribes directly in start(), same as ToastFeature — see
    // IFeature::onEvent()'s own comment.
}

} // namespace lancue
