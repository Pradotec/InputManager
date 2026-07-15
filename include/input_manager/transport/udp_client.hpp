#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
using socket_t = SOCKET;
#else
#include <netinet/in.h>
using socket_t = int;
#endif

namespace im {

class UdpClient {
public:
    UdpClient() = default;
    ~UdpClient();

    UdpClient(const UdpClient&) = delete;
    UdpClient& operator=(const UdpClient&) = delete;

    void open(const std::string& ip, uint16_t port);
    void close();
    bool is_open() const;

    void send(const uint8_t* data, size_t len);
    void send(const std::vector<uint8_t>& data);

    size_t receive(uint8_t* buf, size_t max_len, uint32_t timeout_ms = 500);
    std::vector<uint8_t> receive_vec(size_t max_len = 4096, uint32_t timeout_ms = 500);

private:
#ifdef _WIN32
    socket_t sock_ = INVALID_SOCKET;
    static bool wsa_initialized_;
    static void init_wsa();
#else
    socket_t sock_ = -1;
#endif
    struct sockaddr_in addr_ {};
};

} // namespace im
