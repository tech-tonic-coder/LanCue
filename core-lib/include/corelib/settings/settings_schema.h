#pragma once

#include <optional>

#include <QHash>
#include <QString>
#include <QVector>

#include <nlohmann/json.hpp>

#include "corelib/platform/IGlobalHotkeyManager.h"

namespace lancue::settings {

// Bump only for a breaking change to the shape of Settings below (a field
// renamed or removed, not just added) — see migrate() in settings_schema.cpp
// for how an older on-disk version gets forward-migrated to this one.
inline constexpr int kCurrentSchemaVersion = 1;

// Every setting has its default defined once, here, per §4.5 — nothing
// reads a magic number where a setting's value is needed.
inline constexpr int kDefaultToastDurationMs = 3000;
inline constexpr int kDefaultFollowerDurationMs = 1500;

// A toastDurationMs of this value means "stay visible until manually
// dismissed" (Ctrl+Alt+Shift+Q / the toast's own close button) instead of
// auto-dismissing after N milliseconds — Phase 7's own requirement that a
// permanently-shown toast must not cost extra idle resources: the toast
// window simply never starts its auto-dismiss QTimer when the configured
// duration is this sentinel, rather than starting one with an
// enormous/infinite timeout (§4.7 — no timer at all beats a timer that
// never fires). Any negative value is treated as this sentinel by the
// toast window, but this is the one callers should write.
inline constexpr int kToastDurationPermanentMs = -1;

// Same sentinel, same reasoning, for the follower (Mahdi's own request:
// "a few seconds up to permanent while the program is running") — the
// follower has no dismiss hotkey/close button of its own, so "permanent"
// here means it stays up, continuously tracking the cursor, until the
// next real layout change (which simply re-shows it, still permanent) or
// the daemon exits. FollowerWindow::startHideTimer() already treats any
// durationMs <= 0 as permanent — this constant is the one callers should
// write, matching kToastDurationPermanentMs's own convention.
inline constexpr int kFollowerDurationPermanentMs = -1;

// Card opacity as a percentage (55-100). Bounds are from actual WCAG
// contrast math, not a guess: 65% is where the dark-theme title text
// (the least forgiving case) crosses below AA contrast (4.5:1) against
// a worst-case white desktop behind the toast; 100% is plain full
// opacity. Default raised from a hardcoded 92% to 85% — still very safe
// on contrast, noticeably more see-through than before.
inline constexpr int kToastOpacityPercentMin = 65;
inline constexpr int kToastOpacityPercentMax = 100;
inline constexpr int kDefaultToastOpacityPercent = 85;

// Phase 8 (round 2): the follower window's own opacity — a separate
// field from kToastOpacityPercent above, not a shared one, per Mahdi's
// own request that each window be independently settable. Same
// min/max and the same default *value* as the toast's own (85) — not
// because the two are linked, but because "start the follower looking
// like the toast, then let the user diverge it" is a reasonable
// starting point, and re-deriving a fresh contrast-safe default from
// scratch for a second, differently-styled window wasn't warranted.
inline constexpr int kFollowerOpacityPercentMin = 65;
inline constexpr int kFollowerOpacityPercentMax = 100;
inline constexpr int kDefaultFollowerOpacityPercent = 85;

// Field-name constants used both as the JSON object key for that field and
// as the EventType::SettingsChanged payload identifying which field just
// changed (see SettingsManager) — one name, two uses, so a rename can't
// desync the wire format from the live-update signal.
inline constexpr const char* kFieldToastDuration = "toastDurationMs";
inline constexpr const char* kFieldToastOpacityPercent = "toastOpacityPercent";
inline constexpr const char* kFieldFollowerDuration = "followerDurationMs";
inline constexpr const char* kFieldFollowerOpacityPercent = "followerOpacityPercent";
inline constexpr const char* kFieldHotkeys = "hotkeys";
inline constexpr const char* kFieldLayoutConversionPairs = "layoutConversionPairs";
// Phase 7: whether a keyboard-layout change should show the toast at all
// ("اگه کاربر فعال کرده" — Mahdi's own spec, 2026-08-28). The manual-show
// hotkey (platform::kToastShowHotkeyId) is deliberately unaffected by
// this flag — it's an explicit user action, not a layout-change reaction,
// so it always shows regardless of whether the automatic trigger is on.
inline constexpr const char* kFieldToastEnabled = "toastEnabled";
// Same idea, for the follower window (added 2026-09-20, alongside the
// immediate-hide-on-disable behavior both features now share — see
// ToastFeature's own SettingsChanged handler and FollowerFeature's own
// identical one). No settings-UI toggle exists yet to set this (Phase 9
// still owns that), but the field, the IPC setter, and the live-hide
// behavior are all real today, reachable the same way toastEnabled
// already is (lancue-debug-cli) — added now rather than left for
// Phase 9 because Mahdi asked the exact behavioral question this answers
// before that UI exists, and there was no reason to make him ask twice.
inline constexpr const char* kFieldFollowerEnabled = "followerEnabled";
// Phase 7: LanCue's own UI language — see i18n/language.h's AppLanguage.
// Stored as its toCode() string ("en"/"fa"), not the enum's numeric
// value, for the same human-readable-on-disk reasoning as every other
// string field in this schema.
inline constexpr const char* kFieldAppLanguage = "appLanguage";
// Phase 7 (round 9, 2026-09-04): where on the target monitor the toast is
// anchored — see corelib/ui/toast_position.h's own doc comment for the
// full 3x3 grid this stores one of (as ui::toCode()'s string output, same
// "richer enum owned by the feature/UI layer, plain string owned by the
// schema" split kFieldAppLanguage already uses for i18n::AppLanguage).
inline constexpr const char* kFieldToastPosition = "toastPosition";
// Phase 7 (round 10, 2026-09-05): which screen(s) the toast targets when
// more than one is connected — see corelib/ui/toast_monitor_mode.h's own
// doc comment for the four modes this stores one of (as ui::toCode()'s
// string output, same split as kFieldToastPosition).
inline constexpr const char* kFieldToastMonitorMode = "toastMonitorMode";
// "auto"/"light"/"dark" — see ui::AppThemeMode. Data/IPC layer only for
// now; the actual toggle is Phase 9's job.
inline constexpr const char* kFieldAppThemeMode = "appThemeMode";
// Only consulted when toastMonitorMode is "specific" — an index into
// QGuiApplication::screens() at the moment the toast is shown. NOT a
// stable monitor identity across reconnects/replugs (Qt doesn't expose
// one portably that survives a monitor being unplugged and replugged in
// a different port) — if the index is out of range when a toast is
// about to show (fewer screens connected now than when this was set),
// ToastFeature falls back to the primary screen rather than failing to
// show anything (see that class's own resolveTargetScreens() comment).
inline constexpr const char* kFieldToastMonitorIndex = "toastMonitorIndex";
// Layout id -> swatch number (corelib/ui/layout_color_palette.h, 1-8).
// Empty is valid — means nothing's been assigned yet.
inline constexpr const char* kFieldLayoutColorAssignments = "layoutColorAssignments";

// A stored HotkeyCombo. platform::HotkeyCombo::toString() is explicitly
// documented as a display string, not a serialization format, so the
// combo is round-tripped here as its two raw Qt enum values instead —
// unambiguous and exact.
struct HotkeyBinding {
    int modifiers = static_cast<int>(Qt::NoModifier);
    int key = static_cast<int>(Qt::Key_unknown);

    platform::HotkeyCombo toCombo() const;
    static HotkeyBinding fromCombo(const platform::HotkeyCombo& combo);

    bool operator==(const HotkeyBinding& other) const {
        return modifiers == other.modifiers && key == other.key;
    }
};

// A pair of normalized layout ids (Phase 2's "xx-yy" scheme) the user
// wants text conversion available between. This is user preference data —
// which layout pairs conversion applies to — not the character-mapping
// tables themselves, which are Phase 6's data tables per §4.4 ("a new
// keyboard-layout pair is a new table, not new code" refers to the
// conversion map, not this list of which pairs are active).
struct ConversionPair {
    QString layoutA;
    QString layoutB;

    bool operator==(const ConversionPair& other) const {
        return layoutA == other.layoutA && layoutB == other.layoutB;
    }
};

// The full versioned settings schema. Every field has a real default
// (below in defaultSettings()), so a freshly-constructed Settings is
// already valid and usable without a load() ever having happened.
struct Settings {
    int version = kCurrentSchemaVersion;
    int toastDurationMs = kDefaultToastDurationMs;
    int toastOpacityPercent = kDefaultToastOpacityPercent;
    int followerDurationMs = kDefaultFollowerDurationMs;
    int followerOpacityPercent = kDefaultFollowerOpacityPercent;
    QHash<QString, HotkeyBinding> hotkeys;
    QVector<ConversionPair> layoutConversionPairs;
    bool toastEnabled = true;
    bool followerEnabled = true;
    // i18n::AppLanguage::toCode()'s output ("en"/"fa"), not the enum
    // itself — see kFieldAppLanguage's own comment on why this schema
    // stores the code string rather than including i18n/language.h here
    // (same "raw serializable value, not the richer type" choice
    // HotkeyBinding already makes for platform::HotkeyCombo).
    QString appLanguage = QStringLiteral("en");
    // ui::toCode(ui::ToastPosition::BottomRight) — matches this feature's
    // original hardcoded position from before positions were
    // configurable (round 9), so an existing settings.json predating this
    // field (parsed via fromJson()'s per-field fallback) keeps showing
    // the toast exactly where it always did.
    QString toastPosition = QStringLiteral("bottom-right");
    // ui::toCode(ui::ToastMonitorMode::CursorScreen) — matches this
    // feature's original single-monitor-targeting behavior from before
    // per-monitor targeting existed (round 10), same backward-
    // compatibility reasoning as toastPosition's own default above.
    QString toastMonitorMode = QStringLiteral("cursor");
    QString appThemeMode = QStringLiteral("auto");
    int toastMonitorIndex = 0;
    // Empty by default — nothing assigned until the picker UI (Phase 9)
    // or default-assignment sets something.
    QHash<QString, int> layoutColorAssignments;
};

// The default hotkey set (today: just GlobalHotkeyFeature's own
// kConvertTextHotkeyId, since no other action exists yet — see the Phase
// 3 Learnings entry) and a default conversion pair (en-us/fa-ir, matching
// §1's own stated Persian/English use case) — a fresh install is usable
// out of the box rather than starting with nothing configured.
Settings defaultSettings();

nlohmann::json toJson(const Settings& settings);

// Lenient per-field parsing: a missing or wrong-typed individual field
// falls back to that field's own default rather than failing the whole
// parse, so a partially-corrupt or older/newer file degrades gracefully
// field-by-field. Returns std::nullopt only when the top-level JSON isn't
// even an object, or "version" is missing/non-integer/greater than
// kCurrentSchemaVersion (a future schema version this binary doesn't
// understand) — SettingsManager treats nullopt as "fall back to
// defaults entirely" (§5 Phase 4: "corrupt/unreadable config file must
// fall back to defaults rather than crashing").
std::optional<Settings> fromJson(const nlohmann::json& json);

} // namespace lancue::settings
