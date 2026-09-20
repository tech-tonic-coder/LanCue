#include "corelib/settings/settings_schema.h"

#include "corelib/ui/app_theme_mode.h"
#include "corelib/ui/layout_color_palette.h"
#include "corelib/ui/toast_monitor_mode.h"
#include "corelib/ui/toast_position.h"

namespace lancue::settings {

platform::HotkeyCombo HotkeyBinding::toCombo() const {
    return platform::HotkeyCombo{static_cast<Qt::KeyboardModifiers>(modifiers), static_cast<Qt::Key>(key)};
}

HotkeyBinding HotkeyBinding::fromCombo(const platform::HotkeyCombo& combo) {
    HotkeyBinding binding;
    binding.modifiers = static_cast<int>(combo.modifiers);
    binding.key = static_cast<int>(combo.key);
    return binding;
}

Settings defaultSettings() {
    Settings settings;
    // Phase 6's multi-hotkey scheme (§5 Phase 6, ported from Switex):
    // one generic auto-detecting hotkey plus one dedicated hotkey per
    // target layout. Replaces Phase 3's single placeholder "convertText"
    // id — repurposed into "convertAuto" rather than kept alongside it,
    // since it was always meant to become the real conversion hotkey and
    // no other id/combo referenced it. All six dedicated hotkeys are
    // registered by default (matching Switex's own daemon, which
    // registers all of its per-language hotkeys unconditionally too) —
    // each is fully standalone, detecting its own source via
    // conversion::detectLayout() (see ConversionFeature), unlike the
    // generic hotkey below, which reads the OS layout-switch history
    // instead and doesn't consult layoutConversionPairs at all.
    settings.hotkeys.insert(platform::kConvertAutoHotkeyId,
                             HotkeyBinding::fromCombo(platform::kDefaultConvertAutoHotkeyCombo));
    settings.hotkeys.insert(platform::kConvertToEnUsHotkeyId,
                             HotkeyBinding::fromCombo(platform::kDefaultConvertToEnUsHotkeyCombo));
    settings.hotkeys.insert(platform::kConvertToFaIrHotkeyId,
                             HotkeyBinding::fromCombo(platform::kDefaultConvertToFaIrHotkeyCombo));
    settings.hotkeys.insert(platform::kConvertToArSaHotkeyId,
                             HotkeyBinding::fromCombo(platform::kDefaultConvertToArSaHotkeyCombo));
    settings.hotkeys.insert(platform::kConvertToRuRuHotkeyId,
                             HotkeyBinding::fromCombo(platform::kDefaultConvertToRuRuHotkeyCombo));
    settings.hotkeys.insert(platform::kConvertToTrTrHotkeyId,
                             HotkeyBinding::fromCombo(platform::kDefaultConvertToTrTrHotkeyCombo));
    settings.hotkeys.insert(platform::kConvertToHeIlHotkeyId,
                             HotkeyBinding::fromCombo(platform::kDefaultConvertToHeIlHotkeyCombo));
    // NOTE: ConversionFeature's generic hotkey no longer reads
    // layoutConversionPairs as of this phase — it reads the OS layout
    // *transition* (previous -> current) instead, matching Switex's own
    // proven behavior more closely than the configured-pair design an
    // earlier draft of this phase had sketched (see the Phase 6
    // Learnings entry). This field is kept in the schema — still
    // round-tripped through settings.json and the SetLayoutConversion-
    // Pairs IPC handler — since Phase 9's settings UI may still want it
    // for other purposes (e.g. driving which layout pairs a picker
    // offers), just not consulted by convertAuto's own logic anymore.
    settings.layoutConversionPairs.push_back(ConversionPair{QStringLiteral("en-us"), QStringLiteral("fa-ir")});

    // Phase 7: the toast defaults on (a fresh install should demonstrate
    // its own headline feature without extra setup) and the app UI
    // language defaults to English regardless of OS locale (§ this
    // phase's own requirement — see i18n/language.h's own comment on
    // why no locale-detection exists). Dedicated hotkeys for dismissing
    // (Ctrl+Alt+Shift+Q) and manually showing (Ctrl+Alt+Shift+L) the
    // toast are registered by default the same way Phase 6's six
    // conversion hotkeys are — see the Phase 7 Learnings entry for why
    // Q/L were chosen over the Phase 6 letter-per-language scheme's own
    // taken letters.
    settings.toastEnabled = true;
    settings.followerEnabled = true;
    settings.appLanguage = QStringLiteral("en");
    // Struct member default above already covers this (QStringLiteral
    // ctor runs regardless of this function), but set explicitly anyway,
    // matching how toastEnabled/appLanguage are both set explicitly here
    // too rather than left to only the struct default — keeps this
    // function readable as "the complete Phase 7 default", not half of it
    // living silently in the struct definition instead.
    settings.toastPosition = ui::toCode(ui::ToastPosition::BottomRight);
    settings.toastMonitorMode = ui::toCode(ui::ToastMonitorMode::CursorScreen);
    settings.appThemeMode = ui::toCode(ui::AppThemeMode::Auto);
    settings.toastMonitorIndex = 0;
    settings.layoutColorAssignments.clear(); // no defaults assigned yet
    settings.hotkeys.insert(platform::kToastDismissHotkeyId,
                             HotkeyBinding::fromCombo(platform::kDefaultToastDismissHotkeyCombo));
    settings.hotkeys.insert(platform::kToastShowHotkeyId,
                             HotkeyBinding::fromCombo(platform::kDefaultToastShowHotkeyCombo));
    return settings;
}

nlohmann::json toJson(const Settings& settings) {
    nlohmann::json hotkeysJson = nlohmann::json::object();
    for (auto it = settings.hotkeys.constBegin(); it != settings.hotkeys.constEnd(); ++it) {
        hotkeysJson[it.key().toStdString()] = {{"modifiers", it.value().modifiers}, {"key", it.value().key}};
    }

    nlohmann::json pairsJson = nlohmann::json::array();
    for (const auto& pair : settings.layoutConversionPairs) {
        pairsJson.push_back({{"layoutA", pair.layoutA.toStdString()}, {"layoutB", pair.layoutB.toStdString()}});
    }

    nlohmann::json layoutColorAssignmentsJson = nlohmann::json::object();
    for (auto it = settings.layoutColorAssignments.constBegin(); it != settings.layoutColorAssignments.constEnd();
         ++it) {
        layoutColorAssignmentsJson[it.key().toStdString()] = it.value();
    }

    nlohmann::json j;
    j["version"] = settings.version;
    j[kFieldToastDuration] = settings.toastDurationMs;
    j[kFieldToastOpacityPercent] = settings.toastOpacityPercent;
    j[kFieldFollowerDuration] = settings.followerDurationMs;
    j[kFieldFollowerOpacityPercent] = settings.followerOpacityPercent;
    j[kFieldHotkeys] = hotkeysJson;
    j[kFieldLayoutConversionPairs] = pairsJson;
    j[kFieldToastEnabled] = settings.toastEnabled;
    j[kFieldFollowerEnabled] = settings.followerEnabled;
    j[kFieldAppLanguage] = settings.appLanguage.toStdString();
    j[kFieldToastPosition] = settings.toastPosition.toStdString();
    j[kFieldToastMonitorMode] = settings.toastMonitorMode.toStdString();
    j[kFieldAppThemeMode] = settings.appThemeMode.toStdString();
    j[kFieldToastMonitorIndex] = settings.toastMonitorIndex;
    j[kFieldLayoutColorAssignments] = layoutColorAssignmentsJson;
    return j;
}

namespace {

// No prior schema version exists yet to migrate from — this is the
// documented seam Phase 4's own requirement asks for ("forward migration
// for future schema versions", §5), filled in only once a real version 2
// exists (§4.3: don't build migration steps for a change that hasn't
// happened). A file whose version is already kCurrentSchemaVersion passes
// through unchanged.
nlohmann::json migrate(nlohmann::json json, int fromVersion) {
    (void)fromVersion;
    return json;
}

} // namespace

std::optional<Settings> fromJson(const nlohmann::json& json) {
    if (!json.is_object() || !json.contains("version") || !json["version"].is_number_integer()) {
        return std::nullopt;
    }

    const int fileVersion = json["version"].get<int>();
    if (fileVersion > kCurrentSchemaVersion) {
        // A newer binary wrote this file than the one reading it now —
        // no migration path can go backward, so this is treated the same
        // as a corrupt file (fall back to defaults) rather than guessing
        // at a downgrade.
        return std::nullopt;
    }

    const nlohmann::json migrated = migrate(json, fileVersion);

    const Settings defaults = defaultSettings();
    Settings settings;
    settings.version = kCurrentSchemaVersion;
    settings.toastDurationMs = migrated.value(kFieldToastDuration, defaults.toastDurationMs);

    settings.toastOpacityPercent = defaults.toastOpacityPercent;
    if (migrated.contains(kFieldToastOpacityPercent) && migrated[kFieldToastOpacityPercent].is_number_integer()) {
        const int percent = migrated[kFieldToastOpacityPercent].get<int>();
        if (percent >= kToastOpacityPercentMin && percent <= kToastOpacityPercentMax) {
            settings.toastOpacityPercent = percent;
        }
        // Out of range (or wrong type) — keep the default rather than
        // fail the whole load.
    }
    settings.followerDurationMs = migrated.value(kFieldFollowerDuration, defaults.followerDurationMs);

    settings.followerOpacityPercent = defaults.followerOpacityPercent;
    if (migrated.contains(kFieldFollowerOpacityPercent) && migrated[kFieldFollowerOpacityPercent].is_number_integer()) {
        const int percent = migrated[kFieldFollowerOpacityPercent].get<int>();
        if (percent >= kFollowerOpacityPercentMin && percent <= kFollowerOpacityPercentMax) {
            settings.followerOpacityPercent = percent;
        }
        // Out of range (or wrong type) — keep the default rather than
        // fail the whole load, same as toastOpacityPercent above.
    }

    // NOT a full replace: start from the current defaults, then let the
    // file's entries override/add to them. This is what makes an old
    // settings.json (written before this phase existed, with only
    // "convertText" in it) still end up with all seven of this phase's
    // hotkey ids registered — a plain "if the file has a hotkeys object,
    // use exactly and only what's in it" (what this used to do, via a
    // now-removed settings.hotkeys.clear() here) would silently drop
    // every default id the file predates, which is exactly what happened
    // on Mahdi's own machine: convertAuto and all six convertTo<Layout>
    // ids never got registered at all, because the old file only knew
    // about "convertText". A stale id like that old "convertText" itself
    // is carried over as a harmless inert entry (nothing subscribes to
    // it anymore) rather than actively pruned — simplest safe behavior,
    // not worth a dedicated migration step for.
    settings.hotkeys = defaults.hotkeys;
    if (migrated.contains(kFieldHotkeys) && migrated[kFieldHotkeys].is_object()) {
        for (auto it = migrated[kFieldHotkeys].begin(); it != migrated[kFieldHotkeys].end(); ++it) {
            const auto& entry = it.value();
            if (!entry.is_object() || !entry.contains("modifiers") || !entry.contains("key")) {
                continue;
            }
            HotkeyBinding binding;
            binding.modifiers = entry.value("modifiers", 0);
            binding.key = entry.value("key", static_cast<int>(Qt::Key_unknown));
            settings.hotkeys.insert(QString::fromStdString(it.key()), binding);
        }
    }

    settings.layoutConversionPairs = defaults.layoutConversionPairs;
    if (migrated.contains(kFieldLayoutConversionPairs) && migrated[kFieldLayoutConversionPairs].is_array()) {
        settings.layoutConversionPairs.clear();
        for (const auto& entry : migrated[kFieldLayoutConversionPairs]) {
            if (!entry.is_object() || !entry.contains("layoutA") || !entry.contains("layoutB")) {
                continue;
            }
            ConversionPair pair;
            pair.layoutA = QString::fromStdString(entry.value("layoutA", std::string()));
            pair.layoutB = QString::fromStdString(entry.value("layoutB", std::string()));
            settings.layoutConversionPairs.push_back(pair);
        }
    }

    settings.toastEnabled = migrated.value(kFieldToastEnabled, defaults.toastEnabled);
    settings.followerEnabled = migrated.value(kFieldFollowerEnabled, defaults.followerEnabled);
    settings.appLanguage =
        QString::fromStdString(migrated.value(kFieldAppLanguage, defaults.appLanguage.toStdString()));
    settings.toastPosition =
        QString::fromStdString(migrated.value(kFieldToastPosition, defaults.toastPosition.toStdString()));
    settings.toastMonitorMode =
        QString::fromStdString(migrated.value(kFieldToastMonitorMode, defaults.toastMonitorMode.toStdString()));
    settings.appThemeMode =
        QString::fromStdString(migrated.value(kFieldAppThemeMode, defaults.appThemeMode.toStdString()));
    settings.toastMonitorIndex = migrated.value(kFieldToastMonitorIndex, defaults.toastMonitorIndex);

    settings.layoutColorAssignments = defaults.layoutColorAssignments;
    if (migrated.contains(kFieldLayoutColorAssignments) && migrated[kFieldLayoutColorAssignments].is_object()) {
        settings.layoutColorAssignments.clear();
        for (auto it = migrated[kFieldLayoutColorAssignments].begin();
             it != migrated[kFieldLayoutColorAssignments].end(); ++it) {
            // Drop bad entries instead of failing the whole load.
            if (!it.value().is_number_integer()) {
                continue;
            }
            const int swatchIndex = it.value().get<int>();
            if (!ui::isValidLayoutColorSwatch(swatchIndex)) {
                continue;
            }
            settings.layoutColorAssignments.insert(QString::fromStdString(it.key()), swatchIndex);
        }
    }

    return settings;
}

} // namespace lancue::settings
