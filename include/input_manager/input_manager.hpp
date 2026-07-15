#pragma once

#include "input_manager/core/types.hpp"
#include "input_manager/core/errors.hpp"
#include "input_manager/core/device.hpp"
#include "input_manager/devices/kmbox_b.hpp"
#include "input_manager/devices/kmbox_net.hpp"
#include "input_manager/devices/makcu.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace im {

class InputManager {
public:
    InputManager() = default;
    ~InputManager();

    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;

    // -- Device registration -------------------------------------------------

    void add_kmbox_b(const std::string& name, const std::string& port,
                     uint32_t baud_rate = 115200);

    void add_kmbox_net(const std::string& name, const std::string& ip,
                       uint16_t port, const std::string& uuid);

    void add_makcu(const std::string& name, const std::string& port,
                   uint32_t baud_rate = 128000);

    void add_device(const std::string& name, std::unique_ptr<InputDevice> device);

    // -- Device access -------------------------------------------------------

    InputDevice& get(const std::string& name);
    const InputDevice& get(const std::string& name) const;

    InputDevice& operator[](const std::string& name);

    template <typename T>
    T& get_as(const std::string& name) {
        return dynamic_cast<T&>(get(name));
    }

    bool has(const std::string& name) const;
    void remove(const std::string& name);
    std::vector<std::string> device_names() const;
    size_t device_count() const;

    // -- Bulk operations -----------------------------------------------------

    void connect_all();
    void disconnect_all();

    void mouse_move_all(int32_t dx, int32_t dy);
    void key_release_all_devices();

    // -- Active device (optional convenience) --------------------------------

    void set_active(const std::string& name);
    InputDevice& active();
    const std::string& active_name() const;

private:
    std::unordered_map<std::string, std::unique_ptr<InputDevice>> devices_;
    std::string active_name_;
};

} // namespace im
