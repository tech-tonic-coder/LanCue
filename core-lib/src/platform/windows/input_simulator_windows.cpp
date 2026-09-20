#include "input_simulator_windows.h"

#include <Windows.h>

namespace lancue::platform::windows {

namespace {

// Builds one key-down or key-up INPUT event for a virtual-key code.
// `vk`'s scan code is left at 0 — SendInput doesn't require a real scan
// code for a synthetic KEYEVENTF_KEYDOWN/KEYUP event the way a hardware-
// level injection would, and every real usage of SendInput for this exact
// "send a modifier+key chord" purpose does the same.
INPUT makeKeyInput(WORD vk, bool keyUp) {
    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    input.ki.dwFlags = keyUp ? KEYEVENTF_KEYUP : 0;
    return input;
}

// Sends Ctrl+<vk> as a single atomic SendInput batch: release every
// standard modifier (Ctrl/Alt/Shift/both Win keys) via synthetic key-up,
// then press Ctrl, press the key, release the key, release Ctrl —
// matching the down/down/up/up order a real keypress produces, since some
// applications' keyboard handling distinguishes "modifier already held
// when the key went down" from other orderings.
//
// The modifier release exists because this project's own hotkeys are
// themselves Ctrl/Alt/Shift/Win combinations (RegisterHotKey posts
// WM_HOTKEY as soon as the combo's final key goes down — Win32's own
// documentation confirms this, e.g. MOD_KEYUP existing specifically as an
// *opt-in* to instead wait for release — so by default the combo's
// modifiers are typically still physically held at the moment this
// fires). Sending a synthetic Ctrl+C chord without releasing them first
// means the target application sees the synthetic chord layered on top
// of whatever real modifier state is still active (e.g. Ctrl+Alt+C
// instead of a clean Ctrl+C), which it may not recognize as "copy" at
// all. This is a well-documented pitfall in hotkey/keyboard-automation
// tooling generally, not specific to this project — the AutoHotkey
// community's own standard remedy for exactly this "stuck modifier"
// symptom is the same fix applied here: an explicit key-up for the
// modifier before injecting new simulated input. A synthetic key-up for a
// key that isn't actually held is a safe no-op.
//
// All 9 events go through one SendInput call rather than a separate call
// for the release and the chord: a single call is inserted into the
// input stream as one unit, so there's no gap between "modifiers
// released" and "chord injected" where a real, concurrent hardware event
// could land and interleave with the synthetic sequence.
void sendCtrlChord(WORD vk) {
    INPUT inputs[9] = {
        makeKeyInput(VK_CONTROL, true), makeKeyInput(VK_MENU, true),  makeKeyInput(VK_SHIFT, true),
        makeKeyInput(VK_LWIN, true),    makeKeyInput(VK_RWIN, true),  makeKeyInput(VK_CONTROL, false),
        makeKeyInput(vk, false),        makeKeyInput(vk, true),       makeKeyInput(VK_CONTROL, true),
    };
    SendInput(9, inputs, sizeof(INPUT));
}

} // namespace

void InputSimulatorWindows::simulateCopy() {
    sendCtrlChord('C');
}

void InputSimulatorWindows::simulatePaste() {
    sendCtrlChord('V');
}

} // namespace lancue::platform::windows

namespace lancue::platform {

std::unique_ptr<IInputSimulator> createInputSimulator() {
    return std::make_unique<windows::InputSimulatorWindows>();
}

} // namespace lancue::platform
