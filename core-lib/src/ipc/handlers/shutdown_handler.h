#pragma once

#include "corelib/ipc/dispatcher.h"

namespace lancue::ipc::handlers {

void registerShutdownHandler(Dispatcher& dispatcher);

} // namespace lancue::ipc::handlers
