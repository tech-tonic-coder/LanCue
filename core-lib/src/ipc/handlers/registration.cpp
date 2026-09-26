#include "corelib/ipc/registration.h"

#include "ipc/handlers/debug_capture_selection_handler.h"
#include "ipc/handlers/debug_paste_replacement_handler.h"
#include "ipc/handlers/get_settings_handler.h"
#include "ipc/handlers/ping_handler.h"
#include "ipc/handlers/remove_hotkey_handler.h"
#include "ipc/handlers/save_settings_handler.h"
#include "ipc/handlers/set_app_language_handler.h"
#include "ipc/handlers/set_app_theme_mode_handler.h"
#include "ipc/handlers/set_follower_duration_handler.h"
#include "ipc/handlers/set_follower_enabled_handler.h"
#include "ipc/handlers/set_follower_opacity_percent_handler.h"
#include "ipc/handlers/set_hotkey_handler.h"
#include "ipc/handlers/set_layout_color_assignment_handler.h"
#include "ipc/handlers/set_layout_conversion_pairs_handler.h"
#include "ipc/handlers/set_toast_duration_handler.h"
#include "ipc/handlers/set_toast_opacity_percent_handler.h"
#include "ipc/handlers/set_toast_enabled_handler.h"
#include "ipc/handlers/set_toast_monitor_index_handler.h"
#include "ipc/handlers/set_toast_monitor_mode_handler.h"
#include "ipc/handlers/set_toast_position_handler.h"
#include "ipc/handlers/shutdown_handler.h"

namespace lancue::ipc {

void registerBuiltinHandlers(Dispatcher& dispatcher, settings::SettingsManager& settingsManager,
                              platform::SelectionClipboardBridge& selectionBridge) {
    handlers::registerPingHandler(dispatcher);
    handlers::registerShutdownHandler(dispatcher);
    handlers::registerSetToastDurationHandler(dispatcher, settingsManager);
    handlers::registerSetToastOpacityPercentHandler(dispatcher, settingsManager);
    handlers::registerSetFollowerDurationHandler(dispatcher, settingsManager);
    handlers::registerSetFollowerOpacityPercentHandler(dispatcher, settingsManager);
    handlers::registerSetHotkeyHandler(dispatcher, settingsManager);
    handlers::registerRemoveHotkeyHandler(dispatcher, settingsManager);
    handlers::registerSetLayoutConversionPairsHandler(dispatcher, settingsManager);
    handlers::registerSetLayoutColorAssignmentHandler(dispatcher, settingsManager);
    handlers::registerSetToastEnabledHandler(dispatcher, settingsManager);
    handlers::registerSetFollowerEnabledHandler(dispatcher, settingsManager);
    handlers::registerSetAppLanguageHandler(dispatcher, settingsManager);
    handlers::registerSetAppThemeModeHandler(dispatcher, settingsManager);
    handlers::registerSetToastPositionHandler(dispatcher, settingsManager);
    handlers::registerSetToastMonitorModeHandler(dispatcher, settingsManager);
    handlers::registerSetToastMonitorIndexHandler(dispatcher, settingsManager);
    handlers::registerGetSettingsHandler(dispatcher, settingsManager);
    handlers::registerSaveSettingsHandler(dispatcher, settingsManager);
    handlers::registerDebugCaptureSelectionHandler(dispatcher, selectionBridge);
    handlers::registerDebugPasteReplacementHandler(dispatcher, selectionBridge);
}

} // namespace lancue::ipc
