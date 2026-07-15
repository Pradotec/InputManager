#pragma once

#include <stdexcept>
#include <string>

namespace im {

class InputManagerError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class ConnectionError : public InputManagerError {
public:
    using InputManagerError::InputManagerError;
};

class DeviceTimeoutError : public InputManagerError {
public:
    using InputManagerError::InputManagerError;
};

class ProtocolError : public InputManagerError {
public:
    using InputManagerError::InputManagerError;
};

class DeviceNotFoundError : public InputManagerError {
public:
    using InputManagerError::InputManagerError;
};

class DeviceNotConnectedError : public InputManagerError {
public:
    using InputManagerError::InputManagerError;
};

} // namespace im
