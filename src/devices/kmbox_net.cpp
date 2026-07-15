#include "input_manager/devices/kmbox_net.hpp"
#include "input_manager/core/errors.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <random>
#include <thread>

namespace im {

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

static void push_u32_le(std::vector<uint8_t>& v, uint32_t val) {
    v.push_back(static_cast<uint8_t>( val        & 0xFF));
    v.push_back(static_cast<uint8_t>((val >>  8) & 0xFF));
    v.push_back(static_cast<uint8_t>((val >> 16) & 0xFF));
    v.push_back(static_cast<uint8_t>((val >> 24) & 0xFF));
}

static void push_i32_le(std::vector<uint8_t>& v, int32_t val) {
    push_u32_le(v, static_cast<uint32_t>(val));
}

static uint32_t read_u32_le(const uint8_t* p) {
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) |
           (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}

static uint32_t random_u32() {
    static thread_local std::mt19937 gen(
        static_cast<unsigned>(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    return gen();
}

// ---------------------------------------------------------------------------
// AES-128-ECB — software implementation (no external dependencies)
// ---------------------------------------------------------------------------

static void aes_expand_key(const uint8_t key[16], uint32_t rk[44]);
static void aes_encrypt_block(const uint32_t rk[44], const uint8_t in[16],
                               uint8_t out[16]);
static void aes_decrypt_block(const uint32_t rk[44], const uint8_t in[16],
                               uint8_t out[16]);

static const uint8_t SBOX[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16,
};

static const uint8_t INV_SBOX[256] = {
    0x52,0x09,0x6a,0xd5,0x30,0x36,0xa5,0x38,0xbf,0x40,0xa3,0x9e,0x81,0xf3,0xd7,0xfb,
    0x7c,0xe3,0x39,0x82,0x9b,0x2f,0xff,0x87,0x34,0x8e,0x43,0x44,0xc4,0xde,0xe9,0xcb,
    0x54,0x7b,0x94,0x32,0xa6,0xc2,0x23,0x3d,0xee,0x4c,0x95,0x0b,0x42,0xfa,0xc3,0x4e,
    0x08,0x2e,0xa1,0x66,0x28,0xd9,0x24,0xb2,0x76,0x5b,0xa2,0x49,0x6d,0x8b,0xd1,0x25,
    0x72,0xf8,0xf6,0x64,0x86,0x68,0x98,0x16,0xd4,0xa4,0x5c,0xcc,0x5d,0x65,0xb6,0x92,
    0x6c,0x70,0x48,0x50,0xfd,0xed,0xb9,0xda,0x5e,0x15,0x46,0x57,0xa7,0x8d,0x9d,0x84,
    0x90,0xd8,0xab,0x00,0x8c,0xbc,0xd3,0x0a,0xf7,0xe4,0x58,0x05,0xb8,0xb3,0x45,0x06,
    0xd0,0x2c,0x1e,0x8f,0xca,0x3f,0x0f,0x02,0xc1,0xaf,0xbd,0x03,0x01,0x13,0x8a,0x6b,
    0x3a,0x91,0x11,0x41,0x4f,0x67,0xdc,0xea,0x97,0xf2,0xcf,0xce,0xf0,0xb4,0xe6,0x73,
    0x96,0xac,0x74,0x22,0xe7,0xad,0x35,0x85,0xe2,0xf9,0x37,0xe8,0x1c,0x75,0xdf,0x6e,
    0x47,0xf1,0x1a,0x71,0x1d,0x29,0xc5,0x89,0x6f,0xb7,0x62,0x0e,0xaa,0x18,0xbe,0x1b,
    0xfc,0x56,0x3e,0x4b,0xc6,0xd2,0x79,0x20,0x9a,0xdb,0xc0,0xfe,0x78,0xcd,0x5a,0xf4,
    0x1f,0xdd,0xa8,0x33,0x88,0x07,0xc7,0x31,0xb1,0x12,0x10,0x59,0x27,0x80,0xec,0x5f,
    0x60,0x51,0x7f,0xa9,0x19,0xb5,0x4a,0x0d,0x2d,0xe5,0x7a,0x9f,0x93,0xc9,0x9c,0xef,
    0xa0,0xe0,0x3b,0x4d,0xae,0x2a,0xf5,0xb0,0xc8,0xeb,0xbb,0x3c,0x83,0x53,0x99,0x61,
    0x17,0x2b,0x04,0x7e,0xba,0x77,0xd6,0x26,0xe1,0x69,0x14,0x63,0x55,0x21,0x0c,0x7d,
};

static const uint8_t RCON[11] = {
    0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
};

static uint32_t sub_word(uint32_t w) {
    return (uint32_t(SBOX[(w >> 24) & 0xFF]) << 24) |
           (uint32_t(SBOX[(w >> 16) & 0xFF]) << 16) |
           (uint32_t(SBOX[(w >>  8) & 0xFF]) <<  8) |
           (uint32_t(SBOX[ w        & 0xFF]));
}

static uint32_t rot_word(uint32_t w) {
    return (w << 8) | (w >> 24);
}

static uint32_t load_be32(const uint8_t* p) {
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) |
           (uint32_t(p[2]) <<  8) |  uint32_t(p[3]);
}

static void store_be32(uint8_t* p, uint32_t v) {
    p[0] = uint8_t(v >> 24); p[1] = uint8_t(v >> 16);
    p[2] = uint8_t(v >>  8); p[3] = uint8_t(v);
}

static void aes_expand_key(const uint8_t key[16], uint32_t rk[44]) {
    for (int i = 0; i < 4; ++i)
        rk[i] = load_be32(key + 4 * i);
    for (int i = 4; i < 44; ++i) {
        uint32_t t = rk[i - 1];
        if (i % 4 == 0)
            t = sub_word(rot_word(t)) ^ (uint32_t(RCON[i / 4]) << 24);
        rk[i] = rk[i - 4] ^ t;
    }
}

static uint8_t gmul(uint8_t a, uint8_t b) {
    uint8_t p = 0;
    for (int i = 0; i < 8; ++i) {
        if (b & 1) p ^= a;
        bool hi = a & 0x80;
        a <<= 1;
        if (hi) a ^= 0x1b;
        b >>= 1;
    }
    return p;
}

static void aes_encrypt_block(const uint32_t rk[44], const uint8_t in[16],
                               uint8_t out[16]) {
    uint8_t state[4][4];
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            state[r][c] = in[c * 4 + r];

    for (int c = 0; c < 4; ++c) {
        uint32_t k = rk[c];
        state[0][c] ^= uint8_t(k >> 24); state[1][c] ^= uint8_t(k >> 16);
        state[2][c] ^= uint8_t(k >> 8);  state[3][c] ^= uint8_t(k);
    }

    for (int round = 1; round <= 10; ++round) {
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                state[r][c] = SBOX[state[r][c]];

        uint8_t tmp;
        tmp = state[1][0]; state[1][0]=state[1][1]; state[1][1]=state[1][2];
        state[1][2]=state[1][3]; state[1][3]=tmp;
        tmp = state[2][0]; state[2][0]=state[2][2]; state[2][2]=tmp;
        tmp = state[2][1]; state[2][1]=state[2][3]; state[2][3]=tmp;
        tmp = state[3][3]; state[3][3]=state[3][2]; state[3][2]=state[3][1];
        state[3][1]=state[3][0]; state[3][0]=tmp;

        if (round < 10) {
            for (int c = 0; c < 4; ++c) {
                uint8_t a0=state[0][c], a1=state[1][c], a2=state[2][c], a3=state[3][c];
                state[0][c] = gmul(a0,2)^gmul(a1,3)^a2^a3;
                state[1][c] = a0^gmul(a1,2)^gmul(a2,3)^a3;
                state[2][c] = a0^a1^gmul(a2,2)^gmul(a3,3);
                state[3][c] = gmul(a0,3)^a1^a2^gmul(a3,2);
            }
        }

        for (int c = 0; c < 4; ++c) {
            uint32_t k = rk[round * 4 + c];
            state[0][c] ^= uint8_t(k >> 24); state[1][c] ^= uint8_t(k >> 16);
            state[2][c] ^= uint8_t(k >> 8);  state[3][c] ^= uint8_t(k);
        }
    }

    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            out[c * 4 + r] = state[r][c];
}

static void aes_decrypt_block(const uint32_t rk[44], const uint8_t in[16],
                               uint8_t out[16]) {
    uint8_t state[4][4];
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            state[r][c] = in[c * 4 + r];

    for (int c = 0; c < 4; ++c) {
        uint32_t k = rk[40 + c];
        state[0][c] ^= uint8_t(k >> 24); state[1][c] ^= uint8_t(k >> 16);
        state[2][c] ^= uint8_t(k >> 8);  state[3][c] ^= uint8_t(k);
    }

    for (int round = 9; round >= 0; --round) {
        uint8_t tmp;
        tmp = state[1][3]; state[1][3]=state[1][2]; state[1][2]=state[1][1];
        state[1][1]=state[1][0]; state[1][0]=tmp;
        tmp = state[2][0]; state[2][0]=state[2][2]; state[2][2]=tmp;
        tmp = state[2][1]; state[2][1]=state[2][3]; state[2][3]=tmp;
        tmp = state[3][0]; state[3][0]=state[3][1]; state[3][1]=state[3][2];
        state[3][2]=state[3][3]; state[3][3]=tmp;

        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                state[r][c] = INV_SBOX[state[r][c]];

        for (int c = 0; c < 4; ++c) {
            uint32_t k = rk[round * 4 + c];
            state[0][c] ^= uint8_t(k >> 24); state[1][c] ^= uint8_t(k >> 16);
            state[2][c] ^= uint8_t(k >> 8);  state[3][c] ^= uint8_t(k);
        }

        if (round > 0) {
            for (int c = 0; c < 4; ++c) {
                uint8_t a0=state[0][c], a1=state[1][c], a2=state[2][c], a3=state[3][c];
                state[0][c] = gmul(a0,14)^gmul(a1,11)^gmul(a2,13)^gmul(a3,9);
                state[1][c] = gmul(a0,9)^gmul(a1,14)^gmul(a2,11)^gmul(a3,13);
                state[2][c] = gmul(a0,13)^gmul(a1,9)^gmul(a2,14)^gmul(a3,11);
                state[3][c] = gmul(a0,11)^gmul(a1,13)^gmul(a2,9)^gmul(a3,14);
            }
        }
    }

    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            out[c * 4 + r] = state[r][c];
}

// ---------------------------------------------------------------------------
// KMBoxNet — construction
// ---------------------------------------------------------------------------

KMBoxNet::KMBoxNet(const std::string& ip, uint16_t port, const std::string& uuid)
    : ip_(ip), port_(port), uuid_(uuid) {}

KMBoxNet::~KMBoxNet() { disconnect(); }

// ---------------------------------------------------------------------------
// connection — init(ip, port, uuid)
// ---------------------------------------------------------------------------

void KMBoxNet::connect() {
    std::lock_guard<std::mutex> lock(mutex_);
    udp_.open(ip_, port_);

    std::memset(aes_key_.data(), 0, 16);
    for (size_t i = 0; i < uuid_.size(); ++i)
        aes_key_[i % 16] ^= static_cast<uint8_t>(uuid_[i]);

    std::vector<uint8_t> payload(uuid_.begin(), uuid_.end());
    auto hdr = build_header(NET_CMD_CONNECT, static_cast<uint32_t>(payload.size()));
    hdr.insert(hdr.end(), payload.begin(), payload.end());
    udp_.send(hdr);

    auto resp = udp_.receive_vec(256, 2000);
    if (resp.size() < HEADER_SIZE)
        throw ConnectionError("KMBox Net: no response to connect");

    connected_ = true;
    std::memset(current_keys_, 0, sizeof(current_keys_));
    current_buttons_ = 0;
    current_modifier_ = 0;
}

void KMBoxNet::disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (connected_) {
        udp_.close();
        connected_ = false;
    }
}

bool KMBoxNet::is_connected() const { return connected_; }

// ---------------------------------------------------------------------------
// protocol — binary UDP packets with optional AES-128-ECB
// ---------------------------------------------------------------------------

std::vector<uint8_t> KMBoxNet::build_header(NetCmd cmd, uint32_t data_len) const {
    std::vector<uint8_t> hdr;
    hdr.reserve(HEADER_SIZE);
    push_u32_le(hdr, MAGIC);
    push_u32_le(hdr, static_cast<uint32_t>(cmd));
    push_u32_le(hdr, random_u32());
    push_u32_le(hdr, data_len);
    return hdr;
}

std::vector<uint8_t> KMBoxNet::encrypt_data(const std::vector<uint8_t>& data) const {
    if (data.empty()) return data;

    size_t padded_len = ((data.size() + 15) / 16) * 16;
    std::vector<uint8_t> padded(padded_len, 0);
    std::memcpy(padded.data(), data.data(), data.size());

    uint32_t rk[44];
    aes_expand_key(aes_key_.data(), rk);

    std::vector<uint8_t> out(padded_len);
    for (size_t i = 0; i < padded_len; i += 16)
        aes_encrypt_block(rk, padded.data() + i, out.data() + i);

    return out;
}

std::vector<uint8_t> KMBoxNet::decrypt_data(const std::vector<uint8_t>& data) const {
    if (data.empty() || data.size() % 16 != 0) return data;

    uint32_t rk[44];
    aes_expand_key(aes_key_.data(), rk);

    std::vector<uint8_t> out(data.size());
    for (size_t i = 0; i < data.size(); i += 16)
        aes_decrypt_block(rk, data.data() + i, out.data() + i);

    return out;
}

void KMBoxNet::send_packet(NetCmd cmd, const std::vector<uint8_t>& payload,
                           bool force_encrypt) {
    require_connected();
    bool do_encrypt = force_encrypt || encryption_enabled_;
    auto enc_payload = do_encrypt ? encrypt_data(payload) : payload;
    auto hdr = build_header(cmd, static_cast<uint32_t>(enc_payload.size()));
    hdr.insert(hdr.end(), enc_payload.begin(), enc_payload.end());
    udp_.send(hdr);
}

std::vector<uint8_t> KMBoxNet::send_and_receive(NetCmd cmd,
                                                 const std::vector<uint8_t>& payload,
                                                 bool force_encrypt) {
    send_packet(cmd, payload, force_encrypt);
    auto resp = udp_.receive_vec(4096, 1000);
    if (resp.size() <= HEADER_SIZE) return {};
    std::vector<uint8_t> body(resp.begin() + HEADER_SIZE, resp.end());
    bool do_encrypt = force_encrypt || encryption_enabled_;
    return do_encrypt ? decrypt_data(body) : body;
}

// ---------------------------------------------------------------------------
// mouse — move(x,y), move_auto(x,y,ms), move_beizer(x,y,ms,cx,cy,cx,cy)
// ---------------------------------------------------------------------------

void KMBoxNet::mouse_move(int32_t dx, int32_t dy) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> data;
    push_i32_le(data, dx);
    push_i32_le(data, dy);
    send_packet(NET_CMD_MOUSE_MOVE, data);
}

void KMBoxNet::mouse_move_absolute(int32_t x, int32_t y) {
    mouse_move(x, y);
}

void KMBoxNet::mouse_move_smooth(int32_t dx, int32_t dy, uint32_t duration_ms) {
    move_auto(dx, dy, duration_ms);
}

void KMBoxNet::move_auto(int32_t x, int32_t y, uint32_t duration_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> data;
    push_i32_le(data, x);
    push_i32_le(data, y);
    push_u32_le(data, duration_ms);
    send_packet(NET_CMD_MOUSE_AUTO, data);
}

void KMBoxNet::move_beizer(int32_t x, int32_t y, uint32_t duration_ms,
                           int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> data;
    push_i32_le(data, x);
    push_i32_le(data, y);
    push_u32_le(data, duration_ms);
    push_i32_le(data, cx1);
    push_i32_le(data, cy1);
    push_i32_le(data, cx2);
    push_i32_le(data, cy2);
    send_packet(NET_CMD_MOUSE_BEIZER, data);
}

void KMBoxNet::mouse_combined(uint8_t buttons, int32_t x, int32_t y, int32_t wheel) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> data;
    data.push_back(buttons);
    data.push_back(0); data.push_back(0); data.push_back(0);
    push_i32_le(data, x);
    push_i32_le(data, y);
    push_i32_le(data, wheel);
    send_packet(NET_CMD_MOUSE_ALL, data);
}

// ---------------------------------------------------------------------------
// button control — left/right/middle/side1/side2(state)
// ---------------------------------------------------------------------------

void KMBoxNet::left(int state) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> data;
    push_i32_le(data, state);
    send_packet(NET_CMD_MOUSE_LEFT, data);
    if (state) current_buttons_ |= 0x01;
    else       current_buttons_ &= ~0x01;
}

void KMBoxNet::right(int state) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> data;
    push_i32_le(data, state);
    send_packet(NET_CMD_MOUSE_RIGHT, data);
    if (state) current_buttons_ |= 0x02;
    else       current_buttons_ &= ~0x02;
}

void KMBoxNet::middle(int state) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> data;
    push_i32_le(data, state);
    send_packet(NET_CMD_MOUSE_MIDDLE, data);
    if (state) current_buttons_ |= 0x04;
    else       current_buttons_ &= ~0x04;
}

void KMBoxNet::side1(int state) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> data;
    push_i32_le(data, state);
    send_packet(NET_CMD_MOUSE_SIDE1, data);
    if (state) current_buttons_ |= 0x08;
    else       current_buttons_ &= ~0x08;
}

void KMBoxNet::side2(int state) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> data;
    push_i32_le(data, state);
    send_packet(NET_CMD_MOUSE_SIDE2, data);
    if (state) current_buttons_ |= 0x10;
    else       current_buttons_ &= ~0x10;
}

void KMBoxNet::wheel(int direction) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> data;
    push_i32_le(data, direction);
    send_packet(NET_CMD_MOUSE_WHEEL, data);
}

void KMBoxNet::mouse_press(MouseButton button) {
    if (has_flag(button, MouseButton::Left))   left(1);
    else if (has_flag(button, MouseButton::Right))  right(1);
    else if (has_flag(button, MouseButton::Middle)) middle(1);
    else if (has_flag(button, MouseButton::Side1))  side1(1);
    else if (has_flag(button, MouseButton::Side2))  side2(1);
}

void KMBoxNet::mouse_release(MouseButton button) {
    if (has_flag(button, MouseButton::Left))   left(0);
    else if (has_flag(button, MouseButton::Right))  right(0);
    else if (has_flag(button, MouseButton::Middle)) middle(0);
    else if (has_flag(button, MouseButton::Side1))  side1(0);
    else if (has_flag(button, MouseButton::Side2))  side2(0);
}

void KMBoxNet::mouse_click(MouseButton button) {
    mouse_press(button);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    mouse_release(button);
}

void KMBoxNet::mouse_double_click(MouseButton button) {
    mouse_click(button);
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    mouse_click(button);
}

void KMBoxNet::mouse_scroll(int32_t delta) {
    wheel(delta);
}

// ---------------------------------------------------------------------------
// keyboard — keydown(hid_code), keyup(hid_code)
// ---------------------------------------------------------------------------

void KMBoxNet::keydown(uint8_t hid_code) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> data;
    push_u32_le(data, hid_code);
    send_packet(NET_CMD_KEYBOARD_DOWN, data);
}

void KMBoxNet::keyup(uint8_t hid_code) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> data;
    push_u32_le(data, hid_code);
    send_packet(NET_CMD_KEYBOARD_UP, data);
}

void KMBoxNet::key_press(KeyCode key, KeyModifier modifiers) {
    if (modifiers != KeyModifier::None) {
        if (static_cast<uint8_t>(modifiers & KeyModifier::LeftCtrl))   keydown(0xE0);
        if (static_cast<uint8_t>(modifiers & KeyModifier::LeftShift))  keydown(0xE1);
        if (static_cast<uint8_t>(modifiers & KeyModifier::LeftAlt))    keydown(0xE2);
        if (static_cast<uint8_t>(modifiers & KeyModifier::LeftGui))    keydown(0xE3);
        if (static_cast<uint8_t>(modifiers & KeyModifier::RightCtrl))  keydown(0xE4);
        if (static_cast<uint8_t>(modifiers & KeyModifier::RightShift)) keydown(0xE5);
        if (static_cast<uint8_t>(modifiers & KeyModifier::RightAlt))   keydown(0xE6);
        if (static_cast<uint8_t>(modifiers & KeyModifier::RightGui))   keydown(0xE7);
    }
    keydown(static_cast<uint8_t>(key));
}

void KMBoxNet::key_release(KeyCode key) {
    keyup(static_cast<uint8_t>(key));
}

void KMBoxNet::key_tap(KeyCode key, KeyModifier modifiers) {
    key_press(key, modifiers);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    key_release(key);
    if (modifiers != KeyModifier::None) {
        if (static_cast<uint8_t>(modifiers & KeyModifier::LeftCtrl))   keyup(0xE0);
        if (static_cast<uint8_t>(modifiers & KeyModifier::LeftShift))  keyup(0xE1);
        if (static_cast<uint8_t>(modifiers & KeyModifier::LeftAlt))    keyup(0xE2);
        if (static_cast<uint8_t>(modifiers & KeyModifier::LeftGui))    keyup(0xE3);
        if (static_cast<uint8_t>(modifiers & KeyModifier::RightCtrl))  keyup(0xE4);
        if (static_cast<uint8_t>(modifiers & KeyModifier::RightShift)) keyup(0xE5);
        if (static_cast<uint8_t>(modifiers & KeyModifier::RightAlt))   keyup(0xE6);
        if (static_cast<uint8_t>(modifiers & KeyModifier::RightGui))   keyup(0xE7);
    }
}

void KMBoxNet::key_release_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    send_packet(NET_CMD_RELEASE_ALL);
    current_buttons_ = 0;
    current_modifier_ = 0;
    std::memset(current_keys_, 0, sizeof(current_keys_));
}

void KMBoxNet::type_string(const std::string& text, uint32_t interval_ms) {
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

std::string KMBoxNet::device_name() const { return "KMBox Net"; }

DeviceInfo KMBoxNet::get_info() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto resp = send_and_receive(NET_CMD_INFO);
    DeviceInfo info;
    info.device_type = "KMBox Net";
    if (resp.size() >= 4) {
        info.firmware_version = std::to_string(resp[0]) + "." +
                                std::to_string(resp[1]) + "." +
                                std::to_string(resp[2]);
    } else {
        info.firmware_version = "unknown";
    }
    return info;
}

void KMBoxNet::reboot() {
    std::lock_guard<std::mutex> lock(mutex_);
    send_packet(NET_CMD_REBOOT);
    connected_ = false;
}

// ---------------------------------------------------------------------------
// KMBox Net specific — encryption
// ---------------------------------------------------------------------------

void KMBoxNet::set_encryption(bool enabled) {
    encryption_enabled_ = enabled;
}

bool KMBoxNet::encryption_enabled() const {
    return encryption_enabled_;
}

// ---------------------------------------------------------------------------
// monitor — physical input monitoring
// ---------------------------------------------------------------------------

void KMBoxNet::monitor(int port) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> data;
    push_i32_le(data, port);
    send_packet(NET_CMD_MONITOR, data);
}

// ---------------------------------------------------------------------------
// isdown — query physical button state
// ---------------------------------------------------------------------------

bool KMBoxNet::isdown_left() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> data;
    push_u32_le(data, 0);
    auto resp = send_and_receive(NET_CMD_ISDOWN, data);
    return resp.size() >= 4 && read_u32_le(resp.data()) != 0;
}

bool KMBoxNet::isdown_right() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> data;
    push_u32_le(data, 1);
    auto resp = send_and_receive(NET_CMD_ISDOWN, data);
    return resp.size() >= 4 && read_u32_le(resp.data()) != 0;
}

bool KMBoxNet::isdown_middle() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> data;
    push_u32_le(data, 2);
    auto resp = send_and_receive(NET_CMD_ISDOWN, data);
    return resp.size() >= 4 && read_u32_le(resp.data()) != 0;
}

bool KMBoxNet::isdown_side1() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> data;
    push_u32_le(data, 3);
    auto resp = send_and_receive(NET_CMD_ISDOWN, data);
    return resp.size() >= 4 && read_u32_le(resp.data()) != 0;
}

bool KMBoxNet::isdown_side2() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> data;
    push_u32_le(data, 4);
    auto resp = send_and_receive(NET_CMD_ISDOWN, data);
    return resp.size() >= 4 && read_u32_le(resp.data()) != 0;
}

bool KMBoxNet::isdown_key(uint8_t hid_code) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> data;
    push_u32_le(data, 0x100 | hid_code);
    auto resp = send_and_receive(NET_CMD_ISDOWN, data);
    return resp.size() >= 4 && read_u32_le(resp.data()) != 0;
}

bool KMBoxNet::isdown_key(KeyCode key) {
    return isdown_key(static_cast<uint8_t>(key));
}

// ---------------------------------------------------------------------------
// encrypted variants — force AES regardless of global toggle
// ---------------------------------------------------------------------------

void KMBoxNet::enc_move(int32_t x, int32_t y) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> data;
    push_i32_le(data, x);
    push_i32_le(data, y);
    send_packet(NET_CMD_MOUSE_MOVE, data, true);
}

void KMBoxNet::enc_left(int state) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> data;
    push_i32_le(data, state);
    send_packet(NET_CMD_MOUSE_LEFT, data, true);
}

void KMBoxNet::enc_right(int state) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> data;
    push_i32_le(data, state);
    send_packet(NET_CMD_MOUSE_RIGHT, data, true);
}

void KMBoxNet::enc_middle(int state) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> data;
    push_i32_le(data, state);
    send_packet(NET_CMD_MOUSE_MIDDLE, data, true);
}

void KMBoxNet::enc_side1(int state) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> data;
    push_i32_le(data, state);
    send_packet(NET_CMD_MOUSE_SIDE1, data, true);
}

void KMBoxNet::enc_side2(int state) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> data;
    push_i32_le(data, state);
    send_packet(NET_CMD_MOUSE_SIDE2, data, true);
}

void KMBoxNet::enc_wheel(int direction) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> data;
    push_i32_le(data, direction);
    send_packet(NET_CMD_MOUSE_WHEEL, data, true);
}

// ---------------------------------------------------------------------------
// mask — mouse and keyboard masking
// ---------------------------------------------------------------------------

void KMBoxNet::mask_mouse(int32_t x, int32_t y) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> data;
    push_i32_le(data, x);
    push_i32_le(data, y);
    send_packet(NET_CMD_MASK_MOUSE, data);
}

void KMBoxNet::unmask_mouse() {
    std::lock_guard<std::mutex> lock(mutex_);
    send_packet(NET_CMD_UNMASK_MOUSE);
}

void KMBoxNet::mask_keyboard(uint8_t hid_code) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> data;
    push_u32_le(data, hid_code);
    send_packet(NET_CMD_MASK_KB, data);
}

void KMBoxNet::unmask_keyboard() {
    std::lock_guard<std::mutex> lock(mutex_);
    send_packet(NET_CMD_UNMASK_KB);
}

} // namespace im
