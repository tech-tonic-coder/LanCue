#pragma once

#include <functional>
#include <optional>

#include <QString>

#include "corelib/platform/selection_clipboard_bridge.h"
#include "corelib/settings/settings_manager.h"
#include "feature.h"

namespace lancue {

class EventBus;

// Phase 6's feature: reacts to GlobalHotkeyFeature's HotkeyPressed events
// for the seven conversion hotkeys (platform::kConvertAutoHotkeyId, plus
// one kConvertTo<Layout>HotkeyId per supported language — see
// IGlobalHotkeyManager.h) by driving SelectionClipboardBridge's
// capture/convert/paste flow through conversion::convert(). This is what
// Phase 5's main.cpp comment called "Phase 6 replaces this block with
// real conversion logic" — that temporary diagnostic wiring is gone,
// replaced by this proper IFeature.
//
// Also tracks the OS keyboard layout's last two values (m_previousLayout,
// m_currentLayout) via EventType::LayoutChanged (published both on every
// real switch by LayoutWatcherFeature and once at startup — see that
// feature's own comment) purely to answer the generic hotkey's "what
// layout was this typed in, and what should it become" question; it does
// not own a watcher of its own (§2.2 — one producer per kind of OS
// state).
class ConversionFeature final : public IFeature {
public:
    ConversionFeature(EventBus& eventBus, settings::SettingsManager& settingsManager,
                       platform::SelectionClipboardBridge& selectionBridge);

    bool start() override;
    void stop() override;
    void onEvent(const Event& event) override;

private:
    // Shared by every hotkey handler below: capture the current
    // selection, resolve the source layout from the captured text via
    // `resolveFromLayout` (the generic hotkey already knows it and
    // ignores the text; the per-language hotkeys call
    // conversion::detectLayout() on it). If `resolveFromLayout` returns
    // std::nullopt — meaning "nothing to do" (only the dedicated
    // per-language hotkeys ever return this, when the text is already in
    // the target layout) — this cancels the capture without pasting
    // anything back, leaving the clipboard untouched. Otherwise it
    // converts to `toLayout` and pastes the result; if nothing usable was
    // captured, or convert() itself fails (no table for the pair), it
    // also cancel()s so nothing is left half-done on the clipboard.
    void captureConvertAndPaste(std::function<std::optional<QString>(const QString&)> resolveFromLayout,
                                 const QString& toLayout);

    // The generic auto-detecting hotkey (Ctrl+Alt+Shift+Space by
    // default). Ported from Switex's own Windows-path behavior, not the
    // earlier "match against a configured settings::ConversionPair"
    // design this project's own roadmap had sketched before this phase
    // (see the Phase 6 Learnings entry for why that draft was
    // superseded): the intended workflow is select the mistyped text,
    // switch the OS keyboard layout to the *correct* one, then press this
    // hotkey — so m_previousLayout (before that switch) is the source and
    // m_currentLayout (after it) is the target. If no real switch has
    // been observed yet (m_previousLayout empty or equal to
    // m_currentLayout), there is nothing to infer a direction from, so
    // this does nothing rather than guessing.
    void handleConvertAuto();

    // A dedicated per-language hotkey (one per supported layout: en-us,
    // fa-ir, ar-sa, ru-ru, tr-tr, he-il): no OS layout switch is expected
    // before pressing it. Always targets `targetLayout` regardless of
    // m_currentLayout, using conversion::detectLayout() on the captured
    // text to decide whether anything needs to happen at all — if the
    // text is already in `targetLayout`, this is a deliberate no-op (see
    // captureConvertAndPaste()'s std::nullopt case), not a conversion to
    // itself. Ported from Switex's own dedicated-hotkey behavior
    // (`detect_lang(t)` as source, fixed target) rather than invented
    // here; see the Phase 6 Learnings entry.
    void handleConvertToTarget(const QString& targetLayout);

    EventBus& m_eventBus;
    settings::SettingsManager& m_settingsManager;
    platform::SelectionClipboardBridge& m_selectionBridge;

    // Both empty until at least one LayoutChanged event arrives; on the
    // first one only m_currentLayout is set (see
    // LayoutWatcherFeature::start()'s own comment on the startup seed) —
    // m_previousLayout stays empty until a second, real switch happens.
    // Only the generic hotkey reads either; per-language hotkeys don't
    // need them.
    QString m_previousLayout;
    QString m_currentLayout;
};

} // namespace lancue
