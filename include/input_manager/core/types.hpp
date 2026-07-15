#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace im {

// ---------------------------------------------------------------------------
// Mouse
// ---------------------------------------------------------------------------

enum class MouseButton : uint8_t {
    None   = 0x00,
    Left   = 0x01,
    Right  = 0x02,
    Middle = 0x04,
    Side1  = 0x08,
    Side2  = 0x10,
};

inline MouseButton operator|(MouseButton a, MouseButton b) {
    return static_cast<MouseButton>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
inline MouseButton operator&(MouseButton a, MouseButton b) {
    return static_cast<MouseButton>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}
inline bool has_flag(MouseButton value, MouseButton flag) {
    return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0;
}

// ---------------------------------------------------------------------------
// Keyboard modifiers
// ---------------------------------------------------------------------------

enum class KeyModifier : uint8_t {
    None       = 0x00,
    LeftCtrl   = 0x01,
    LeftShift  = 0x02,
    LeftAlt    = 0x04,
    LeftGui    = 0x08,
    RightCtrl  = 0x10,
    RightShift = 0x20,
    RightAlt   = 0x40,
    RightGui   = 0x80,
};

inline KeyModifier operator|(KeyModifier a, KeyModifier b) {
    return static_cast<KeyModifier>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
inline KeyModifier operator&(KeyModifier a, KeyModifier b) {
    return static_cast<KeyModifier>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

// ---------------------------------------------------------------------------
// USB HID key codes
// ---------------------------------------------------------------------------

enum class KeyCode : uint8_t {
    None          = 0x00,
    A = 0x04, B = 0x05, C = 0x06, D = 0x07, E = 0x08, F = 0x09,
    G = 0x0A, H = 0x0B, I = 0x0C, J = 0x0D, K = 0x0E, L = 0x0F,
    M = 0x10, N = 0x11, O = 0x12, P = 0x13, Q = 0x14, R = 0x15,
    S = 0x16, T = 0x17, U = 0x18, V = 0x19, W = 0x1A, X = 0x1B,
    Y = 0x1C, Z = 0x1D,
    Num1 = 0x1E, Num2 = 0x1F, Num3 = 0x20, Num4 = 0x21, Num5 = 0x22,
    Num6 = 0x23, Num7 = 0x24, Num8 = 0x25, Num9 = 0x26, Num0 = 0x27,
    Enter      = 0x28,
    Escape     = 0x29,
    Backspace  = 0x2A,
    Tab        = 0x2B,
    Space      = 0x2C,
    Minus      = 0x2D,
    Equal      = 0x2E,
    LBracket   = 0x2F,
    RBracket   = 0x30,
    Backslash  = 0x31,
    Semicolon  = 0x33,
    Quote      = 0x34,
    Grave      = 0x35,
    Comma      = 0x36,
    Period     = 0x37,
    Slash      = 0x38,
    CapsLock   = 0x39,
    F1  = 0x3A, F2  = 0x3B, F3  = 0x3C, F4  = 0x3D,
    F5  = 0x3E, F6  = 0x3F, F7  = 0x40, F8  = 0x41,
    F9  = 0x42, F10 = 0x43, F11 = 0x44, F12 = 0x45,
    PrintScreen = 0x46,
    ScrollLock  = 0x47,
    Pause       = 0x48,
    Insert      = 0x49,
    Home        = 0x4A,
    PageUp      = 0x4B,
    Delete      = 0x4C,
    End         = 0x4D,
    PageDown    = 0x4E,
    RightArrow  = 0x4F,
    LeftArrow   = 0x50,
    DownArrow   = 0x51,
    UpArrow     = 0x52,
    NumLock     = 0x53,
    NumDivide   = 0x54,
    NumMultiply = 0x55,
    NumSubtract = 0x56,
    NumAdd      = 0x57,
    NumEnter    = 0x58,
    Numpad1 = 0x59, Numpad2 = 0x5A, Numpad3 = 0x5B,
    Numpad4 = 0x5C, Numpad5 = 0x5D, Numpad6 = 0x5E,
    Numpad7 = 0x5F, Numpad8 = 0x60, Numpad9 = 0x61,
    Numpad0 = 0x62, NumDecimal = 0x63,
    App     = 0x65,
};

// ---------------------------------------------------------------------------
// Character-to-key mapping (US QWERTY layout)
// ---------------------------------------------------------------------------

struct CharMapping {
    KeyCode    key;
    KeyModifier modifier;
};

inline const std::unordered_map<char, CharMapping>& char_map() {
    static const std::unordered_map<char, CharMapping> map = {
        {'a', {KeyCode::A, KeyModifier::None}}, {'b', {KeyCode::B, KeyModifier::None}},
        {'c', {KeyCode::C, KeyModifier::None}}, {'d', {KeyCode::D, KeyModifier::None}},
        {'e', {KeyCode::E, KeyModifier::None}}, {'f', {KeyCode::F, KeyModifier::None}},
        {'g', {KeyCode::G, KeyModifier::None}}, {'h', {KeyCode::H, KeyModifier::None}},
        {'i', {KeyCode::I, KeyModifier::None}}, {'j', {KeyCode::J, KeyModifier::None}},
        {'k', {KeyCode::K, KeyModifier::None}}, {'l', {KeyCode::L, KeyModifier::None}},
        {'m', {KeyCode::M, KeyModifier::None}}, {'n', {KeyCode::N, KeyModifier::None}},
        {'o', {KeyCode::O, KeyModifier::None}}, {'p', {KeyCode::P, KeyModifier::None}},
        {'q', {KeyCode::Q, KeyModifier::None}}, {'r', {KeyCode::R, KeyModifier::None}},
        {'s', {KeyCode::S, KeyModifier::None}}, {'t', {KeyCode::T, KeyModifier::None}},
        {'u', {KeyCode::U, KeyModifier::None}}, {'v', {KeyCode::V, KeyModifier::None}},
        {'w', {KeyCode::W, KeyModifier::None}}, {'x', {KeyCode::X, KeyModifier::None}},
        {'y', {KeyCode::Y, KeyModifier::None}}, {'z', {KeyCode::Z, KeyModifier::None}},
        {'A', {KeyCode::A, KeyModifier::LeftShift}}, {'B', {KeyCode::B, KeyModifier::LeftShift}},
        {'C', {KeyCode::C, KeyModifier::LeftShift}}, {'D', {KeyCode::D, KeyModifier::LeftShift}},
        {'E', {KeyCode::E, KeyModifier::LeftShift}}, {'F', {KeyCode::F, KeyModifier::LeftShift}},
        {'G', {KeyCode::G, KeyModifier::LeftShift}}, {'H', {KeyCode::H, KeyModifier::LeftShift}},
        {'I', {KeyCode::I, KeyModifier::LeftShift}}, {'J', {KeyCode::J, KeyModifier::LeftShift}},
        {'K', {KeyCode::K, KeyModifier::LeftShift}}, {'L', {KeyCode::L, KeyModifier::LeftShift}},
        {'M', {KeyCode::M, KeyModifier::LeftShift}}, {'N', {KeyCode::N, KeyModifier::LeftShift}},
        {'O', {KeyCode::O, KeyModifier::LeftShift}}, {'P', {KeyCode::P, KeyModifier::LeftShift}},
        {'Q', {KeyCode::Q, KeyModifier::LeftShift}}, {'R', {KeyCode::R, KeyModifier::LeftShift}},
        {'S', {KeyCode::S, KeyModifier::LeftShift}}, {'T', {KeyCode::T, KeyModifier::LeftShift}},
        {'U', {KeyCode::U, KeyModifier::LeftShift}}, {'V', {KeyCode::V, KeyModifier::LeftShift}},
        {'W', {KeyCode::W, KeyModifier::LeftShift}}, {'X', {KeyCode::X, KeyModifier::LeftShift}},
        {'Y', {KeyCode::Y, KeyModifier::LeftShift}}, {'Z', {KeyCode::Z, KeyModifier::LeftShift}},
        {'1', {KeyCode::Num1, KeyModifier::None}}, {'2', {KeyCode::Num2, KeyModifier::None}},
        {'3', {KeyCode::Num3, KeyModifier::None}}, {'4', {KeyCode::Num4, KeyModifier::None}},
        {'5', {KeyCode::Num5, KeyModifier::None}}, {'6', {KeyCode::Num6, KeyModifier::None}},
        {'7', {KeyCode::Num7, KeyModifier::None}}, {'8', {KeyCode::Num8, KeyModifier::None}},
        {'9', {KeyCode::Num9, KeyModifier::None}}, {'0', {KeyCode::Num0, KeyModifier::None}},
        {' ', {KeyCode::Space, KeyModifier::None}},
        {'\n', {KeyCode::Enter, KeyModifier::None}},
        {'\t', {KeyCode::Tab, KeyModifier::None}},
        {'-', {KeyCode::Minus, KeyModifier::None}},
        {'=', {KeyCode::Equal, KeyModifier::None}},
        {'[', {KeyCode::LBracket, KeyModifier::None}},
        {']', {KeyCode::RBracket, KeyModifier::None}},
        {'\\', {KeyCode::Backslash, KeyModifier::None}},
        {';', {KeyCode::Semicolon, KeyModifier::None}},
        {'\'', {KeyCode::Quote, KeyModifier::None}},
        {'`', {KeyCode::Grave, KeyModifier::None}},
        {',', {KeyCode::Comma, KeyModifier::None}},
        {'.', {KeyCode::Period, KeyModifier::None}},
        {'/', {KeyCode::Slash, KeyModifier::None}},
        {'!', {KeyCode::Num1, KeyModifier::LeftShift}},
        {'@', {KeyCode::Num2, KeyModifier::LeftShift}},
        {'#', {KeyCode::Num3, KeyModifier::LeftShift}},
        {'$', {KeyCode::Num4, KeyModifier::LeftShift}},
        {'%', {KeyCode::Num5, KeyModifier::LeftShift}},
        {'^', {KeyCode::Num6, KeyModifier::LeftShift}},
        {'&', {KeyCode::Num7, KeyModifier::LeftShift}},
        {'*', {KeyCode::Num8, KeyModifier::LeftShift}},
        {'(', {KeyCode::Num9, KeyModifier::LeftShift}},
        {')', {KeyCode::Num0, KeyModifier::LeftShift}},
        {'_', {KeyCode::Minus, KeyModifier::LeftShift}},
        {'+', {KeyCode::Equal, KeyModifier::LeftShift}},
        {'{', {KeyCode::LBracket, KeyModifier::LeftShift}},
        {'}', {KeyCode::RBracket, KeyModifier::LeftShift}},
        {'|', {KeyCode::Backslash, KeyModifier::LeftShift}},
        {':', {KeyCode::Semicolon, KeyModifier::LeftShift}},
        {'"', {KeyCode::Quote, KeyModifier::LeftShift}},
        {'~', {KeyCode::Grave, KeyModifier::LeftShift}},
        {'<', {KeyCode::Comma, KeyModifier::LeftShift}},
        {'>', {KeyCode::Period, KeyModifier::LeftShift}},
        {'?', {KeyCode::Slash, KeyModifier::LeftShift}},
    };
    return map;
}

// ---------------------------------------------------------------------------
// Device info
// ---------------------------------------------------------------------------

struct DeviceInfo {
    std::string device_type;
    std::string firmware_version;
    std::string serial_number;
};

} // namespace im
