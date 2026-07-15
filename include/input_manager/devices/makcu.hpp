#pragma once

#include "input_manager/core/device.hpp"
#include "input_manager/transport/serial_port.hpp"
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace im {

// ── Makcu lock targets ──────────────────────────────────────────────────────
enum class MakcuLockTarget {
    MX,   // mouse X axis
    MY,   // mouse Y axis
    MW,   // mouse wheel
    ML,   // left button
    MM,   // middle button
    MR,   // right button
    MS1,  // side button 1
    MS2,  // side button 2
};

// ── Makcu button identifiers ────────────────────────────────────────────────
enum class MakcuButton : uint8_t {
    Left   = 1,
    Right  = 2,
    Middle = 3,
    Mouse4 = 4,
    Mouse5 = 5,
};

// ── Makcu stream mode ───────────────────────────────────────────────────────
enum class MakcuStreamMode : uint8_t {
    Off = 0,
    Raw = 1,
    Mut = 2,
};

// ── Makcu device info ───────────────────────────────────────────────────────
struct MakcuDeviceInfo {
    std::string mac;
    std::string firmware;
    std::string cpu;
    std::string vendor;
    std::string model;
    std::string serial;
    int temp_c       = 0;
    int ram_free     = 0;
    int uptime_s     = 0;
    std::string vid;
    std::string pid;
};

// ── Makcu device ────────────────────────────────────────────────────────────
//
// ASCII protocol over CH343 USB-serial.
// Default 115200 baud, switchable to 4 Mbps.
// VID:PID = 1A86:55D3
//
// Command format:  km.<cmd>(<args>)\r\n
// Response:        <echo/data>\r\n>>> \r\n
//

class Makcu : public InputDevice {
public:
    // CH343 supports up to 4 Mbps — common rates: 115200, 921600, 4000000
    explicit Makcu(const std::string& port, uint32_t baud_rate = 115200);
    ~Makcu() override;

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

    // ── Makcu-specific ──────────────────────────────────────────────────

    // silent move: left-down -> move -> left-up (two HID frames)
    void silent_move(int32_t dx, int32_t dy);

    // click with count and optional delay (default random 35-75ms)
    void click(MakcuButton button, int count = 1, int delay_ms = -1);

    // turbo: rapid-fire mode for a button (delay 1-5000ms, 0 to disable)
    void turbo(MakcuButton button, int delay_ms);
    void turbo_disable_all();

    // lock / unlock axes and buttons
    void lock(MakcuLockTarget target);
    void unlock(MakcuLockTarget target);
    int  lock_state(MakcuLockTarget target);
    std::unordered_map<std::string, int> lock_states_all();

    // streaming mouse data
    void stream_set(MakcuStreamMode mode, int period_ms = 0);
    MakcuStreamMode stream_mode();

    // echo control (suppress command echo in responses)
    void echo(bool enabled);

    // device serial
    std::string serial_number();
    void set_serial(const std::string& serial);

    // full device info
    MakcuDeviceInfo device_info_full();

    // firmware version string
    std::string firmware_version();

    // change baud rate (common: 115200, 921600, 4000000)
    void set_baud(uint32_t baud);

    // keyboard helpers using key names
    void key_down(const std::string& key_name);
    void key_up(const std::string& key_name);
    void key_press_name(const std::string& key_name);

private:
    static constexpr const char* PROMPT = ">>> ";

    std::string send_command(const std::string& cmd);
    void send_command_no_response(const std::string& cmd);
    std::string read_until_prompt(uint32_t timeout_ms = 1000);

    static const char* lock_target_str(MakcuLockTarget target);
    static uint8_t mouse_button_to_makcu(MouseButton button);

    std::string port_name_;
    uint32_t baud_rate_;
    SerialPort serial_;
    std::mutex mutex_;
};

} // namespace im
