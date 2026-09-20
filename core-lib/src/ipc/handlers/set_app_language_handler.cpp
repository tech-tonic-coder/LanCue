#include "ipc/handlers/set_app_language_handler.h"

#include "corelib/i18n/language.h"
#include "corelib/ipc/message_types.h"
#include "corelib/logging/logger.h"

namespace lancue::ipc::handlers {

// Phase 7: sets LanCue's own UI language (i18n::AppLanguage — distinct
// from a keyboard layout id). Validated against i18n::fromCode()'s known
// codes rather than accepted verbatim: an unrecognized code stored as-is
// would silently fall back to English every time i18n::fromCode() reads
// it back (that function's own designed leniency, meant for an
// old/corrupt settings file — not something a live client sending a typo
// should get away with) — an explicit SetAppLanguage error surfaces the
// mistake immediately instead.
void registerSetAppLanguageHandler(Dispatcher& dispatcher, settings::SettingsManager& settingsManager) {
    dispatcher.registerHandler(
        kSetAppLanguage, [&settingsManager](const Message& request, Dispatcher::ReplyCallback reply) {
            Message message;

            if (!request.payload.contains("language") || !request.payload["language"].is_string()) {
                message.type = kError;
                message.payload = {{"reason", "SetAppLanguage requires a string 'language' field"}};
                reply(std::move(message));
                return;
            }

            const QString code = QString::fromStdString(request.payload["language"].get<std::string>());
            // i18n::fromCode() itself is deliberately lenient (falls back
            // to English for an unknown code — the right behavior for an
            // old/corrupt settings file, see that function's own doc
            // comment), so validity has to be checked by round-tripping
            // through toCode() instead of trusting fromCode()'s return
            // value alone: a live client sending a typo should get an
            // error, not a silent, unannounced fallback to English.
            const bool known = i18n::toCode(i18n::fromCode(code, i18n::AppLanguage::English))
                                    .compare(code, Qt::CaseInsensitive) == 0;
            if (!known) {
                message.type = kError;
                message.payload = {{"reason", "SetAppLanguage: unrecognized language code '" + code.toStdString() +
                                                   "' (expected 'en' or 'fa')"}};
                reply(std::move(message));
                return;
            }

            logInfo(QStringLiteral("SetAppLanguage request received: %1").arg(code));
            settingsManager.setAppLanguage(code);

            message.type = kAck;
            message.payload = {{"acknowledged", kSetAppLanguage}};
            reply(std::move(message));
        });
}

} // namespace lancue::ipc::handlers
