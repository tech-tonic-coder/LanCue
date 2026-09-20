#include "corelib/lifecycle/single_instance_guard.h"

#include <QDir>
#include <QStandardPaths>

#include "corelib/logging/logger.h"

namespace lancue {

namespace {
// A stale lock older than this is removed even if QLockFile's own
// staleness heuristic (owning PID not running) is inconclusive — e.g. the
// PID got reused by an unrelated process after a crash, which QLockFile
// alone can't detect from PID liveness. 30s is comfortably longer than
// lancue-core ever legitimately takes to finish acquiring this lock at
// startup.
constexpr int kStaleLockTimeoutMs = 30000;
} // namespace

SingleInstanceGuard::SingleInstanceGuard(const QString& lockFileName) {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QDir().mkpath(dir);
    m_lockFile = std::make_unique<QLockFile>(dir + QLatin1Char('/') + lockFileName);
    m_lockFile->setStaleLockTime(kStaleLockTimeoutMs);
}

bool SingleInstanceGuard::tryAcquire() {
    if (m_lockFile->tryLock(0)) {
        return true;
    }

    switch (m_lockFile->error()) {
    case QLockFile::LockFailedError:
        // Someone else holds it and (per QLockFile's own liveness check)
        // is actually still running.
        logWarning(QStringLiteral("SingleInstanceGuard: another lancue-core instance is already running."));
        return false;
    case QLockFile::PermissionError:
        logError(QStringLiteral("SingleInstanceGuard: no permission to create the lock file."));
        return false;
    default:
        // Covers the stale-lock case explicitly: try to clear it and
        // acquire once more before giving up.
        if (m_lockFile->removeStaleLockFile() && m_lockFile->tryLock(0)) {
            logInfo(QStringLiteral("SingleInstanceGuard: removed a stale lock file from a previous instance."));
            return true;
        }
        logError(QStringLiteral("SingleInstanceGuard: could not acquire the lock (unknown error)."));
        return false;
    }
}

} // namespace lancue
