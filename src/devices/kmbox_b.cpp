#include "input_manager/devices/kmbox_b.hpp"
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

// ---------------------------------------------------------------------------
// construction
// ---------------------------------------------------------------------------

KMBoxB::KMBoxB(const std::string& port, uint32_t baud_rate)
    : port_name_(port), baud_rate_(baud_rate) {}

KMBoxB::~KMBoxB() { disconnect(); }

// ---------------------------------------------------------------------------
// connection
// ---------------------------------------------------------------------------

void KMBoxB::connect() {
    std::lock_guard<std::mutex> lock(mutex_);
    serial_.open(port_name_, baud_rate_);
    connected_ = true;
    std::memset(current_keys_, 0, sizeof(current_keys_));
    current_buttons_ = 0;
    current_modifier_ = 0;
}

void KMBoxB::disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (connected_) {
        serial_.close();
        connected_ = false;
    }
}

bool KMBoxB::is_connected() const { return connected_; }

// ---------------------------------------------------------------------------
// protocol
// ---------------------------------------------------------------------------

std::vector<uint8_t> KMBoxB::build_packet(Cmd cmd,
                                           const std::vector<uint8_t>& data) const {
    std::vector<uint8_t> pkt;
    pkt.reserve(6 + data.size());
    pkt.push_back(HEADER_1);
    pkt.push_back(HEADER_2);
    pkt.push_back(static_cast<uint8_t>(cmd));
    uint16_t len = static_cast<uint16_t>(data.size());
    pkt.push_back(static_cast<uint8_t>(len & 0xFF));
    pkt.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
    pkt.insert(pkt.end(), data.begin(), data.end());

    uint8_t sum = 0;
    for (auto b : pkt) sum += b;
    pkt.push_back(sum);

    return pkt;
}

void KMBoxB::send_command(Cmd cmd, const std::vector<uint8_t>& data) {
    require_connected();
    auto pkt = build_packet(cmd, data);
    serial_.write(pkt);
    serial_.flush();
}

// ---------------------------------------------------------------------------
// mouse
// ---------------------------------------------------------------------------

void KMBoxB::mouse_move(int32_t dx, int32_t dy) {
    std::lock_guard<std::mutex> lock(mutex_);
    dx = std::clamp(dx, -32768, 32767);
    dy = std::clamp(dy, -32768, 32767);
    std::vector<uint8_t> data;
    push_i16(data, static_cast<int16_t>(dx));
    push_i16(data, static_cast<int16_t>(dy));
    send_command(CMD_MOUSE_MOVE, data);
}

void KMBoxB::mouse_move_absolute(int32_t x, int32_t y) {
    mouse_move(x, y);
}

void KMBoxB::mouse_move_smooth(int32_t dx, int32_t dy, uint32_t duration_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> data;
    push_i16(data, static_cast<int16_t>(std::clamp(dx, -32768, 32767)));
    push_i16(data, static_cast<int16_t>(std::clamp(dy, -32768, 32767)));
    push_u16(data, static_cast<uint16_t>(duration_ms));
    send_command(CMD_MOUSE_AUTO, data);
}

void KMBoxB::mouse_press(MouseButton button) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_buttons_ |= static_cast<uint8_t>(button);
    send_command(CMD_MOUSE_BUTTON, {current_buttons_});
}

void KMBoxB::mouse_release(MouseButton button) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_buttons_ &= ~static_cast<uint8_t>(button);
    send_command(CMD_MOUSE_BUTTON, {current_buttons_});
}

void KMBoxB::mouse_click(MouseButton button) {
    mouse_press(button);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    mouse_release(button);
}

void KMBoxB::mouse_double_click(MouseButton button) {
    mouse_click(button);
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    mouse_click(button);
}

void KMBoxB::mouse_scroll(int32_t delta) {
    std::lock_guard<std::mutex> lock(mutex_);
    int8_t d = static_cast<int8_t>(std::clamp(delta, -127, 127));
    send_command(CMD_MOUSE_WHEEL, {static_cast<uint8_t>(d)});
}

// ---------------------------------------------------------------------------
// keyboard
// ---------------------------------------------------------------------------

void KMBoxB::key_press(KeyCode key, KeyModifier modifiers) {
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

    std::vector<uint8_t> data = {current_modifier_, 0x00};
    data.insert(data.end(), current_keys_, current_keys_ + 6);
    send_command(CMD_KEYBOARD, data);
}

void KMBoxB::key_release(KeyCode key) {
    std::lock_guard<std::mutex> lock(mutex_);
    uint8_t code = static_cast<uint8_t>(key);
    for (int i = 0; i < 6; ++i) {
        if (current_keys_[i] == code) { current_keys_[i] = 0; break; }
    }

    std::vector<uint8_t> data = {current_modifier_, 0x00};
    data.insert(data.end(), current_keys_, current_keys_ + 6);
    send_command(CMD_KEYBOARD, data);
}

void KMBoxB::key_tap(KeyCode key, KeyModifier modifiers) {
    key_press(key, modifiers);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    key_release(key);
    if (modifiers != KeyModifier::None) {
        std::lock_guard<std::mutex> lock(mutex_);
        current_modifier_ &= ~static_cast<uint8_t>(modifiers);
        std::vector<uint8_t> data = {current_modifier_, 0x00};
        data.insert(data.end(), current_keys_, current_keys_ + 6);
        send_command(CMD_KEYBOARD, data);
    }
}

void KMBoxB::key_release_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    current_modifier_ = 0;
    std::memset(current_keys_, 0, sizeof(current_keys_));
    send_command(CMD_RELEASE_ALL);
}

void KMBoxB::type_string(const std::string& text, uint32_t interval_ms) {
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

std::string KMBoxB::device_name() const { return "KMBox B"; }

DeviceInfo KMBoxB::get_info() {
    std::lock_guard<std::mutex> lock(mutex_);
    send_command(CMD_INFO);
    try {
        auto resp = serial_.read_exact(16, 1000);
        DeviceInfo info;
        info.device_type = "KMBox B";
        if (resp.size() >= 6) {
            info.firmware_version = std::to_string(resp[3]) + "." +
                                    std::to_string(resp[4]) + "." +
                                    std::to_string(resp[5]);
        }
        return info;
    } catch (...) {
        return {"KMBox B", "unknown", ""};
    }
}

void KMBoxB::reboot() {
    std::lock_guard<std::mutex> lock(mutex_);
    send_command(CMD_REBOOT);
}

void KMBoxB::set_mouse_mask(int32_t mask_x, int32_t mask_y) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> data;
    push_i16(data, static_cast<int16_t>(mask_x));
    push_i16(data, static_cast<int16_t>(mask_y));
    send_command(CMD_MOUSE_MASK, data);
}

} // namespace im
