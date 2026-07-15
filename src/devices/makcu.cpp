#include "input_manager/devices/makcu.hpp"
#include "input_manager/core/errors.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <thread>

namespace im {

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

static void push_i16(std::vector<uint8_t>& v, int16_t val) {
    v.push_back(static_cast<uint8_t>(val & 0xFF));
    v.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
}

static void push_u16(std::vector<uint8_t>& v, uint16_t val) {
    v.push_back(static_cast<uint8_t>(val & 0xFF));
    v.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
}

static void push_i32_le(std::vector<uint8_t>& v, int32_t val) {
    v.push_back(static_cast<uint8_t>( val        & 0xFF));
    v.push_back(static_cast<uint8_t>((val >>  8) & 0xFF));
    v.push_back(static_cast<uint8_t>((val >> 16) & 0xFF));
    v.push_back(static_cast<uint8_t>((val >> 24) & 0xFF));
}

// ---------------------------------------------------------------------------
// construction
// ---------------------------------------------------------------------------

Makcu::Makcu(const std::string& port, uint32_t baud_rate)
    : port_name_(port), baud_rate_(baud_rate) {}

Makcu::~Makcu() { disconnect(); }

// ---------------------------------------------------------------------------
// connection
// ---------------------------------------------------------------------------

void Makcu::connect() {
    std::lock_guard<std::mutex> lock(mutex_);
    serial_.open(port_name_, baud_rate_);
    connected_ = true;
    std::memset(current_keys_, 0, sizeof(current_keys_));
    current_buttons_ = 0;
    current_modifier_ = 0;
}

void Makcu::disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (connected_) {
        serial_.close();
        connected_ = false;
    }
}

bool Makcu::is_connected() const { return connected_; }

// ---------------------------------------------------------------------------
// protocol — packet format: [0xFA] [cmd] [len] [data...] [xor_checksum]
// ---------------------------------------------------------------------------

std::vector<uint8_t> Makcu::build_packet(MCmd cmd,
                                          const std::vector<uint8_t>& data) const {
    std::vector<uint8_t> pkt;
    pkt.reserve(4 + data.size());
    pkt.push_back(HEADER);
    pkt.push_back(static_cast<uint8_t>(cmd));
    pkt.push_back(static_cast<uint8_t>(data.size()));
    pkt.insert(pkt.end(), data.begin(), data.end());

    uint8_t xor_sum = 0;
    for (auto b : pkt) xor_sum ^= b;
    pkt.push_back(xor_sum);

    return pkt;
}

void Makcu::send_command(MCmd cmd, const std::vector<uint8_t>& data) {
    require_connected();
    auto pkt = build_packet(cmd, data);
    serial_.write(pkt);
    serial_.flush();
}

std::vector<uint8_t> Makcu::send_and_receive(MCmd cmd,
                                              const std::vector<uint8_t>& data) {
    send_command(cmd, data);
    try {
        auto hdr = serial_.read_exact(3, 1000);
        if (hdr[0] != HEADER)
            throw ProtocolError("Makcu: invalid response header");
        uint8_t len = hdr[2];
        auto body = serial_.read_exact(len + 1, 1000); // data + checksum
        return std::vector<uint8_t>(body.begin(), body.begin() + len);
    } catch (const DeviceTimeoutError&) {
        return {};
    }
}

// ---------------------------------------------------------------------------
// mouse
// ---------------------------------------------------------------------------

void Makcu::mouse_move(int32_t dx, int32_t dy) {
    std::lock_guard<std::mutex> lock(mutex_);
    dx = std::clamp(dx, -32768, 32767);
    dy = std::clamp(dy, -32768, 32767);
    std::vector<uint8_t> data;
    push_i16(data, static_cast<int16_t>(dx));
    push_i16(data, static_cast<int16_t>(dy));
    send_command(MCMD_MOUSE_MOVE, data);
}

void Makcu::mouse_move_absolute(int32_t x, int32_t y) {
    mouse_move(x, y);
}

void Makcu::mouse_move_smooth(int32_t dx, int32_t dy, uint32_t duration_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> data;
    push_i32_le(data, std::clamp(dx, -32768, 32767));
    push_i32_le(data, std::clamp(dy, -32768, 32767));
    push_u16(data, static_cast<uint16_t>(duration_ms));
    send_command(MCMD_MOUSE_AUTO, data);
}

void Makcu::mouse_press(MouseButton button) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_buttons_ |= static_cast<uint8_t>(button);
    send_command(MCMD_MOUSE_BUTTON, {current_buttons_});
}

void Makcu::mouse_release(MouseButton button) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_buttons_ &= ~static_cast<uint8_t>(button);
    send_command(MCMD_MOUSE_BUTTON, {current_buttons_});
}

void Makcu::mouse_click(MouseButton button) {
    mouse_press(button);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    mouse_release(button);
}

void Makcu::mouse_double_click(MouseButton button) {
    mouse_click(button);
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    mouse_click(button);
}

void Makcu::mouse_scroll(int32_t delta) {
    std::lock_guard<std::mutex> lock(mutex_);
    int8_t d = static_cast<int8_t>(std::clamp(delta, -127, 127));
    send_command(MCMD_MOUSE_WHEEL, {static_cast<uint8_t>(d)});
}

// ---------------------------------------------------------------------------
// keyboard
// ---------------------------------------------------------------------------

void Makcu::key_press(KeyCode key, KeyModifier modifiers) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_modifier_ |= static_cast<uint8_t>(modifiers);

    uint8_t code = static_cast<uint8_t>(key);
    bool already = false;
    for (int i = 0; i < 6; ++i) {
        if (current_keys_[i] == code) { already = true; break; }
    }
    if (!already) {
        for (int i = 0; i < 6; ++i) {
            if (current_keys_[i] == 0) { current_keys_[i] = code; break; }
        }
    }

    std::vector<uint8_t> data = {current_modifier_};
    data.insert(data.end(), current_keys_, current_keys_ + 6);
    send_command(MCMD_KEYBOARD, data);
}

void Makcu::key_release(KeyCode key) {
    std::lock_guard<std::mutex> lock(mutex_);
    uint8_t code = static_cast<uint8_t>(key);
    for (int i = 0; i < 6; ++i) {
        if (current_keys_[i] == code) { current_keys_[i] = 0; break; }
    }
    std::vector<uint8_t> data = {current_modifier_};
    data.insert(data.end(), current_keys_, current_keys_ + 6);
    send_command(MCMD_KEYBOARD, data);
}

void Makcu::key_tap(KeyCode key, KeyModifier modifiers) {
    key_press(key, modifiers);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    key_release(key);
    if (modifiers != KeyModifier::None) {
        std::lock_guard<std::mutex> lock(mutex_);
        current_modifier_ &= ~static_cast<uint8_t>(modifiers);
        std::vector<uint8_t> data = {current_modifier_};
        data.insert(data.end(), current_keys_, current_keys_ + 6);
        send_command(MCMD_KEYBOARD, data);
    }
}

void Makcu::key_release_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    current_modifier_ = 0;
    std::memset(current_keys_, 0, sizeof(current_keys_));
    send_command(MCMD_RELEASE_ALL);
}

void Makcu::type_string(const std::string& text, uint32_t interval_ms) {
    const auto& cmap = char_map();
    for (char c : text) {
        auto it = cmap.find(c);
        if (it == cmap.end()) continue;
        key_tap(it->second.key, it->second.modifier);
        if (interval_ms > 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
    }
}

// ---------------------------------------------------------------------------
// device management
// ---------------------------------------------------------------------------

std::string Makcu::device_name() const { return "Makcu"; }

DeviceInfo Makcu::get_info() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto resp = send_and_receive(MCMD_DEVICE_INFO);
    DeviceInfo info;
    info.device_type = "Makcu";
    if (resp.size() >= 3) {
        info.firmware_version = std::to_string(resp[0]) + "." +
                                std::to_string(resp[1]) + "." +
                                std::to_string(resp[2]);
    } else {
        info.firmware_version = "unknown";
    }
    return info;
}

void Makcu::reboot() {
    std::lock_guard<std::mutex> lock(mutex_);
    send_command(MCMD_REBOOT);
}

void Makcu::set_dpi(uint16_t dpi) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> data = {0x01};
    push_u16(data, dpi);
    send_command(MCMD_SET_CONFIG, data);
}

void Makcu::set_poll_rate(uint16_t rate_hz) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> data = {0x02};
    push_u16(data, rate_hz);
    send_command(MCMD_SET_CONFIG, data);
}

} // namespace im
