#pragma once

#include <memory>

namespace lancue::platform {

// Abstract simulated keyboard input for the copy/paste round trip — one
// implementation per OS (§4.4), selected at build time via
// createInputSimulator() below, no #ifdef at call sites.
//
// Deliberately narrow: just the two gestures this project's own scope
// needs (§4.3) rather than a general "simulate any key" interface —
// nothing here asks for arbitrary key injection, only "copy the current
// selection" and "paste the clipboard", each of which is genuinely a
// single fixed modifier+key chord per OS (Ctrl+C/Ctrl+V on Windows/Linux,
// Cmd+C/Cmd+V on macOS). If a later phase ever needs to simulate
// something else, that's a new method here, not a reason to widen this
// one into a generic key-injection API today.
class IInputSimulator {
public:
    virtual ~IInputSimulator() = default;

    // Simulates the OS's native "copy" chord (Ctrl+C / Cmd+C). Injected
    // into whichever application currently has keyboard focus — this
    // interface has no notion of *which* application that is or needs
    // one, since the OS routes simulated input the same way it routes
    // real input.
    virtual void simulateCopy() = 0;

    // Simulates the OS's native "paste" chord (Ctrl+V / Cmd+V).
    virtual void simulatePaste() = 0;
};

// One-per-OS constructor (§4.4). Implemented in the platform/<os>/ .cpp
// for whichever OS this binary is built for; CMake compiles exactly one
// of those implementation files into core-lib per platform.
std::unique_ptr<IInputSimulator> createInputSimulator();

} // namespace lancue::platform
