#pragma once

#include "input_manager/core/device.hpp"
#include "input_manager/transport/udp_client.hpp"
#include <array>
#include <mutex>
#include <string>
#include <vector>

namespace im {

// ── KMBox Net ───────────────────────────────────────────────────────────────
//
// Network-controlled input device over UDP.
// Initialized with init(ip, port, uuid).
// Supports AES-128-ECB encrypted command variants (enc_*).
// ~1000 calls/sec throughput over 100M network.
//
// Button state: 0 = release, 1 = press
// Mouse x,y range: -32768 to 32768
// Wheel range: -128 to 128

class KMBoxNet : public InputDevice {
public:
    KMBoxNet(const std::string& ip, uint16_t port, const std::string& uuid);
    ~KMBoxNet() override;

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

    // ── KMBox Net specific ──────────────────────────────────────────────

    // encryption — toggle AES-128-ECB encryption for all commands
    void set_encryption(bool enabled);
    bool encryption_enabled() const;

    // smooth mouse movement over duration (linear interpolation)
    void move_auto(int32_t x, int32_t y, uint32_t duration_ms);

    // bezier curve mouse movement with two control points
    void move_beizer(int32_t x, int32_t y, uint32_t duration_ms,
                     int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2);

    // combined mouse report — buttons bitmask + relative x,y + wheel
    void mouse_combined(uint8_t buttons, int32_t x, int32_t y, int32_t wheel);

    // individual button control (state: 0 = up, 1 = down)
    void left(int state);
    void right(int state);
    void middle(int state);
    void side1(int state);
    void side2(int state);
    void wheel(int direction);

    // keyboard by USB HID code
    void keydown(uint8_t hid_code);
    void keyup(uint8_t hid_code);

    // monitor physical input (port > 0 = enable, 0 = disable)
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

    // encrypted variants — always use AES regardless of global toggle
    void enc_move(int32_t x, int32_t y);
    void enc_left(int state);
    void enc_right(int state);
    void enc_middle(int state);
    void enc_side1(int state);
    void enc_side2(int state);
    void enc_wheel(int direction);

    // mouse mask — offset applied to physical mouse input
    void mask_mouse(int32_t x, int32_t y);
    void unmask_mouse();

    // keyboard mask — intercept a physical key
    void mask_keyboard(uint8_t hid_code);
    void unmask_keyboard();

private:
    static constexpr uint32_t MAGIC = 0x4E455400;
    static constexpr size_t HEADER_SIZE = 16;

    enum NetCmd : uint32_t {
        NET_CMD_CONNECT       = 0,
        NET_CMD_MOUSE_MOVE    = 1,
        NET_CMD_MOUSE_LEFT    = 2,
        NET_CMD_MOUSE_RIGHT   = 3,
        NET_CMD_MOUSE_MIDDLE  = 4,
        NET_CMD_MOUSE_WHEEL   = 5,
        NET_CMD_MOUSE_AUTO    = 6,
        NET_CMD_MOUSE_ALL     = 7,
        NET_CMD_KEYBOARD_DOWN = 8,
        NET_CMD_KEYBOARD_UP   = 9,
        NET_CMD_RELEASE_ALL   = 10,
        NET_CMD_MOUSE_BEIZER  = 11,
        NET_CMD_MONITOR       = 12,
        NET_CMD_ISDOWN        = 13,
        NET_CMD_REBOOT        = 14,
        NET_CMD_INFO          = 15,
        NET_CMD_MOUSE_SIDE1   = 16,
        NET_CMD_MOUSE_SIDE2   = 17,
        NET_CMD_MASK_MOUSE    = 18,
        NET_CMD_UNMASK_MOUSE  = 19,
        NET_CMD_MASK_KB       = 20,
        NET_CMD_UNMASK_KB     = 21,
    };

    void send_packet(NetCmd cmd, const std::vector<uint8_t>& payload = {},
                     bool force_encrypt = false);
    std::vector<uint8_t> send_and_receive(NetCmd cmd,
                                          const std::vector<uint8_t>& payload = {},
                                          bool force_encrypt = false);
    std::vector<uint8_t> build_header(NetCmd cmd, uint32_t data_len) const;
    std::vector<uint8_t> encrypt_data(const std::vector<uint8_t>& data) const;
    std::vector<uint8_t> decrypt_data(const std::vector<uint8_t>& data) const;

    std::string ip_;
    uint16_t port_;
    std::string uuid_;
    UdpClient udp_;
    bool encryption_enabled_ = false;
    std::array<uint8_t, 16> aes_key_ {};
    uint8_t current_buttons_ = 0;
    uint8_t current_modifier_ = 0;
    uint8_t current_keys_[6] = {};
    std::mutex mutex_;
};

} // namespace im
