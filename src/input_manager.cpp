#include "input_manager/input_manager.hpp"
#include "input_manager/core/errors.hpp"

namespace im {

InputManager::~InputManager() {
    disconnect_all();
}

// ---------------------------------------------------------------------------
// registration
// ---------------------------------------------------------------------------

void InputManager::add_kmbox_b(const std::string& name, const std::string& port,
                                uint32_t baud_rate) {
    add_device(name, std::make_unique<KMBoxB>(port, baud_rate));
}

void InputManager::add_kmbox_net(const std::string& name, const std::string& ip,
                                  uint16_t port, const std::string& uuid) {
    add_device(name, std::make_unique<KMBoxNet>(ip, port, uuid));
}

void InputManager::add_makcu(const std::string& name, const std::string& port,
                              uint32_t baud_rate) {
    add_device(name, std::make_unique<Makcu>(port, baud_rate));
}

void InputManager::add_device(const std::string& name,
                               std::unique_ptr<InputDevice> device) {
    if (devices_.count(name))
        throw InputManagerError("Device already registered: " + name);
    devices_[name] = std::move(device);
    if (active_name_.empty())
        active_name_ = name;
}

// ---------------------------------------------------------------------------
// access
// ---------------------------------------------------------------------------

InputDevice& InputManager::get(const std::string& name) {
    auto it = devices_.find(name);
    if (it == devices_.end())
        throw DeviceNotFoundError("No device named: " + name);
    return *it->second;
}

const InputDevice& InputManager::get(const std::string& name) const {
    auto it = devices_.find(name);
    if (it == devices_.end())
        throw DeviceNotFoundError("No device named: " + name);
    return *it->second;
}

InputDevice& InputManager::operator[](const std::string& name) {
    return get(name);
}

bool InputManager::has(const std::string& name) const {
    return devices_.count(name) > 0;
}

void InputManager::remove(const std::string& name) {
    auto it = devices_.find(name);
    if (it == devices_.end()) return;
    if (it->second->is_connected())
        it->second->disconnect();
    devices_.erase(it);
    if (active_name_ == name) {
        active_name_ = devices_.empty() ? "" : devices_.begin()->first;
    }
}

std::vector<std::string> InputManager::device_names() const {
    std::vector<std::string> names;
    names.reserve(devices_.size());
    for (const auto& [name, _] : devices_)
        names.push_back(name);
    return names;
}

size_t InputManager::device_count() const {
    return devices_.size();
}

// ---------------------------------------------------------------------------
// bulk ops
// ---------------------------------------------------------------------------

void InputManager::connect_all() {
    for (auto& [_, dev] : devices_) {
        if (!dev->is_connected())
            dev->connect();
    }
}

void InputManager::disconnect_all() {
    for (auto& [_, dev] : devices_) {
        if (dev->is_connected())
            dev->disconnect();
    }
}

void InputManager::mouse_move_all(int32_t dx, int32_t dy) {
    for (auto& [_, dev] : devices_) {
        if (dev->is_connected())
            dev->mouse_move(dx, dy);
    }
}

void InputManager::key_release_all_devices() {
    for (auto& [_, dev] : devices_) {
        if (dev->is_connected())
            dev->key_release_all();
    }
}

// ---------------------------------------------------------------------------
// active device
// ---------------------------------------------------------------------------

void InputManager::set_active(const std::string& name) {
    if (!has(name))
        throw DeviceNotFoundError("No device named: " + name);
    active_name_ = name;
}

InputDevice& InputManager::active() {
    if (active_name_.empty())
        throw InputManagerError("No active device set");
    return get(active_name_);
}

const std::string& InputManager::active_name() const {
    return active_name_;
}

} // namespace im
