#pragma once

#include "input_manager/core/types.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace im {

class InputDevice {
public:
    virtual ~InputDevice() = default;

    // -- Connection ----------------------------------------------------------
    virtual void connect() = 0;
    virtual void disconnect() = 0;
    virtual bool is_connected() const = 0;

    // -- Mouse ---------------------------------------------------------------
    virtual void mouse_move(int32_t dx, int32_t dy) = 0;
    virtual void mouse_move_absolute(int32_t x, int32_t y) = 0;
    virtual void mouse_move_smooth(int32_t dx, int32_t dy, uint32_t duration_ms) = 0;

    virtual void mouse_press(MouseButton button) = 0;
    virtual void mouse_release(MouseButton button) = 0;
    virtual void mouse_click(MouseButton button = MouseButton::Left) = 0;
    virtual void mouse_double_click(MouseButton button = MouseButton::Left) = 0;

    virtual void mouse_scroll(int32_t delta) = 0;

    // -- Keyboard ------------------------------------------------------------
    virtual void key_press(KeyCode key, KeyModifier modifiers = KeyModifier::None) = 0;
    virtual void key_release(KeyCode key) = 0;
    virtual void key_tap(KeyCode key, KeyModifier modifiers = KeyModifier::None) = 0;
    virtual void key_release_all() = 0;

    virtual void type_string(const std::string& text, uint32_t interval_ms = 20) = 0;

    // -- Device info ---------------------------------------------------------
    virtual std::string device_name() const = 0;
    virtual DeviceInfo get_info() = 0;
    virtual void reboot() = 0;

protected:
    void require_connected() const;
    bool connected_ = false;
};

} // namespace im
