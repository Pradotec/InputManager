#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#endif

namespace im {

class SerialPort {
public:
    SerialPort() = default;
    ~SerialPort();

    SerialPort(const SerialPort&) = delete;
    SerialPort& operator=(const SerialPort&) = delete;

    void open(const std::string& port, uint32_t baud_rate);
    void close();
    bool is_open() const;

    void write(const uint8_t* data, size_t len);
    void write(const std::vector<uint8_t>& data);

    size_t read(uint8_t* buf, size_t max_len, uint32_t timeout_ms = 500);
    std::vector<uint8_t> read_exact(size_t count, uint32_t timeout_ms = 500);

    void flush();

    static std::vector<std::string> list_ports();

private:
#ifdef _WIN32
    HANDLE handle_ = INVALID_HANDLE_VALUE;
#else
    int fd_ = -1;
    struct termios original_tio_ {};
#endif
};

} // namespace im
