#pragma once

#include "corelib/eventbus/event_bus.h"
#include "corelib/ipc/dispatcher.h"
#include "corelib/settings/settings_manager.h"

namespace lancue::ipc::handlers {

void registerSetToastMonitorModeHandler(Dispatcher& dispatcher, settings::SettingsManager& settingsManager);

} // namespace lancue::ipc::handlers
