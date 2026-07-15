#pragma once

#include "input_manager/core/device.hpp"
#include "input_manager/transport/serial_port.hpp"
#include <mutex>
#include <string>

namespace im {

// ── KMBox B / B+ / B Pro ────────────────────────────────────────────────────
//
// ASCII protocol over CH340 USB-serial.
// Default 115200 baud, configurable via km.baud().
// VID:PID configurable via device.VID() / device.PID().
//
// Command format:  km.<cmd>(<args>)\r\n
// Response:        <data>\r\n
//
// B+ APIs are compatible with B version.
// Device discovery: look for "USB-SERIAL CH340" COM port.

class KMBoxB : public InputDevice {
public:
    explicit KMBoxB(const std::string& port, uint32_t baud_rate = 115200);
    ~KMBoxB() override;

    // ── Connection ──────────────────────────────────────────────────────
    void connect() override;
    void disconnect() override;
    bool is_connected() const override;

    // ── Mouse ───────────────────────────────────────────────────────────
    void mouse_move(int32_t dx, int32_t dy) override;
    void mouse_move_absolute(int32_t x, int32_t y) override;
    void mouse_move_smooth(int32_t dx, int32_t dy, uint32_t duration_ms) override;
    void mouse_press(MouseButton button) override;
    void mouse_release(MouseButton button) override;
    void mouse_click(MouseButton button = MouseButton::Left) override;
    void mouse_double_click(MouseButton button = MouseButton::Left) override;
    void mouse_scroll(int32_t delta) override;

    // ── Keyboard ────────────────────────────────────────────────────────
    void key_press(KeyCode key, KeyModifier modifiers = KeyModifier::None) override;
    void key_release(KeyCode key) override;
    void key_tap(KeyCode key, KeyModifier modifiers = KeyModifier::None) override;
    void key_release_all() override;
    void type_string(const std::string& text, uint32_t interval_ms = 20) override;

    // ── Device ──────────────────────────────────────────────────────────
    std::string device_name() const override;
    DeviceInfo get_info() override;
    void reboot() override;

    // ── KMBox B specific ────────────────────────────────────────────────

    // mouse move with speed parameter (controls interpolation speed)
    void mouse_move_speed(int32_t dx, int32_t dy, int speed);

    // mouse mask — offset added to physical mouse input
    void set_mouse_mask(int32_t mask_x, int32_t mask_y);
    void clear_mouse_mask();

    // monitor physical input state (port > 0 = enable, 0 = disable)
    void monitor(int port);

    // query physical mouse button state
    bool isdown_left();
    bool isdown_right();
    bool isdown_middle();
    bool isdown_side1();
    bool isdown_side2();

    // query physical keyboard key state (USB HID code)
    bool isdown_key(uint8_t hid_code);
    bool isdown_key(KeyCode key);

    // change baud rate (takes effect after reboot)
    void set_baud(uint32_t baud);

    // LCD display text (B Pro feature)
    void lcd(const std::string& text);

    // set USB VID/PID (takes effect after reboot)
    void set_vid(const std::string& vid);
    void set_pid(const std::string& pid);

private:
    std::string send_command(const std::string& cmd);
    void send_command_no_response(const std::string& cmd);
    std::string read_response(uint32_t timeout_ms = 500);

    static const char* button_cmd_name(MouseButton button);

    std::string port_name_;
    uint32_t baud_rate_;
    SerialPort serial_;
    std::mutex mutex_;
};

} // namespace im
