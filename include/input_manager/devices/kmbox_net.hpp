#pragma once

#include "input_manager/core/device.hpp"
#include "input_manager/transport/udp_client.hpp"
#include <array>
#include <mutex>
#include <string>
#include <vector>

namespace im {

class KMBoxNet : public InputDevice {
public:
    KMBoxNet(const std::string& ip, uint16_t port, const std::string& uuid);
    ~KMBoxNet() override;

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

    // -- KMBox Net specific --
    void set_encryption(bool enabled);
    void set_monitor_resolution(uint16_t width, uint16_t height);
    std::vector<uint8_t> monitor_capture();

private:
    static constexpr uint32_t MAGIC = 0x4E455400; // "NET\0"
    static constexpr size_t HEADER_SIZE = 16;

    enum NetCmd : uint32_t {
        NET_CMD_CONNECT    = 0,
        NET_CMD_MOUSE_MOVE = 1,
        NET_CMD_MOUSE_BTN  = 2,
        NET_CMD_MOUSE_WHEEL = 3,
        NET_CMD_KEYBOARD   = 4,
        NET_CMD_RELEASE    = 5,
        NET_CMD_MOUSE_AUTO = 6,
        NET_CMD_REBOOT     = 7,
        NET_CMD_CONFIG     = 8,
        NET_CMD_MONITOR    = 9,
        NET_CMD_MOUSE_ABS  = 10,
        NET_CMD_INFO       = 11,
    };

    void send_packet(NetCmd cmd, const std::vector<uint8_t>& payload = {});
    std::vector<uint8_t> send_and_receive(NetCmd cmd, const std::vector<uint8_t>& payload = {});
    std::vector<uint8_t> build_header(NetCmd cmd, uint32_t data_len) const;
    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& data) const;
    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& data) const;

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
