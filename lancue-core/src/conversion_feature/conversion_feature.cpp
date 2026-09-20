#include "conversion_feature.h"

#include <functional>

#include "corelib/conversion/conversion_engine.h"
#include "corelib/eventbus/event.h"
#include "corelib/eventbus/event_bus.h"
#include "corelib/logging/logger.h"
#include "corelib/platform/IGlobalHotkeyManager.h"

namespace lancue {

ConversionFeature::ConversionFeature(EventBus& eventBus, settings::SettingsManager& settingsManager,
                                      platform::SelectionClipboardBridge& selectionBridge)
    : m_eventBus(eventBus), m_settingsManager(settingsManager), m_selectionBridge(selectionBridge) {}

bool ConversionFeature::start() {
    m_eventBus.subscribe(EventType::LayoutChanged, [this](const Event& event) {
        m_previousLayout = m_currentLayout;
        m_currentLayout = event.data.toString();
    });

    m_eventBus.subscribe(EventType::HotkeyPressed, [this](const Event& event) {
        const QString id = event.data.toString();
        if (id == platform::kConvertAutoHotkeyId) {
            handleConvertAuto();
        } else if (id == platform::kConvertToEnUsHotkeyId) {
            handleConvertToTarget(QStringLiteral("en-us"));
        } else if (id == platform::kConvertToFaIrHotkeyId) {
            handleConvertToTarget(QStringLiteral("fa-ir"));
        } else if (id == platform::kConvertToArSaHotkeyId) {
            handleConvertToTarget(QStringLiteral("ar-sa"));
        } else if (id == platform::kConvertToRuRuHotkeyId) {
            handleConvertToTarget(QStringLiteral("ru-ru"));
        } else if (id == platform::kConvertToTrTrHotkeyId) {
            handleConvertToTarget(QStringLiteral("tr-tr"));
        } else if (id == platform::kConvertToHeIlHotkeyId) {
            handleConvertToTarget(QStringLiteral("he-il"));
        }
        // Any other id (none exist yet) is silently not this feature's
        // concern, same as GlobalHotkeyFeature's own onEvent() pattern.
    });

    return true;
}

void ConversionFeature::stop() {
    // Nothing to release: this feature owns no OS handle of its own —
    // just EventBus subscriptions, which (same as GlobalHotkeyFeature)
    // live for the daemon's whole lifetime rather than being torn down
    // here.
}

void ConversionFeature::onEvent(const Event& /*event*/) {
    // This feature subscribes directly in start() instead (see the
    // class comment on IFeature::onEvent() for why that's the documented
    // alternative path — same choice GlobalHotkeyFeature already made).
}

void ConversionFeature::captureConvertAndPaste(std::function<std::optional<QString>(const QString&)> resolveFromLayout,
                                                const QString& toLayout) {
    m_selectionBridge.captureSelection(
        platform::SelectionClipboardBridge::kDefaultCaptureTimeoutMs,
        [this, resolveFromLayout = std::move(resolveFromLayout), toLayout](
            platform::SelectionClipboardBridge::CaptureResult result) {
            if (!result.success) {
                logWarning(QStringLiteral("ConversionFeature: capture failed or nothing was selected."));
                m_selectionBridge.cancel();
                return;
            }

            const std::optional<QString> fromLayout = resolveFromLayout(result.text);
            if (!fromLayout.has_value()) {
                // Only reached from handleConvertToTarget(): the
                // captured text is already in the hotkey's target
                // layout, so §5 Phase 6's own spec applies — "nothing
                // happens" — rather than pasting identical text back.
                // Phase 7 hook: this is where a toast/system
                // notification saying so would fire once Phase 7's toast
                // window exists; not mandatory per this phase's own
                // instructions, so just logged for now.
                logInfo(QStringLiteral("ConversionFeature: selection is already in target layout '%1'; nothing to "
                                        "convert.")
                            .arg(toLayout));
                m_selectionBridge.cancel();
                return;
            }

            const conversion::ConversionResult converted = conversion::convert(result.text, *fromLayout, toLayout);
            if (!converted.ok) {
                logWarning(QStringLiteral("ConversionFeature: no character map for '%1' -> '%2'; nothing pasted.")
                               .arg(*fromLayout, toLayout));
                m_selectionBridge.cancel();
                return;
            }

            m_selectionBridge.pasteReplacement(converted.text);
        });
}

void ConversionFeature::handleConvertAuto() {
    // Ported from Switex's own Windows-path behavior for this hotkey
    // (switex.py's `convert_by_system_layout()`), not the "match against
    // a configured settings::ConversionPair" design this project's own
    // roadmap had sketched in an earlier phase — that draft is
    // superseded; see the Phase 6 Learnings entry for why matching
    // Switex's actual proven behavior was chosen instead once it came
    // under scrutiny.
    //
    // The intended workflow: select the mistyped text, switch the OS
    // keyboard layout to the *correct* one, then press this hotkey. At
    // that moment m_currentLayout is the destination (the layout just
    // switched to) and m_previousLayout is where the garbled text
    // actually came from — so this converts m_previousLayout ->
    // m_currentLayout, unconditionally, using whatever the last observed
    // transition was. If no real switch has been observed yet
    // (m_previousLayout empty, or equal to m_currentLayout because
    // LayoutChanged has only ever fired once — the startup seed, see
    // LayoutWatcherFeature::start()) there is no transition to convert
    // along, so this does nothing rather than guessing a direction.
    if (m_previousLayout.isEmpty() || m_previousLayout == m_currentLayout) {
        // Nothing captured yet at this point — the clipboard hasn't been
        // touched, so there's nothing to cancel()/restore, just a log so
        // this isn't a silent no-op a user would be confused by.
        logWarning(QStringLiteral("ConversionFeature: no observed layout switch yet (previous='%1', current='%2'); "
                                   "convertAuto hotkey ignored.")
                       .arg(m_previousLayout.isEmpty() ? QStringLiteral("(none)") : m_previousLayout,
                            m_currentLayout.isEmpty() ? QStringLiteral("(none)") : m_currentLayout));
        return;
    }

    const QString fromLayout = m_previousLayout;
    const QString toLayout = m_currentLayout;
    captureConvertAndPaste([fromLayout](const QString&) -> std::optional<QString> { return fromLayout; }, toLayout);
}

void ConversionFeature::handleConvertToTarget(const QString& targetLayout) {
    // Per-language dedicated hotkeys deliberately ignore m_currentLayout
    // and m_previousLayout (§5 Phase 6: no OS layout switch is expected
    // before pressing one of these) — so, unlike handleConvertAuto(),
    // there's no layout transition to read a source from. Ported from
    // Switex's own behavior for these exact hotkeys (switex.py's
    // per-language entries in its `hotkeys` dict all use `detect_lang(t)`
    // as the source with a fixed target) rather than inventing a
    // different rule here — see conversion::detectLayout() and the Phase
    // 6 Learnings entry.
    //
    // Returning std::nullopt when the detected source already equals the
    // target tells captureConvertAndPaste() this is the "text is already
    // in the target layout, do nothing" case §5 Phase 6 specifies,
    // rather than a normal (and here pointless) same-layout convert()
    // call.
    captureConvertAndPaste(
        [targetLayout](const QString& text) -> std::optional<QString> {
            const QString detected = conversion::detectLayout(text);
            if (detected == targetLayout) {
                return std::nullopt;
            }
            return detected;
        },
        targetLayout);
}

} // namespace lancue
