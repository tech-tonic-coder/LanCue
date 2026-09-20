#pragma once

#include "corelib/ipc/dispatcher.h"
#include "corelib/settings/settings_manager.h"

namespace lancue::ipc::handlers {

void registerSetLayoutConversionPairsHandler(Dispatcher& dispatcher, settings::SettingsManager& settingsManager);

} // namespace lancue::ipc::handlers
