#include "input_manager/devices/makcu.hpp"
#include "input_manager/core/errors.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <sstream>
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

    // flush any pending data
    uint8_t junk[256];
    serial_.read(junk, sizeof(junk), 100);
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
// protocol — ASCII commands: km.<cmd>(<args>)\r\n
//            response ends with ">>> " prompt
// ---------------------------------------------------------------------------

std::string Makcu::read_until_prompt(uint32_t timeout_ms) {
    std::string result;
    auto deadline = std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(timeout_ms);

    while (std::chrono::steady_clock::now() < deadline) {
        uint8_t buf[256];
        size_t n = serial_.read(buf, sizeof(buf), 50);
        if (n > 0) {
            result.append(reinterpret_cast<char*>(buf), n);
            if (result.size() >= 4) {
                auto pos = result.find(PROMPT);
                if (pos != std::string::npos) {
                    return result.substr(0, pos);
                }
            }
        }
    }
    return result;
}

std::string Makcu::send_command(const std::string& cmd) {
    require_connected();
    std::string full = cmd + "\r\n";
    serial_.write(reinterpret_cast<const uint8_t*>(full.data()), full.size());
    serial_.flush();
    std::string resp = read_until_prompt(1000);
    // strip the echoed command from the response
    auto nl = resp.find('\n');
    if (nl != std::string::npos)
        return trim(resp.substr(nl + 1));
    return trim(resp);
}

void Makcu::send_command_no_response(const std::string& cmd) {
    require_connected();
    std::string full = cmd + "\r\n";
    serial_.write(reinterpret_cast<const uint8_t*>(full.data()), full.size());
    serial_.flush();
}

// ---------------------------------------------------------------------------
// lock target names
// ---------------------------------------------------------------------------

const char* Makcu::lock_target_str(MakcuLockTarget target) {
    switch (target) {
        case MakcuLockTarget::MX:  return "mx";
        case MakcuLockTarget::MY:  return "my";
        case MakcuLockTarget::MW:  return "mw";
        case MakcuLockTarget::ML:  return "ml";
        case MakcuLockTarget::MM:  return "mm";
        case MakcuLockTarget::MR:  return "mr";
        case MakcuLockTarget::MS1: return "ms1";
        case MakcuLockTarget::MS2: return "ms2";
    }
    return "mx";
}

uint8_t Makcu::mouse_button_to_makcu(MouseButton button) {
    if (has_flag(button, MouseButton::Left))   return 1;
    if (has_flag(button, MouseButton::Right))  return 2;
    if (has_flag(button, MouseButton::Middle)) return 3;
    if (has_flag(button, MouseButton::Side1))  return 4;
    if (has_flag(button, MouseButton::Side2))  return 5;
    return 1;
}

// ---------------------------------------------------------------------------
// mouse
// ---------------------------------------------------------------------------

void Makcu::mouse_move(int32_t dx, int32_t dy) {
    std::lock_guard<std::mutex> lock(mutex_);
    send_command("km.move(" + std::to_string(dx) + "," + std::to_string(dy) + ")");
}

void Makcu::mouse_move_absolute(int32_t x, int32_t y) {
    // Makcu only supports relative movement
    mouse_move(x, y);
}

void Makcu::mouse_move_smooth(int32_t dx, int32_t dy, uint32_t duration_ms) {
    // Makcu has no native smooth move; interpolate with multiple km.move calls
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
    // final step: send remainder
    if (remaining_x != 0 || remaining_y != 0)
        mouse_move(remaining_x, remaining_y);
}

void Makcu::mouse_press(MouseButton button) {
    std::lock_guard<std::mutex> lock(mutex_);
    uint8_t btn = mouse_button_to_makcu(button);
    send_command("km.button_down(" + std::to_string(btn) + ")");
}

void Makcu::mouse_release(MouseButton button) {
    std::lock_guard<std::mutex> lock(mutex_);
    uint8_t btn = mouse_button_to_makcu(button);
    send_command("km.button_up(" + std::to_string(btn) + ")");
}

void Makcu::mouse_click(MouseButton button) {
    std::lock_guard<std::mutex> lock(mutex_);
    uint8_t btn = mouse_button_to_makcu(button);
    send_command("km.click(" + std::to_string(btn) + ")");
}

void Makcu::mouse_double_click(MouseButton button) {
    std::lock_guard<std::mutex> lock(mutex_);
    uint8_t btn = mouse_button_to_makcu(button);
    send_command("km.click(" + std::to_string(btn) + ",2)");
}

void Makcu::mouse_scroll(int32_t delta) {
    std::lock_guard<std::mutex> lock(mutex_);
    send_command("km.wheel(" + std::to_string(delta) + ")");
}

// ---------------------------------------------------------------------------
// keyboard
// ---------------------------------------------------------------------------

void Makcu::key_press(KeyCode key, KeyModifier modifiers) {
    std::lock_guard<std::mutex> lock(mutex_);
    // send modifier keys first if needed
    if (modifiers != KeyModifier::None) {
        if (static_cast<uint8_t>(modifiers & KeyModifier::LeftCtrl))
            send_command("km.down('lctrl')");
        if (static_cast<uint8_t>(modifiers & KeyModifier::LeftShift))
            send_command("km.down('lshift')");
        if (static_cast<uint8_t>(modifiers & KeyModifier::LeftAlt))
            send_command("km.down('lalt')");
        if (static_cast<uint8_t>(modifiers & KeyModifier::LeftGui))
            send_command("km.down('lgui')");
        if (static_cast<uint8_t>(modifiers & KeyModifier::RightCtrl))
            send_command("km.down('rctrl')");
        if (static_cast<uint8_t>(modifiers & KeyModifier::RightShift))
            send_command("km.down('rshift')");
        if (static_cast<uint8_t>(modifiers & KeyModifier::RightAlt))
            send_command("km.down('ralt')");
        if (static_cast<uint8_t>(modifiers & KeyModifier::RightGui))
            send_command("km.down('rgui')");
    }
    // send key as HID code
    send_command("km.down(" + std::to_string(static_cast<int>(key)) + ")");
}

void Makcu::key_release(KeyCode key) {
    std::lock_guard<std::mutex> lock(mutex_);
    send_command("km.up(" + std::to_string(static_cast<int>(key)) + ")");
}

void Makcu::key_tap(KeyCode key, KeyModifier modifiers) {
    key_press(key, modifiers);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    key_release(key);
    // release modifiers
    if (modifiers != KeyModifier::None) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (static_cast<uint8_t>(modifiers & KeyModifier::LeftCtrl))
            send_command("km.up('lctrl')");
        if (static_cast<uint8_t>(modifiers & KeyModifier::LeftShift))
            send_command("km.up('lshift')");
        if (static_cast<uint8_t>(modifiers & KeyModifier::LeftAlt))
            send_command("km.up('lalt')");
        if (static_cast<uint8_t>(modifiers & KeyModifier::LeftGui))
            send_command("km.up('lgui')");
        if (static_cast<uint8_t>(modifiers & KeyModifier::RightCtrl))
            send_command("km.up('rctrl')");
        if (static_cast<uint8_t>(modifiers & KeyModifier::RightShift))
            send_command("km.up('rshift')");
        if (static_cast<uint8_t>(modifiers & KeyModifier::RightAlt))
            send_command("km.up('ralt')");
        if (static_cast<uint8_t>(modifiers & KeyModifier::RightGui))
            send_command("km.up('rgui')");
    }
}

void Makcu::key_release_all() {
    // release all standard modifier keys
    std::lock_guard<std::mutex> lock(mutex_);
    send_command("km.up('lctrl')");
    send_command("km.up('lshift')");
    send_command("km.up('lalt')");
    send_command("km.up('lgui')");
    send_command("km.up('rctrl')");
    send_command("km.up('rshift')");
    send_command("km.up('ralt')");
    send_command("km.up('rgui')");
}

void Makcu::type_string(const std::string& text, uint32_t /* interval_ms */) {
    // km.type() handles timing and shift automatically
    std::lock_guard<std::mutex> lock(mutex_);
    // escape single quotes in the text
    std::string escaped;
    for (char c : text) {
        if (c == '\'') escaped += "\\'";
        else if (c == '\\') escaped += "\\\\";
        else escaped += c;
    }
    send_command("km.type('" + escaped + "')");
}

// ---------------------------------------------------------------------------
// device management
// ---------------------------------------------------------------------------

std::string Makcu::device_name() const { return "Makcu"; }

DeviceInfo Makcu::get_info() {
    auto full = device_info_full();
    DeviceInfo info;
    info.device_type = "Makcu";
    info.firmware_version = full.firmware;
    info.serial_number = full.serial;
    return info;
}

void Makcu::reboot() {
    std::lock_guard<std::mutex> lock(mutex_);
    send_command_no_response("km.reboot()");
    connected_ = false;
}

// ---------------------------------------------------------------------------
// Makcu-specific
// ---------------------------------------------------------------------------

void Makcu::silent_move(int32_t dx, int32_t dy) {
    std::lock_guard<std::mutex> lock(mutex_);
    send_command("km.silent_move(" + std::to_string(dx) + "," +
                 std::to_string(dy) + ")");
}

void Makcu::click(MakcuButton button, int count, int delay_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string cmd = "km.click(" + std::to_string(static_cast<int>(button));
    if (count > 1 || delay_ms >= 0) {
        cmd += "," + std::to_string(count);
        if (delay_ms >= 0)
            cmd += "," + std::to_string(delay_ms);
    }
    cmd += ")";
    send_command(cmd);
}

void Makcu::turbo(MakcuButton button, int delay_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    send_command("km.turbo(" + std::to_string(static_cast<int>(button)) + "," +
                 std::to_string(delay_ms) + ")");
}

void Makcu::turbo_disable_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    send_command("km.turbo(0)");
}

void Makcu::lock(MakcuLockTarget target) {
    std::lock_guard<std::mutex> lock(mutex_);
    send_command(std::string("km.lock(") + lock_target_str(target) + ",1)");
}

void Makcu::unlock(MakcuLockTarget target) {
    std::lock_guard<std::mutex> lock(mutex_);
    send_command(std::string("km.lock(") + lock_target_str(target) + ",0)");
}

int Makcu::lock_state(MakcuLockTarget target) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto resp = send_command(std::string("km.lock(") + lock_target_str(target) + ")");
    try { return std::stoi(resp); }
    catch (...) { return 0; }
}

std::unordered_map<std::string, int> Makcu::lock_states_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto resp = send_command("km.lock()");
    std::unordered_map<std::string, int> states;

    // parse key=value pairs
    std::istringstream stream(resp);
    std::string line;
    while (std::getline(stream, line)) {
        auto eq = line.find('=');
        if (eq != std::string::npos) {
            std::string key = trim(line.substr(0, eq));
            std::string val = trim(line.substr(eq + 1));
            try { states[key] = std::stoi(val); }
            catch (...) { states[key] = 0; }
        }
    }
    return states;
}

void Makcu::stream_set(MakcuStreamMode mode, int period_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string cmd = "km.stream(" + std::to_string(static_cast<int>(mode));
    if (period_ms > 0)
        cmd += "," + std::to_string(period_ms);
    cmd += ")";
    send_command(cmd);
}

MakcuStreamMode Makcu::stream_mode() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto resp = send_command("km.stream()");
    try {
        int val = std::stoi(resp);
        return static_cast<MakcuStreamMode>(val);
    } catch (...) {
        return MakcuStreamMode::Off;
    }
}

void Makcu::echo(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    send_command("km.echo(" + std::to_string(enabled ? 1 : 0) + ")");
}

std::string Makcu::serial_number() {
    std::lock_guard<std::mutex> lock(mutex_);
    return send_command("km.serial()");
}

void Makcu::set_serial(const std::string& serial) {
    std::lock_guard<std::mutex> lock(mutex_);
    send_command("km.set_serial('" + serial + "')");
}

MakcuDeviceInfo Makcu::device_info_full() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto resp = send_command("km.info()");

    MakcuDeviceInfo info;
    std::istringstream stream(resp);
    std::string line;
    while (std::getline(stream, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = trim(line.substr(0, eq));
        std::string val = trim(line.substr(eq + 1));

        if (key == "MAC")      info.mac = val;
        else if (key == "FW")  info.firmware = val;
        else if (key == "CPU") info.cpu = val;
        else if (key == "VENDOR") info.vendor = val;
        else if (key == "MODEL")  info.model = val;
        else if (key == "SERIAL") info.serial = val;
        else if (key == "VID")    info.vid = val;
        else if (key == "PID")    info.pid = val;
        else if (key == "TEMP") {
            try { info.temp_c = std::stoi(val); } catch (...) {}
        }
        else if (key == "RAM") {
            try { info.ram_free = std::stoi(val); } catch (...) {}
        }
        else if (key == "UPTIME") {
            try { info.uptime_s = std::stoi(val); } catch (...) {}
        }
    }
    return info;
}

std::string Makcu::firmware_version() {
    std::lock_guard<std::mutex> lock(mutex_);
    return send_command("km.version()");
}

void Makcu::key_down(const std::string& key_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    send_command("km.down('" + key_name + "')");
}

void Makcu::key_up(const std::string& key_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    send_command("km.up('" + key_name + "')");
}

void Makcu::key_press_name(const std::string& key_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    send_command("km.press('" + key_name + "')");
}

} // namespace im
