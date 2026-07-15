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

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
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

    uint8_t junk[256];
    serial_.read(junk, sizeof(junk), 100);
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
// protocol — ASCII commands: km.<cmd>(<args>)\r\n
// ---------------------------------------------------------------------------

std::string KMBoxB::read_response(uint32_t timeout_ms) {
    std::string result;
    auto deadline = std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(timeout_ms);

    while (std::chrono::steady_clock::now() < deadline) {
        uint8_t buf[256];
        size_t n = serial_.read(buf, sizeof(buf), 50);
        if (n > 0) {
            result.append(reinterpret_cast<char*>(buf), n);
            if (result.size() >= 2 && result.back() == '\n')
                return result;
        }
    }
    return result;
}

std::string KMBoxB::send_command(const std::string& cmd) {
    require_connected();
    std::string full = cmd + "\r\n";
    serial_.write(reinterpret_cast<const uint8_t*>(full.data()), full.size());
    serial_.flush();
    std::string resp = read_response(500);
    auto nl = resp.find('\n');
    if (nl != std::string::npos)
        return trim(resp.substr(nl + 1));
    return trim(resp);
}

void KMBoxB::send_command_no_response(const std::string& cmd) {
    require_connected();
    std::string full = cmd + "\r\n";
    serial_.write(reinterpret_cast<const uint8_t*>(full.data()), full.size());
    serial_.flush();
}

const char* KMBoxB::button_cmd_name(MouseButton button) {
    if (has_flag(button, MouseButton::Left))   return "left";
    if (has_flag(button, MouseButton::Right))  return "right";
    if (has_flag(button, MouseButton::Middle)) return "middle";
    if (has_flag(button, MouseButton::Side1))  return "side1";
    if (has_flag(button, MouseButton::Side2))  return "side2";
    return "left";
}

// ---------------------------------------------------------------------------
// mouse — km.move(x,y), km.left(state), km.right(state), etc.
// ---------------------------------------------------------------------------

void KMBoxB::mouse_move(int32_t dx, int32_t dy) {
    std::lock_guard<std::mutex> lock(mutex_);
    send_command("km.move(" + std::to_string(dx) + "," +
                 std::to_string(dy) + ")");
}

void KMBoxB::mouse_move_speed(int32_t dx, int32_t dy, int speed) {
    std::lock_guard<std::mutex> lock(mutex_);
    send_command("km.move(" + std::to_string(dx) + "," +
                 std::to_string(dy) + "," + std::to_string(speed) + ")");
}

void KMBoxB::mouse_move_absolute(int32_t x, int32_t y) {
    mouse_move(x, y);
}

void KMBoxB::mouse_move_smooth(int32_t dx, int32_t dy, uint32_t duration_ms) {
    const int steps = std::max(1, static_cast<int>(duration_ms / 5));
    const int step_dx = dx / steps;
    const int step_dy = dy / steps;
    int remaining_x = dx;
    int remaining_y = dy;

    for (int i = 0; i < steps - 1; ++i) {
        mouse_move(step_dx, step_dy);
        remaining_x -= step_dx;
        remaining_y -= step_dy;
        std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms / steps));
    }
    if (remaining_x != 0 || remaining_y != 0)
        mouse_move(remaining_x, remaining_y);
}

void KMBoxB::mouse_press(MouseButton button) {
    std::lock_guard<std::mutex> lock(mutex_);
    send_command(std::string("km.") + button_cmd_name(button) + "(1)");
}

void KMBoxB::mouse_release(MouseButton button) {
    std::lock_guard<std::mutex> lock(mutex_);
    send_command(std::string("km.") + button_cmd_name(button) + "(0)");
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
    send_command("km.wheel(" + std::to_string(delta) + ")");
}

// ---------------------------------------------------------------------------
// keyboard — km.keydown(hid_code), km.keyup(hid_code)
// ---------------------------------------------------------------------------

void KMBoxB::key_press(KeyCode key, KeyModifier modifiers) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (modifiers != KeyModifier::None) {
        if (static_cast<uint8_t>(modifiers & KeyModifier::LeftCtrl))
            send_command("km.keydown(224)");
        if (static_cast<uint8_t>(modifiers & KeyModifier::LeftShift))
            send_command("km.keydown(225)");
        if (static_cast<uint8_t>(modifiers & KeyModifier::LeftAlt))
            send_command("km.keydown(226)");
        if (static_cast<uint8_t>(modifiers & KeyModifier::LeftGui))
            send_command("km.keydown(227)");
        if (static_cast<uint8_t>(modifiers & KeyModifier::RightCtrl))
            send_command("km.keydown(228)");
        if (static_cast<uint8_t>(modifiers & KeyModifier::RightShift))
            send_command("km.keydown(229)");
        if (static_cast<uint8_t>(modifiers & KeyModifier::RightAlt))
            send_command("km.keydown(230)");
        if (static_cast<uint8_t>(modifiers & KeyModifier::RightGui))
            send_command("km.keydown(231)");
    }
    send_command("km.keydown(" + std::to_string(static_cast<int>(key)) + ")");
}

void KMBoxB::key_release(KeyCode key) {
    std::lock_guard<std::mutex> lock(mutex_);
    send_command("km.keyup(" + std::to_string(static_cast<int>(key)) + ")");
}

void KMBoxB::key_tap(KeyCode key, KeyModifier modifiers) {
    key_press(key, modifiers);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    key_release(key);
    if (modifiers != KeyModifier::None) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (static_cast<uint8_t>(modifiers & KeyModifier::LeftCtrl))
            send_command("km.keyup(224)");
        if (static_cast<uint8_t>(modifiers & KeyModifier::LeftShift))
            send_command("km.keyup(225)");
        if (static_cast<uint8_t>(modifiers & KeyModifier::LeftAlt))
            send_command("km.keyup(226)");
        if (static_cast<uint8_t>(modifiers & KeyModifier::LeftGui))
            send_command("km.keyup(227)");
        if (static_cast<uint8_t>(modifiers & KeyModifier::RightCtrl))
            send_command("km.keyup(228)");
        if (static_cast<uint8_t>(modifiers & KeyModifier::RightShift))
            send_command("km.keyup(229)");
        if (static_cast<uint8_t>(modifiers & KeyModifier::RightAlt))
            send_command("km.keyup(230)");
        if (static_cast<uint8_t>(modifiers & KeyModifier::RightGui))
            send_command("km.keyup(231)");
    }
}

void KMBoxB::key_release_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (int code = 224; code <= 231; ++code)
        send_command("km.keyup(" + std::to_string(code) + ")");
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
    DeviceInfo info;
    info.device_type = "KMBox B";
    info.firmware_version = "unknown";
    return info;
}

void KMBoxB::reboot() {
    std::lock_guard<std::mutex> lock(mutex_);
    send_command_no_response("km.reboot()");
    connected_ = false;
}

// ---------------------------------------------------------------------------
// KMBox B specific — mask, monitor, isdown, baud, lcd, VID/PID
// ---------------------------------------------------------------------------

void KMBoxB::set_mouse_mask(int32_t mask_x, int32_t mask_y) {
    std::lock_guard<std::mutex> lock(mutex_);
    send_command("km.mask(" + std::to_string(mask_x) + "," +
                 std::to_string(mask_y) + ")");
}

void KMBoxB::clear_mouse_mask() {
    set_mouse_mask(0, 0);
}

void KMBoxB::monitor(int port) {
    std::lock_guard<std::mutex> lock(mutex_);
    send_command("km.monitor(" + std::to_string(port) + ")");
}

bool KMBoxB::isdown_left() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto resp = send_command("km.isdown(left)");
    return resp == "1" || resp == "true";
}

bool KMBoxB::isdown_right() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto resp = send_command("km.isdown(right)");
    return resp == "1" || resp == "true";
}

bool KMBoxB::isdown_middle() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto resp = send_command("km.isdown(middle)");
    return resp == "1" || resp == "true";
}

bool KMBoxB::isdown_side1() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto resp = send_command("km.isdown(side1)");
    return resp == "1" || resp == "true";
}

bool KMBoxB::isdown_side2() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto resp = send_command("km.isdown(side2)");
    return resp == "1" || resp == "true";
}

void KMBoxB::set_baud(uint32_t baud) {
    std::lock_guard<std::mutex> lock(mutex_);
    send_command("km.baud(" + std::to_string(baud) + ")");
}

void KMBoxB::lcd(const std::string& text) {
    std::lock_guard<std::mutex> lock(mutex_);
    send_command("km.lcd('" + text + "')");
}

void KMBoxB::set_vid(const std::string& vid) {
    std::lock_guard<std::mutex> lock(mutex_);
    send_command("device.VID('" + vid + "')");
}

void KMBoxB::set_pid(const std::string& pid) {
    std::lock_guard<std::mutex> lock(mutex_);
    send_command("device.PID('" + pid + "')");
}

} // namespace im
