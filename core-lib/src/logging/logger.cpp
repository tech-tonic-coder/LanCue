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
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/logs");
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
