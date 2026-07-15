#include "input_manager/transport/udp_client.hpp"
#include "input_manager/core/errors.hpp"

#include <cstring>

#ifdef _WIN32
// ============================================================================
//  Windows implementation
// ============================================================================

#pragma comment(lib, "ws2_32.lib")

namespace im {

bool UdpClient::wsa_initialized_ = false;

void UdpClient::init_wsa() {
    if (!wsa_initialized_) {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
            throw ConnectionError("WSAStartup failed");
        wsa_initialized_ = true;
    }
}

void UdpClient::open(const std::string& ip, uint16_t port) {
    if (is_open()) close();
    init_wsa();

    sock_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_ == INVALID_SOCKET)
        throw ConnectionError("Failed to create UDP socket");

    std::memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr) <= 0) {
        closesocket(sock_);
        sock_ = INVALID_SOCKET;
        throw ConnectionError("Invalid IP address: " + ip);
    }
}

void UdpClient::close() {
    if (sock_ != INVALID_SOCKET) {
        closesocket(sock_);
        sock_ = INVALID_SOCKET;
    }
}

bool UdpClient::is_open() const {
    return sock_ != INVALID_SOCKET;
}

void UdpClient::send(const uint8_t* data, size_t len) {
    if (!is_open()) throw DeviceNotConnectedError("UDP socket not open");
    int sent = sendto(sock_, reinterpret_cast<const char*>(data),
                      static_cast<int>(len), 0,
                      reinterpret_cast<struct sockaddr*>(&addr_),
                      sizeof(addr_));
    if (sent < 0)
        throw ConnectionError("UDP send failed");
}

void UdpClient::send(const std::vector<uint8_t>& data) {
    send(data.data(), data.size());
}

size_t UdpClient::receive(uint8_t* buf, size_t max_len, uint32_t timeout_ms) {
    if (!is_open()) throw DeviceNotConnectedError("UDP socket not open");

    DWORD tv = timeout_ms;
    setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO,
               reinterpret_cast<const char*>(&tv), sizeof(tv));

    int n = recv(sock_, reinterpret_cast<char*>(buf), static_cast<int>(max_len), 0);
    return (n > 0) ? static_cast<size_t>(n) : 0;
}

std::vector<uint8_t> UdpClient::receive_vec(size_t max_len, uint32_t timeout_ms) {
    std::vector<uint8_t> buf(max_len);
    size_t n = receive(buf.data(), max_len, timeout_ms);
    buf.resize(n);
    return buf;
}

UdpClient::~UdpClient() { close(); }

} // namespace im

#else
// ============================================================================
//  POSIX implementation
// ============================================================================

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

namespace im {

void UdpClient::open(const std::string& ip, uint16_t port) {
    if (is_open()) close();

    sock_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_ < 0)
        throw ConnectionError("Failed to create UDP socket");

    std::memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr) <= 0) {
        ::close(sock_);
        sock_ = -1;
        throw ConnectionError("Invalid IP address: " + ip);
    }
}

void UdpClient::close() {
    if (sock_ >= 0) {
        ::close(sock_);
        sock_ = -1;
    }
}

bool UdpClient::is_open() const {
    return sock_ >= 0;
}

void UdpClient::send(const uint8_t* data, size_t len) {
    if (!is_open()) throw DeviceNotConnectedError("UDP socket not open");
    ssize_t sent = sendto(sock_, data, len, 0,
                          reinterpret_cast<const struct sockaddr*>(&addr_),
                          sizeof(addr_));
    if (sent < 0)
        throw ConnectionError("UDP send failed");
}

void UdpClient::send(const std::vector<uint8_t>& data) {
    send(data.data(), data.size());
}

size_t UdpClient::receive(uint8_t* buf, size_t max_len, uint32_t timeout_ms) {
    if (!is_open()) throw DeviceNotConnectedError("UDP socket not open");

    struct timeval tv;
    tv.tv_sec  = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    ssize_t n = recv(sock_, buf, max_len, 0);
    return (n > 0) ? static_cast<size_t>(n) : 0;
}

std::vector<uint8_t> UdpClient::receive_vec(size_t max_len, uint32_t timeout_ms) {
    std::vector<uint8_t> buf(max_len);
    size_t n = receive(buf.data(), max_len, timeout_ms);
    buf.resize(n);
    return buf;
}

UdpClient::~UdpClient() { close(); }

} // namespace im

#endif
