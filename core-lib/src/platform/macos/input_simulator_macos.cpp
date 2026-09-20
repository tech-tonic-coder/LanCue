#include "input_simulator_macos.h"

#include "corelib/logging/logger.h"

namespace lancue::platform::macos {

void InputSimulatorMacos::simulateCopy() {
    lancue::logError(QStringLiteral("InputSimulatorMacos: not yet implemented — see the class comment for the "
                                     "planned CGEventCreateKeyboardEvent-based approach."));
}

void InputSimulatorMacos::simulatePaste() {
    lancue::logError(QStringLiteral("InputSimulatorMacos: not yet implemented — see the class comment for the "
                                     "planned CGEventCreateKeyboardEvent-based approach."));
}

} // namespace lancue::platform::macos

namespace lancue::platform {

std::unique_ptr<IInputSimulator> createInputSimulator() {
    return std::make_unique<macos::InputSimulatorMacos>();
}

} // namespace lancue::platform
