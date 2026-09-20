#pragma once

#include <QString>
#include <QVector>

#include "corelib/eventbus/event_bus.h"
#include "corelib/settings/settings_schema.h"

namespace lancue::settings {

// Owns the one in-memory Settings instance lancue-core runs with, backed
// by a versioned JSON file at a per-OS config location. Per §2.2
// ("settings changes preview live, persist on demand"): every mutator
// below updates the in-memory copy and publishes EventType::SettingsChanged
// immediately (field name in the payload) so a running IFeature can react
// without restarting — but the on-disk file is only ever written by
// save(), never by a mutator, so a live-preview tweak never touches disk
// on its own (§5 Phase 4's own verification step for this).
//
// Not a QObject: nothing here needs Qt's signal/slot mechanism of its own
// — mutators publish directly through the EventBus reference they're
// given — so this stays a plain class, consistent with Phase 2/3's own
// non-QObject platform types, and needs no AUTOMOC entry.
class SettingsManager {
public:
    // `overrideFilePath`, when non-empty, is used verbatim instead of the
    // real per-OS config location — exists solely so tests can exercise
    // load()/save() against a throwaway temp file instead of the
    // developer's actual settings.json (§4.3: justified by the concrete
    // need to unit-test load/save without touching real user state, not
    // speculative). Production code (main.cpp) always uses the
    // single-argument constructor.
    explicit SettingsManager(EventBus& eventBus, QString overrideFilePath = QString());

    // Reads the settings file at filePath() and replaces the in-memory
    // Settings with what's there. On any failure to read or parse it
    // (missing file, invalid JSON, a "version" field this binary can't
    // understand) falls back to defaultSettings() and logs why, rather
    // than crashing the daemon on startup (§5 Phase 4 requirement) — a
    // missing file specifically (first run) logs at info level, not as a
    // warning, since it isn't actually a problem.
    void load();

    // Writes the current in-memory Settings to filePath(), creating the
    // containing directory if needed. Returns false (and logs) if the
    // write itself fails (e.g. permissions) — the in-memory state is
    // unaffected either way.
    bool save();

    const Settings& current() const { return m_current; }

    void setToastDurationMs(int ms);
    // `percent` is assumed already validated by the caller (55-100).
    void setToastOpacityPercent(int percent);
    void setFollowerDurationMs(int ms);
    // `percent` is assumed already validated by the caller (65-100) —
    // mirrors setToastOpacityPercent()'s own contract exactly.
    void setFollowerOpacityPercent(int percent);

    // Registers or replaces the combo stored under `id`. Doesn't touch
    // the platform hotkey manager itself — GlobalHotkeyFeature subscribes
    // to the SettingsChanged event this publishes and reconciles its own
    // registrations from current().hotkeys, keeping "what's configured"
    // (here) and "what's actually claimed with the OS" (GlobalHotkeyFeature)
    // as two separate concerns per §2.2's platform-isolation rule.
    void setHotkey(const QString& id, const platform::HotkeyCombo& combo);
    void removeHotkey(const QString& id);

    // Replaces the entire list — there's no per-pair add/remove message,
    // since Phase 9's settings UI is expected to edit the whole list as
    // one form and submit it, not manage pairs one IPC round-trip at a
    // time.
    void setLayoutConversionPairs(const QVector<ConversionPair>& pairs);

    // Phase 7: same live-preview-then-persist-on-Save contract as every
    // mutator above — ToastFeature (lancue-core) reacts to the
    // SettingsChanged this publishes rather than being told directly, so
    // a change from any source (IPC today; Phase 9's settings UI later)
    // reaches the toast the same way.
    void setToastEnabled(bool enabled);
    void setFollowerEnabled(bool enabled);
    void setAppLanguage(const QString& languageCode);
    // Phase 7 (round 9): same live-preview-then-persist-on-Save contract
    // as setAppLanguage above. `positionCode` is stored verbatim (already
    // validated against ui::toCode()'s known set by the IPC handler
    // calling this, the same division of responsibility
    // setAppLanguage/set_app_language_handler.cpp already uses).
    void setToastPosition(const QString& positionCode);
    // Phase 7 (round 10): same live-preview-then-persist-on-Save contract.
    // Two separate mutators/fields (not one combined "monitor selection"
    // message) — toastMonitorIndex is meaningless outside "specific"
    // mode, but keeping it its own field/message matches every other
    // field in this schema being independently settable, and avoids
    // needing a combined payload shape for the one mode that uses both.
    void setToastMonitorMode(const QString& modeCode);
    void setAppThemeMode(const QString& modeCode);
    void setToastMonitorIndex(int index);

    // Assigns one layout's badge/stripe swatch (per-layout, not a
    // whole-map replace). `swatchIndex` is assumed already validated by
    // the caller.
    void setLayoutColorAssignment(const QString& layoutId, int swatchIndex);

private:
    QString filePath() const;
    void publishChanged(const QString& field);

    EventBus& m_eventBus;
    Settings m_current;
    QString m_overrideFilePath;
};

} // namespace lancue::settings
