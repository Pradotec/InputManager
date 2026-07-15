#pragma once

#include "input_manager/core/device.hpp"
#include "input_manager/transport/serial_port.hpp"
#include <mutex>
#include <string>
#include <vector>

namespace im {

class Makcu : public InputDevice {
public:
    explicit Makcu(const std::string& port, uint32_t baud_rate = 128000);
    ~Makcu() override;

    // -- Connection --
    void connect() override;
    void disconnect() override;
    bool is_connected() const override;

    // -- Mouse --
    void mouse_move(int32_t dx, int32_t dy) override;
    void mouse_move_absolute(int32_t x, int32_t y) override;
    void mouse_move_smooth(int32_t dx, int32_t dy, uint32_t duration_ms) override;
    void mouse_press(MouseButton button) override;
    void mouse_release(MouseButton button) override;
    void mouse_click(MouseButton button = MouseButton::Left) override;
    void mouse_double_click(MouseButton button = MouseButton::Left) override;
    void mouse_scroll(int32_t delta) override;

    // -- Keyboard --
    void key_press(KeyCode key, KeyModifier modifiers = KeyModifier::None) override;
    void key_release(KeyCode key) override;
    void key_tap(KeyCode key, KeyModifier modifiers = KeyModifier::None) override;
    void key_release_all() override;
    void type_string(const std::string& text, uint32_t interval_ms = 20) override;

    // -- Device --
    std::string device_name() const override;
    DeviceInfo get_info() override;
    void reboot() override;

    // -- Makcu specific --
    void set_dpi(uint16_t dpi);
    void set_poll_rate(uint16_t rate_hz);

private:
    static constexpr uint8_t HEADER = 0xFA;

    enum MCmd : uint8_t {
        MCMD_MOUSE_MOVE    = 0x01,
        MCMD_MOUSE_BUTTON  = 0x02,
        MCMD_MOUSE_WHEEL   = 0x03,
        MCMD_KEYBOARD      = 0x04,
        MCMD_RELEASE_ALL   = 0x05,
        MCMD_MOUSE_AUTO    = 0x0A,
        MCMD_SET_CONFIG    = 0x10,
        MCMD_DEVICE_INFO   = 0x20,
        MCMD_REBOOT        = 0x30,
    };

    void send_command(MCmd cmd, const std::vector<uint8_t>& data = {});
    std::vector<uint8_t> send_and_receive(MCmd cmd, const std::vector<uint8_t>& data = {});
    std::vector<uint8_t> build_packet(MCmd cmd, const std::vector<uint8_t>& data) const;

    std::string port_name_;
    uint32_t baud_rate_;
    SerialPort serial_;
    uint8_t current_buttons_ = 0;
    uint8_t current_modifier_ = 0;
    uint8_t current_keys_[6] = {};
    std::mutex mutex_;
};

} // namespace im
