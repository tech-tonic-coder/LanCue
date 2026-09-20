#pragma once

#include <memory>

#include <QLockFile>
#include <QString>

namespace lancue {

// Enforces "only one lancue-core at a time" using QLockFile rather than a
// hand-rolled PID-in-a-file check. QLockFile stamps the lock with PID +
// hostname + app name and, on tryLock() failure, can tell a live holder
// apart from a stale one left by a process that crashed without cleaning
// up — that stale-vs-live distinction is exactly what a naive "does the
// file exist" check can't do, and is the mechanism that satisfies the
// roadmap's Phase 1 requirement that a crashed previous instance must not
// deadlock a restart.
class SingleInstanceGuard {
public:
    explicit SingleInstanceGuard(const QString& lockFileName);

    // Attempts to acquire the lock, clearing it first if it's stale
    // (owning PID no longer running). Returns false if another instance is
    // genuinely running right now.
    bool tryAcquire();

private:
    std::unique_ptr<QLockFile> m_lockFile;
};

} // namespace lancue
