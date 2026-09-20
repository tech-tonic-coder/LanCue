#include <catch2/catch_test_macros.hpp>

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTemporaryDir>

#include "corelib/eventbus/event_bus.h"
#include "corelib/settings/settings_manager.h"
#include "corelib/settings/settings_schema.h"

using lancue::Event;
using lancue::EventBus;
using lancue::EventType;
using namespace lancue::settings;
namespace platform = lancue::platform;

TEST_CASE("defaultSettings() is already valid without a load()", "[settings]") {
    const Settings settings = defaultSettings();

    CHECK(settings.version == kCurrentSchemaVersion);
    CHECK(settings.toastDurationMs == kDefaultToastDurationMs);
    CHECK(settings.followerDurationMs == kDefaultFollowerDurationMs);
    CHECK_FALSE(settings.hotkeys.isEmpty());
    CHECK_FALSE(settings.layoutConversionPairs.isEmpty());
    CHECK(settings.toastEnabled == true);
    CHECK(settings.appLanguage == QStringLiteral("en"));
    CHECK(settings.toastPosition == QStringLiteral("bottom-right"));
    CHECK(settings.toastMonitorMode == QStringLiteral("cursor"));
    CHECK(settings.toastMonitorIndex == 0);
    CHECK(settings.layoutColorAssignments.isEmpty());
    CHECK(settings.toastOpacityPercent == kDefaultToastOpacityPercent);
    CHECK(settings.appThemeMode == QStringLiteral("auto"));
    CHECK(settings.hotkeys.contains(platform::kToastDismissHotkeyId));
    CHECK(settings.hotkeys.contains(platform::kToastShowHotkeyId));
}

TEST_CASE("toJson/fromJson round-trips every field exactly", "[settings]") {
    Settings original = defaultSettings();
    original.toastDurationMs = 4200;
    original.followerDurationMs = 900;
    original.hotkeys.insert(QStringLiteral("secondAction"),
                             HotkeyBinding{static_cast<int>(Qt::ShiftModifier), static_cast<int>(Qt::Key_K)});
    original.layoutConversionPairs.push_back(ConversionPair{QStringLiteral("de-de"), QStringLiteral("ru-ru")});
    original.toastEnabled = false;
    original.appLanguage = QStringLiteral("fa");
    original.toastPosition = QStringLiteral("top-left");
    original.toastMonitorMode = QStringLiteral("all");
    original.toastMonitorIndex = 2;
    original.layoutColorAssignments.insert(QStringLiteral("en-us"), 1);
    original.layoutColorAssignments.insert(QStringLiteral("fa-ir"), 2);
    original.toastOpacityPercent = 75;
    original.appThemeMode = QStringLiteral("dark");

    const nlohmann::json json = toJson(original);
    const auto roundTripped = fromJson(json);

    REQUIRE(roundTripped.has_value());
    const Settings& result = roundTripped.value();

    CHECK(result.version == original.version);
    CHECK(result.toastDurationMs == original.toastDurationMs);
    CHECK(result.followerDurationMs == original.followerDurationMs);
    CHECK(result.hotkeys.size() == original.hotkeys.size());
    for (auto it = original.hotkeys.constBegin(); it != original.hotkeys.constEnd(); ++it) {
        REQUIRE(result.hotkeys.contains(it.key()));
        CHECK(result.hotkeys.value(it.key()) == it.value());
    }
    CHECK(result.layoutConversionPairs == original.layoutConversionPairs);
    CHECK(result.toastEnabled == original.toastEnabled);
    CHECK(result.appLanguage == original.appLanguage);
    CHECK(result.toastPosition == original.toastPosition);
    CHECK(result.toastMonitorMode == original.toastMonitorMode);
    CHECK(result.toastMonitorIndex == original.toastMonitorIndex);
    CHECK(result.layoutColorAssignments == original.layoutColorAssignments);
    CHECK(result.toastOpacityPercent == original.toastOpacityPercent);
    CHECK(result.appThemeMode == original.appThemeMode);
}

TEST_CASE("fromJson() falls back to defaults per-field for toastEnabled/appLanguage", "[settings]") {
    // An old settings.json without these fields should still load fine.
    nlohmann::json json;
    json["version"] = kCurrentSchemaVersion;

    const auto parsed = fromJson(json);
    REQUIRE(parsed.has_value());
    CHECK(parsed->toastEnabled == defaultSettings().toastEnabled);
    CHECK(parsed->appLanguage == defaultSettings().appLanguage);
    CHECK(parsed->toastPosition == defaultSettings().toastPosition);
    CHECK(parsed->toastMonitorMode == defaultSettings().toastMonitorMode);
    CHECK(parsed->toastMonitorIndex == defaultSettings().toastMonitorIndex);
    CHECK(parsed->layoutColorAssignments == defaultSettings().layoutColorAssignments);
}

TEST_CASE("fromJson() drops invalid layoutColorAssignments entries per-entry, not the whole field", "[settings]") {
    // One bad entry (wrong type, out of range) gets dropped; the rest of
    // the map still loads.
    nlohmann::json json;
    json["version"] = kCurrentSchemaVersion;
    json[kFieldLayoutColorAssignments] = {
        {"en-us", 1},      // valid
        {"fa-ir", "2"},    // wrong type (string, not int) -> dropped
        {"ar-sa", 9},      // out of the 1-8 palette range -> dropped
        {"ru-ru", 0},      // out of range (below min) -> dropped
        {"tr-tr", 4},      // valid
    };

    const auto parsed = fromJson(json);
    REQUIRE(parsed.has_value());
    CHECK(parsed->layoutColorAssignments.size() == 2);
    CHECK(parsed->layoutColorAssignments.value(QStringLiteral("en-us")) == 1);
    CHECK(parsed->layoutColorAssignments.value(QStringLiteral("tr-tr")) == 4);
    CHECK_FALSE(parsed->layoutColorAssignments.contains(QStringLiteral("fa-ir")));
    CHECK_FALSE(parsed->layoutColorAssignments.contains(QStringLiteral("ar-sa")));
    CHECK_FALSE(parsed->layoutColorAssignments.contains(QStringLiteral("ru-ru")));
}

TEST_CASE("fromJson() keeps the default toastOpacityPercent when the stored value is out of range", "[settings]") {
    nlohmann::json json;
    json["version"] = kCurrentSchemaVersion;
    json[kFieldToastOpacityPercent] = 40; // below kToastOpacityPercentMin

    const auto parsed = fromJson(json);
    REQUIRE(parsed.has_value());
    CHECK(parsed->toastOpacityPercent == kDefaultToastOpacityPercent);
}

TEST_CASE("fromJson() accepts a valid toastOpacityPercent", "[settings]") {
    nlohmann::json json;
    json["version"] = kCurrentSchemaVersion;
    json[kFieldToastOpacityPercent] = 70;

    const auto parsed = fromJson(json);
    REQUIRE(parsed.has_value());
    CHECK(parsed->toastOpacityPercent == 70);
}

TEST_CASE("HotkeyBinding round-trips through platform::HotkeyCombo", "[settings]") {
    const platform::HotkeyCombo combo{Qt::ControlModifier | Qt::ShiftModifier, Qt::Key_P};
    const HotkeyBinding binding = HotkeyBinding::fromCombo(combo);
    const platform::HotkeyCombo restored = binding.toCombo();

    CHECK(restored.modifiers == combo.modifiers);
    CHECK(restored.key == combo.key);
}

TEST_CASE("fromJson merges an old file's hotkeys onto current defaults, not a wholesale replace", "[settings]") {
    // Regression test for a real bug hit on Mahdi's own machine
    // (2026-08-24): a settings.json written before Phase 6 introduced
    // convertAuto and the six convertTo<Layout> ids only had the old
    // "convertText" id in it. fromJson used to *replace* the default
    // hotkeys map wholesale with whatever the file had — meaning every
    // Phase 6 hotkey silently never got registered on an existing
    // install, even though a fresh install had them. The fix is to merge
    // the file's entries onto the current defaults instead.
    nlohmann::json oldHotkeyEntry;
    oldHotkeyEntry["modifiers"] = 6;
    oldHotkeyEntry["key"] = 76;
    nlohmann::json hotkeysObject;
    hotkeysObject["convertText"] = oldHotkeyEntry;

    nlohmann::json fileWithOnlyOneOldHotkey;
    fileWithOnlyOneOldHotkey["version"] = kCurrentSchemaVersion;
    fileWithOnlyOneOldHotkey["hotkeys"] = hotkeysObject;

    const auto result = fromJson(fileWithOnlyOneOldHotkey);
    REQUIRE(result.has_value());

    // The stale id from the old file is still present (carried over as a
    // harmless inert entry, not pruned)...
    CHECK(result->hotkeys.contains(QStringLiteral("convertText")));
    // ...but every current default id — none of which existed in this
    // simulated old file — must ALSO be present, with its default combo,
    // not silently dropped.
    const Settings defaults = defaultSettings();
    for (auto it = defaults.hotkeys.constBegin(); it != defaults.hotkeys.constEnd(); ++it) {
        CAPTURE(it.key());
        REQUIRE(result->hotkeys.contains(it.key()));
        CHECK(result->hotkeys.value(it.key()) == it.value());
    }
}

TEST_CASE("fromJson lets a file's hotkey value override the matching default, not the other way around",
          "[settings]") {
    // The merge direction matters: a user's actual customization for an
    // id that also happens to have a default must win, or "merge" would
    // just be a different way to silently discard real settings.
    const platform::HotkeyCombo customCombo{Qt::ControlModifier, Qt::Key_Z};

    nlohmann::json customEntry;
    customEntry["modifiers"] = static_cast<int>(customCombo.modifiers);
    customEntry["key"] = static_cast<int>(customCombo.key);
    nlohmann::json hotkeysObject;
    hotkeysObject[platform::kConvertAutoHotkeyId.toStdString()] = customEntry;

    nlohmann::json fileWithCustomizedDefault;
    fileWithCustomizedDefault["version"] = kCurrentSchemaVersion;
    fileWithCustomizedDefault["hotkeys"] = hotkeysObject;

    const auto result = fromJson(fileWithCustomizedDefault);
    REQUIRE(result.has_value());
    REQUIRE(result->hotkeys.contains(platform::kConvertAutoHotkeyId));
    const platform::HotkeyCombo restored = result->hotkeys.value(platform::kConvertAutoHotkeyId).toCombo();
    CHECK(restored.modifiers == customCombo.modifiers);
    CHECK(restored.key == customCombo.key);
}

TEST_CASE("fromJson falls back per-field for a partially-missing object", "[settings]") {
    // Missing "hotkeys" and "layoutConversionPairs" entirely, but a valid
    // version and toastDurationMs — should keep the valid fields and use
    // defaults for the missing ones, not fail the whole parse.
    nlohmann::json partial = {{"version", kCurrentSchemaVersion}, {"toastDurationMs", 7777}};

    const auto result = fromJson(partial);
    REQUIRE(result.has_value());
    CHECK(result->toastDurationMs == 7777);
    CHECK(result->followerDurationMs == kDefaultFollowerDurationMs);
    CHECK_FALSE(result->hotkeys.isEmpty());
}

TEST_CASE("fromJson rejects a missing or non-integer version", "[settings]") {
    CHECK_FALSE(fromJson(nlohmann::json::object()).has_value());
    CHECK_FALSE(fromJson(nlohmann::json{{"version", "not-a-number"}}).has_value());
}

TEST_CASE("fromJson rejects a version newer than this binary understands", "[settings]") {
    const nlohmann::json future = {{"version", kCurrentSchemaVersion + 1}};
    CHECK_FALSE(fromJson(future).has_value());
}

TEST_CASE("fromJson rejects a non-object payload", "[settings]") {
    CHECK_FALSE(fromJson(nlohmann::json::array()).has_value());
    CHECK_FALSE(fromJson(nlohmann::json(42)).has_value());
}

TEST_CASE("SettingsManager falls back to defaults when the file doesn't exist", "[settings]") {
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("does-not-exist.json"));

    EventBus bus;
    SettingsManager manager(bus, path);
    manager.load();

    CHECK(manager.current().toastDurationMs == kDefaultToastDurationMs);
}

TEST_CASE("SettingsManager falls back to defaults for a corrupt file without crashing", "[settings]") {
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("corrupt.json"));

    QFile file(path);
    REQUIRE(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write("{ this is not valid json ");
    file.close();

    EventBus bus;
    SettingsManager manager(bus, path);
    manager.load();

    CHECK(manager.current().toastDurationMs == kDefaultToastDurationMs);
}

TEST_CASE("SettingsManager round-trips a real save()+load() through disk", "[settings]") {
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("settings.json"));

    EventBus writerBus;
    SettingsManager writer(writerBus, path);
    writer.load(); // no file yet -> defaults
    writer.setToastDurationMs(5555);
    REQUIRE(writer.save());

    EventBus readerBus;
    SettingsManager reader(readerBus, path);
    reader.load();

    CHECK(reader.current().toastDurationMs == 5555);
}

TEST_CASE("a mutator live-previews via EventBus without touching disk until save()", "[settings]") {
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("no-write-yet.json"));

    EventBus bus;
    int changeCount = 0;
    QString lastField;
    bus.subscribe(EventType::SettingsChanged, [&](const Event& e) {
        ++changeCount;
        lastField = e.data.toString();
    });

    SettingsManager manager(bus, path);
    manager.load();
    manager.setFollowerDurationMs(2222);

    CHECK(changeCount == 1);
    CHECK(lastField == QString::fromUtf8(kFieldFollowerDuration));
    CHECK(manager.current().followerDurationMs == 2222);
    CHECK_FALSE(QFile::exists(path));
}

TEST_CASE("setHotkey/removeHotkey update the in-memory map and publish 'hotkeys'", "[settings]") {
    EventBus bus;
    QString lastField;
    bus.subscribe(EventType::SettingsChanged, [&](const Event& e) { lastField = e.data.toString(); });

    SettingsManager manager(bus);
    manager.load();

    const platform::HotkeyCombo combo{Qt::ControlModifier, Qt::Key_9};
    manager.setHotkey(QStringLiteral("testAction"), combo);
    CHECK(lastField == QString::fromUtf8(kFieldHotkeys));
    REQUIRE(manager.current().hotkeys.contains(QStringLiteral("testAction")));
    CHECK(manager.current().hotkeys.value(QStringLiteral("testAction")).toCombo().key == combo.key);

    manager.removeHotkey(QStringLiteral("testAction"));
    CHECK(lastField == QString::fromUtf8(kFieldHotkeys));
    CHECK_FALSE(manager.current().hotkeys.contains(QStringLiteral("testAction")));
}

TEST_CASE("setLayoutColorAssignment updates the in-memory map and publishes 'layoutColorAssignments'", "[settings]") {
    EventBus bus;
    QString lastField;
    bus.subscribe(EventType::SettingsChanged, [&](const Event& e) { lastField = e.data.toString(); });

    SettingsManager manager(bus);
    manager.load();

    manager.setLayoutColorAssignment(QStringLiteral("fa-ir"), 2);
    CHECK(lastField == QString::fromUtf8(kFieldLayoutColorAssignments));
    REQUIRE(manager.current().layoutColorAssignments.contains(QStringLiteral("fa-ir")));
    CHECK(manager.current().layoutColorAssignments.value(QStringLiteral("fa-ir")) == 2);

    // Re-assigning the same layout id overwrites, not duplicates/errors.
    manager.setLayoutColorAssignment(QStringLiteral("fa-ir"), 5);
    CHECK(manager.current().layoutColorAssignments.value(QStringLiteral("fa-ir")) == 5);
    CHECK(manager.current().layoutColorAssignments.size() == 1);
}

TEST_CASE("setToastOpacityPercent updates the in-memory value and publishes 'toastOpacityPercent'", "[settings]") {
    EventBus bus;
    QString lastField;
    bus.subscribe(EventType::SettingsChanged, [&](const Event& e) { lastField = e.data.toString(); });

    SettingsManager manager(bus);
    manager.load();

    manager.setToastOpacityPercent(75);
    CHECK(lastField == QString::fromUtf8(kFieldToastOpacityPercent));
    CHECK(manager.current().toastOpacityPercent == 75);
}

TEST_CASE("setAppThemeMode updates the in-memory value and publishes 'appThemeMode'", "[settings]") {
    EventBus bus;
    QString lastField;
    bus.subscribe(EventType::SettingsChanged, [&](const Event& e) { lastField = e.data.toString(); });

    SettingsManager manager(bus);
    manager.load();

    manager.setAppThemeMode(QStringLiteral("dark"));
    CHECK(lastField == QString::fromUtf8(kFieldAppThemeMode));
    CHECK(manager.current().appThemeMode == QStringLiteral("dark"));
}
