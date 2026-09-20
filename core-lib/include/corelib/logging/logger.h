#pragma once

#include <QString>

namespace lancue {

// Minimal file-backed logger. Needed starting this phase because
// lancue-core drops its console window (§2.1 "clean background
// execution") — once that happens, std::cout is gone, so anything worth
// knowing at runtime (IPC errors, lock-file issues, later phases'
// platform gotchas) needs somewhere else to go. Deliberately not a full
// logging framework (no levels config, no rotation) — nothing in the
// roadmap asks for more than "diagnosable after the fact" yet; add real
// structure only when a later phase actually needs it, per §4.3.
namespace Logger {

// componentName distinguishes lancue-core.log from lancue-settings.log
// under the same per-OS log directory.
void init(const QString& componentName);

} // namespace Logger

void logInfo(const QString& message);
void logWarning(const QString& message);
void logError(const QString& message);

} // namespace lancue
