#include "corelib/logging/logger.h"

#include <cstdio>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>

namespace lancue {

namespace {
QFile* g_logFile = nullptr;

void writeLine(const QString& level, const QString& message) {
    const QString line = QStringLiteral("[%1] %2 %3")
                              .arg(QDateTime::currentDateTime().toString(Qt::ISODate), level, message);

    if (g_logFile && g_logFile->isOpen()) {
        QTextStream stream(g_logFile);
        stream << line << Qt::endl;
    }
#ifndef NDEBUG
    // Debug builds keep stderr output too, since a developer running
    // lancue-core directly from a terminal during development shouldn't
    // have to go find the log file for every message.
    std::fprintf(stderr, "%s\n", qUtf8Printable(line));
#endif
}
} // namespace

namespace Logger {

void init(const QString& componentName) {
    // AppLocalDataLocation, not AppDataLocation: on Windows, AppDataLocation
    // resolves to the Roaming profile (%APPDATA%) while AppLocalDataLocation
    // and SettingsManager's AppConfigLocation both resolve to the Local
    // profile (%LOCALAPPDATA%) — confirmed via Qt's own docs (round 9: a
    // real CI run exposed that logs and settings.json lived in two
    // different folders on Windows, which a since-corrected comment in
    // settings_manager.cpp had wrongly assumed couldn't happen). Logs are
    // machine-specific operational data anyway, which is a better fit for
    // the Local profile than something meant to roam with the user.
    const QString dir =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + QStringLiteral("/logs");
    QDir().mkpath(dir);

    delete g_logFile;
    g_logFile = new QFile(dir + QLatin1Char('/') + componentName + QStringLiteral(".log"));
    if (!g_logFile->open(QIODevice::Append | QIODevice::Text)) {
        // Nothing else to fall back to here — writeLine() already checks
        // isOpen() before writing, so a failed open just means log lines
        // silently only go to stderr (debug builds) instead of a file.
        delete g_logFile;
        g_logFile = nullptr;
    }
}

} // namespace Logger

void logInfo(const QString& message) { writeLine(QStringLiteral("INFO "), message); }
void logWarning(const QString& message) { writeLine(QStringLiteral("WARN "), message); }
void logError(const QString& message) { writeLine(QStringLiteral("ERROR"), message); }

} // namespace lancue
