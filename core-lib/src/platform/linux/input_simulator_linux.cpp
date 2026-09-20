#include "input_simulator_linux.h"

#include "corelib/logging/logger.h"

namespace lancue::platform::linux_ {

void InputSimulatorLinux::simulateCopy() {
    lancue::logError(QStringLiteral("InputSimulatorLinux: not yet implemented — see the class comment for the "
                                     "planned XTestFakeKeyEvent-based approach."));
}

void InputSimulatorLinux::simulatePaste() {
    lancue::logError(QStringLiteral("InputSimulatorLinux: not yet implemented — see the class comment for the "
                                     "planned XTestFakeKeyEvent-based approach."));
}

} // namespace lancue::platform::linux_

namespace lancue::platform {

std::unique_ptr<IInputSimulator> createInputSimulator() {
    return std::make_unique<linux_::InputSimulatorLinux>();
}

} // namespace lancue::platform
