#include "corelib/ipc/socket_name.h"

namespace lancue::ipc {

QString socketName() {
    return QStringLiteral("lancue-ipc");
}

} // namespace lancue::ipc
