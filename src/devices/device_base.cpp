#include "input_manager/core/device.hpp"
#include "input_manager/core/errors.hpp"

namespace im {

void InputDevice::require_connected() const {
    if (!connected_)
        throw DeviceNotConnectedError("Device is not connected");
}

} // namespace im
