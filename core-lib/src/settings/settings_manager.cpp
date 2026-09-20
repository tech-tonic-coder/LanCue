#include "corelib/settings/settings_manager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>

#include <utility>

#include "corelib/logging/logger.h"

namespace lancue::settings {

SettingsManager::SettingsManager(EventBus& eventBus, QString overrideFilePath)
    : m_eventBus(eventBus), m_current(defaultSettings()), m_overrideFilePath(std::move(overrideFilePath)) {}

QString SettingsManager::filePath() const {
    if (!m_overrideFilePath.isEmpty()) {
        return m_overrideFilePath;
    }

    // AppConfigLocation, not Logger's AppDataLocation (§5 Phase 4: "storage
    // location per OS convention"). The two resolve to the same directory
    // on Windows/macOS but differ on Linux — AppConfigLocation is
    // $XDG_CONFIG_HOME (~/.config/lancue), AppDataLocation is
    // $XDG_DATA_HOME (~/.local/share/lancue) — and "~/.config" is
    // literally what this phase's own spec names, so this is a deliberate
    // choice, not an oversight that Logger and SettingsManager use
    // different QStandardPaths locations.
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return dir + QStringLiteral("/settings.json");
}

void SettingsManager::load() {
    const QString path = filePath();
    QFile file(path);

    if (!file.exists()) {
        logInfo(QStringLiteral("SettingsManager: no settings file at '%1' yet, starting with defaults.").arg(path));
        m_current = defaultSettings();
        return;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        logWarning(QStringLiteral("SettingsManager: could not open '%1' (%2), falling back to defaults.")
                       .arg(path, file.errorString()));
        m_current = defaultSettings();
        return;
    }

    const QByteArray bytes = file.readAll();
    file.close();

    nlohmann::json parsed;
    try {
        parsed = nlohmann::json::parse(bytes.constData(), bytes.constData() + bytes.size());
    } catch (const nlohmann::json::parse_error& e) {
        logWarning(QStringLiteral("SettingsManager: '%1' contains invalid JSON (%2), falling back to defaults.")
                       .arg(path, QString::fromStdString(e.what())));
        m_current = defaultSettings();
        return;
    }

    const auto settings = fromJson(parsed);
    if (!settings.has_value()) {
        logWarning(
            QStringLiteral("SettingsManager: '%1' has an unreadable or unsupported schema version, falling back "
                            "to defaults.")
                .arg(path));
        m_current = defaultSettings();
        return;
    }

    m_current = settings.value();
    logInfo(QStringLiteral("SettingsManager: loaded settings from '%1'.").arg(path));
}

bool SettingsManager::save() {
    const QString path = filePath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        logError(QStringLiteral("SettingsManager: could not write '%1' (%2).").arg(path, file.errorString()));
        return false;
    }

    const std::string body = toJson(m_current).dump(2);
    QTextStream stream(&file);
    stream << QString::fromStdString(body);
    file.close();

    logInfo(QStringLiteral("SettingsManager: saved settings to '%1'.").arg(path));
    return true;
}

void SettingsManager::publishChanged(const QString& field) {
    Event event;
    event.type = EventType::SettingsChanged;
    event.data = field;
    m_eventBus.publish(event);
}

void SettingsManager::setToastDurationMs(int ms) {
    m_current.toastDurationMs = ms;
    publishChanged(QString::fromUtf8(kFieldToastDuration));
}

void SettingsManager::setToastOpacityPercent(int percent) {
    m_current.toastOpacityPercent = percent;
    publishChanged(QString::fromUtf8(kFieldToastOpacityPercent));
}

void SettingsManager::setFollowerDurationMs(int ms) {
    m_current.followerDurationMs = ms;
    publishChanged(QString::fromUtf8(kFieldFollowerDuration));
}

void SettingsManager::setFollowerOpacityPercent(int percent) {
    m_current.followerOpacityPercent = percent;
    publishChanged(QString::fromUtf8(kFieldFollowerOpacityPercent));
}

void SettingsManager::setHotkey(const QString& id, const platform::HotkeyCombo& combo) {
    m_current.hotkeys.insert(id, HotkeyBinding::fromCombo(combo));
    publishChanged(QString::fromUtf8(kFieldHotkeys));
}

void SettingsManager::removeHotkey(const QString& id) {
    m_current.hotkeys.remove(id);
    publishChanged(QString::fromUtf8(kFieldHotkeys));
}

void SettingsManager::setLayoutConversionPairs(const QVector<ConversionPair>& pairs) {
    m_current.layoutConversionPairs = pairs;
    publishChanged(QString::fromUtf8(kFieldLayoutConversionPairs));
}

void SettingsManager::setToastEnabled(bool enabled) {
    m_current.toastEnabled = enabled;
    publishChanged(QString::fromUtf8(kFieldToastEnabled));
}

void SettingsManager::setFollowerEnabled(bool enabled) {
    m_current.followerEnabled = enabled;
    publishChanged(QString::fromUtf8(kFieldFollowerEnabled));
}

void SettingsManager::setAppLanguage(const QString& languageCode) {
    m_current.appLanguage = languageCode;
    publishChanged(QString::fromUtf8(kFieldAppLanguage));
}

void SettingsManager::setToastPosition(const QString& positionCode) {
    m_current.toastPosition = positionCode;
    publishChanged(QString::fromUtf8(kFieldToastPosition));
}

void SettingsManager::setToastMonitorMode(const QString& modeCode) {
    m_current.toastMonitorMode = modeCode;
    publishChanged(QString::fromUtf8(kFieldToastMonitorMode));
}

void SettingsManager::setAppThemeMode(const QString& modeCode) {
    m_current.appThemeMode = modeCode;
    publishChanged(QString::fromUtf8(kFieldAppThemeMode));
}

void SettingsManager::setToastMonitorIndex(int index) {
    m_current.toastMonitorIndex = index;
    publishChanged(QString::fromUtf8(kFieldToastMonitorIndex));
}

void SettingsManager::setLayoutColorAssignment(const QString& layoutId, int swatchIndex) {
    m_current.layoutColorAssignments.insert(layoutId, swatchIndex);
    publishChanged(QString::fromUtf8(kFieldLayoutColorAssignments));
}

} // namespace lancue::settings
