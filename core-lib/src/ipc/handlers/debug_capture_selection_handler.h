#pragma once

#include "corelib/ipc/dispatcher.h"
#include "corelib/platform/selection_clipboard_bridge.h"

namespace lancue::ipc::handlers {

void registerDebugCaptureSelectionHandler(Dispatcher& dispatcher, platform::SelectionClipboardBridge& bridge);

} // namespace lancue::ipc::handlers
