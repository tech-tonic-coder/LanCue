#pragma once

#include <functional>
#include <memory>

#include <QString>
#include <QtCore/qnamespace.h>

namespace lancue::platform {

// A modifier+key combination, OS-agnostic. Reuses Qt::KeyboardModifiers/
// Qt::Key rather than inventing a parallel LanCue-specific key scheme —
// unlike keyboard-layout ids (Phase 2), there's no cross-OS reporting
// difference to normalize away here: this is a value *we* choose and
// pass down to the platform layer, not something read back from the OS
// in different shapes per platform. Each platform implementation maps
// this to its own native representation internally (see
// global_hotkey_manager_windows.cpp's key-mapping table).
struct HotkeyCombo {
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;
    Qt::Key key = Qt::Key_unknown;

    bool operator==(const HotkeyCombo& other) const {
        return modifiers == other.modifiers && key == other.key;
    }
    bool operator!=(const HotkeyCombo& other) const {
        return !(*this == other);
    }

    // Human-readable form for logging and for the HotkeyRegistrationFailed
    // event payload, e.g. "Ctrl+Alt+L". Not meant to round-trip back into
    // a HotkeyCombo — it's a display string, not a serialization format.
    QString toString() const;
};

// LanCue is expected to grow more than one independently-triggerable
// action (confirmed 2026-08-05 — see the Phase 3 Learnings entry), so
// every hotkey is identified by a caller-chosen string id rather than
// this interface only ever tracking one combo. Ids are opaque to this
// interface — it's just a key for "which combo is this", not something
// with OS meaning — so callers (GlobalHotkeyFeature today; a real
// settings-backed action registry from Phase 4/9 on) can name them
// however suits the action they represent, e.g. "convertText".
using HotkeyId = QString;

// Phase 4 sources the real hotkey set from user settings now; these stay
// as the fallback defaults a fresh install ships with (see
// settings::defaultSettings()) and as ConversionFeature's own
// last-resort if settings somehow come back with an empty hotkey map.
// Defined once here per §4.5 rather than re-declared wherever a "default
// hotkey" is needed. core-lib can't depend on lancue-core's features
// (wrong direction, §2.1), so these ids/combos live down here where both
// core-lib's settings::defaultSettings() and lancue-core's
// ConversionFeature can reach them.
//
// Phase 6 replaces the single Phase 3 placeholder hotkey ("convertText",
// Ctrl+Alt+L) with Switex's own multi-hotkey scheme (ported combos, not
// just the concept — see the Phase 6 Learnings entry for why matching
// Switex exactly here was the deliberate choice): one generic
// auto-detect hotkey plus one dedicated hotkey per target layout, each
// combo Ctrl+Alt+Shift+<letter> — the letter matching Switex's own
// per-language hotkey scheme exactly (switex.py's `hotkeys` dict), not
// simply "first letter of the English name" (that description happens
// to match every one of these, but Switex's actual rule is the
// language's own short code: 'f' for fa, 'e' for en, 'a' for ar, 'r' for
// ru, 't' for tr, 'h' for he).
inline const HotkeyId kConvertAutoHotkeyId = QStringLiteral("convertAuto");
inline const HotkeyCombo kDefaultConvertAutoHotkeyCombo{Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier,
                                                         Qt::Key_Space};

inline const HotkeyId kConvertToEnUsHotkeyId = QStringLiteral("convertToEnUs");
inline const HotkeyCombo kDefaultConvertToEnUsHotkeyCombo{Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier,
                                                           Qt::Key_E};

inline const HotkeyId kConvertToFaIrHotkeyId = QStringLiteral("convertToFaIr");
inline const HotkeyCombo kDefaultConvertToFaIrHotkeyCombo{Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier,
                                                           Qt::Key_F};

inline const HotkeyId kConvertToArSaHotkeyId = QStringLiteral("convertToArSa");
inline const HotkeyCombo kDefaultConvertToArSaHotkeyCombo{Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier,
                                                           Qt::Key_A};

inline const HotkeyId kConvertToRuRuHotkeyId = QStringLiteral("convertToRuRu");
inline const HotkeyCombo kDefaultConvertToRuRuHotkeyCombo{Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier,
                                                           Qt::Key_R};

inline const HotkeyId kConvertToTrTrHotkeyId = QStringLiteral("convertToTrTr");
inline const HotkeyCombo kDefaultConvertToTrTrHotkeyCombo{Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier,
                                                           Qt::Key_T};

inline const HotkeyId kConvertToHeIlHotkeyId = QStringLiteral("convertToHeIl");
inline const HotkeyCombo kDefaultConvertToHeIlHotkeyCombo{Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier,
                                                           Qt::Key_H};

// Phase 7: the toast window's own two hotkeys, same Ctrl+Alt+Shift+<letter>
// family as the six above (consistent with the rest of this project's
// scheme rather than a different modifier combination for no reason) but
// deliberately picked from letters none of the six conversion hotkeys
// already use (E/F/A/R/T/H) — Q for "dismiss" (mnemonic: Quit/close) and
// L for manually "showing" the toast on demand (no layout-name letter fit
// as cleanly, so this one isn't a mnemonic — see the Phase 7 Learnings
// entry for why Q/L specifically, over Mahdi's own alternative of V). As
// with every hotkey in this project, an actual OS-level conflict is
// caught and reported at registration time via
// EventType::HotkeyRegistrationFailed (GlobalHotkeyFeature), not
// something this default combo choice can fully rule out up front.
inline const HotkeyId kToastDismissHotkeyId = QStringLiteral("toastDismiss");
inline const HotkeyCombo kDefaultToastDismissHotkeyCombo{Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier,
                                                          Qt::Key_Q};

inline const HotkeyId kToastShowHotkeyId = QStringLiteral("toastShow");
inline const HotkeyCombo kDefaultToastShowHotkeyCombo{Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier,
                                                       Qt::Key_L};

// Abstract, event-driven global hotkey registration — one implementation
// per OS (§4.4), selected at build time via createGlobalHotkeyManager()
// below, no #ifdef at call sites. Unlike IKeyboardLayoutWatcher (which
// only observes an OS-owned notification), this interface *claims*
// specific key combinations for this process, so failure here commonly
// means another application already owns the combination, not that the
// mechanism itself is broken — callers must treat registerHotkey()
// returning false as an expected runtime outcome to surface, not just a
// log line (§5 Phase 3 requirements).
//
// Supports any number of simultaneously-registered hotkeys, each
// independent of the others — registering or unregistering one must
// never disturb any other already-registered id.
class IGlobalHotkeyManager {
public:
    // Invoked with the id of whichever registered hotkey was just
    // pressed, so one callback can serve every registered id rather than
    // needing a callback per id.
    using HotkeyPressedCallback = std::function<void(const HotkeyId& id)>;

    virtual ~IGlobalHotkeyManager() = default;

    // Registers the callback invoked every time any registered hotkey is
    // pressed. Must be called before the first registerHotkey().
    // Implementations must only ever invoke this from their own
    // OS-callback/hook handler — never from a polling loop (§4.7).
    virtual void setCallback(HotkeyPressedCallback callback) = 0;

    // Registers `combo` under `id`, replacing whatever combo was
    // previously registered under that same id if any (this is also how
    // a given hotkey is changed at runtime — see this phase's own
    // verification step — there's no separate "change" method). Other
    // ids' registrations are unaffected. Returns false if the OS refused
    // the registration (most commonly: another app already owns this
    // combination) — implementations must log the specific reason where
    // the OS provides one, and must leave any previous registration for
    // this id in place if the new one fails, rather than leaving `id`
    // unregistered.
    virtual bool registerHotkey(const HotkeyId& id, const HotkeyCombo& combo) = 0;

    // Releases the hotkey registered under `id`, if any. Safe to call
    // even if `id` was never registered or its last registration failed.
    virtual void unregisterHotkey(const HotkeyId& id) = 0;

    // Releases every currently-registered hotkey, regardless of id.
    virtual void stop() = 0;
};

// One-per-OS constructor (§4.4). Implemented in the platform/<os>/ .cpp
// for whichever OS this binary is built for; CMake compiles exactly one
// of those implementation files into core-lib per platform.
std::unique_ptr<IGlobalHotkeyManager> createGlobalHotkeyManager();

} // namespace lancue::platform
