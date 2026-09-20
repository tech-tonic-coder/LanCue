#pragma once

#include <QString>

namespace lancue::ipc {

// Shared local-server name both lancue-core (listener) and lancue-settings
// (connector) resolve independently, rather than passing it out of band —
// there's no other IPC channel to pass it over before this one exists.
// QLocalServer/QLocalSocket map this single name to a named pipe on
// Windows and a Unix domain socket under a per-user runtime dir on
// macOS/Linux, so no per-OS branching is needed here.
QString socketName();

} // namespace lancue::ipc
