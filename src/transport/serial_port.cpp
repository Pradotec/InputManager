#include "input_manager/transport/serial_port.hpp"
#include "input_manager/core/errors.hpp"

#include <cstring>
#include <stdexcept>

#ifdef _WIN32
// ============================================================================
//  Windows implementation
// ============================================================================

namespace im {

void SerialPort::open(const std::string& port, uint32_t baud_rate) {
    if (is_open()) close();

    std::string device_path = port;
    if (port.substr(0, 4) != "\\\\.\\")
        device_path = "\\\\.\\" + port;

    handle_ = CreateFileA(
        device_path.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0, nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (handle_ == INVALID_HANDLE_VALUE)
        throw ConnectionError("Failed to open serial port: " + port);

    DCB dcb {};
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(handle_, &dcb)) {
        CloseHandle(handle_);
        handle_ = INVALID_HANDLE_VALUE;
        throw ConnectionError("Failed to get serial port state");
    }

    dcb.BaudRate = baud_rate;
    dcb.ByteSize = 8;
    dcb.Parity   = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;
    dcb.fBinary = TRUE;

    if (!SetCommState(handle_, &dcb)) {
        CloseHandle(handle_);
        handle_ = INVALID_HANDLE_VALUE;
        throw ConnectionError("Failed to configure serial port");
    }

    COMMTIMEOUTS timeouts {};
    timeouts.ReadIntervalTimeout         = 50;
    timeouts.ReadTotalTimeoutMultiplier  = 10;
    timeouts.ReadTotalTimeoutConstant    = 500;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant   = 500;
    SetCommTimeouts(handle_, &timeouts);

    PurgeComm(handle_, PURGE_RXCLEAR | PURGE_TXCLEAR);
}

void SerialPort::close() {
    if (handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(handle_);
        handle_ = INVALID_HANDLE_VALUE;
    }
}

bool SerialPort::is_open() const {
    return handle_ != INVALID_HANDLE_VALUE;
}

void SerialPort::write(const uint8_t* data, size_t len) {
    if (!is_open()) throw DeviceNotConnectedError("Serial port not open");
    DWORD written = 0;
    if (!WriteFile(handle_, data, static_cast<DWORD>(len), &written, nullptr) ||
        written != static_cast<DWORD>(len)) {
        throw ConnectionError("Serial write failed");
    }
}

void SerialPort::write(const std::vector<uint8_t>& data) {
    write(data.data(), data.size());
}

size_t SerialPort::read(uint8_t* buf, size_t max_len, uint32_t timeout_ms) {
    if (!is_open()) throw DeviceNotConnectedError("Serial port not open");

    COMMTIMEOUTS timeouts {};
    timeouts.ReadTotalTimeoutConstant = timeout_ms;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    SetCommTimeouts(handle_, &timeouts);

    DWORD bytes_read = 0;
    ReadFile(handle_, buf, static_cast<DWORD>(max_len), &bytes_read, nullptr);
    return static_cast<size_t>(bytes_read);
}

std::vector<uint8_t> SerialPort::read_exact(size_t count, uint32_t timeout_ms) {
    std::vector<uint8_t> result(count);
    size_t total = 0;
    while (total < count) {
        size_t n = read(result.data() + total, count - total, timeout_ms);
        if (n == 0)
            throw DeviceTimeoutError("Serial read timed out");
        total += n;
    }
    return result;
}

void SerialPort::flush() {
    if (is_open())
        FlushFileBuffers(handle_);
}

std::vector<std::string> SerialPort::list_ports() {
    std::vector<std::string> ports;
    char target[512];
    for (int i = 1; i <= 256; ++i) {
        std::string name = "COM" + std::to_string(i);
        if (QueryDosDeviceA(name.c_str(), target, sizeof(target)) != 0)
            ports.push_back(name);
    }
    return ports;
}

SerialPort::~SerialPort() { close(); }

} // namespace im

#else
// ============================================================================
//  POSIX implementation (Linux / macOS)
// ============================================================================

#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <unistd.h>

namespace im {

void SerialPort::open(const std::string& port, uint32_t baud_rate) {
    if (is_open()) close();

    fd_ = ::open(port.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd_ < 0)
        throw ConnectionError("Failed to open serial port: " + port);

    int flags = fcntl(fd_, F_GETFL, 0);
    fcntl(fd_, F_SETFL, flags & ~O_NONBLOCK);

    tcgetattr(fd_, &original_tio_);

    struct termios tio {};
    tio.c_cflag = CS8 | CLOCAL | CREAD;
    tio.c_iflag = IGNPAR;
    tio.c_oflag = 0;
    tio.c_lflag = 0;
    tio.c_cc[VMIN]  = 0;
    tio.c_cc[VTIME] = 5;

    speed_t speed;
    switch (baud_rate) {
        case 9600:   speed = B9600;   break;
        case 19200:  speed = B19200;  break;
        case 38400:  speed = B38400;  break;
        case 57600:  speed = B57600;  break;
        case 115200: speed = B115200; break;
#ifdef B128000
        case 128000: speed = B128000; break;
#endif
        case 230400: speed = B230400; break;
        case 460800: speed = B460800; break;
#ifdef B921600
        case 921600: speed = B921600; break;
#endif
        default:
            speed = B115200;
            break;
    }
    cfsetispeed(&tio, speed);
    cfsetospeed(&tio, speed);

    tcflush(fd_, TCIFLUSH);
    tcsetattr(fd_, TCSANOW, &tio);
}

void SerialPort::close() {
    if (fd_ >= 0) {
        tcsetattr(fd_, TCSANOW, &original_tio_);
        ::close(fd_);
        fd_ = -1;
    }
}

bool SerialPort::is_open() const {
    return fd_ >= 0;
}

void SerialPort::write(const uint8_t* data, size_t len) {
    if (!is_open()) throw DeviceNotConnectedError("Serial port not open");
    ssize_t written = ::write(fd_, data, len);
    if (written < 0 || static_cast<size_t>(written) != len)
        throw ConnectionError("Serial write failed");
}

void SerialPort::write(const std::vector<uint8_t>& data) {
    write(data.data(), data.size());
}

size_t SerialPort::read(uint8_t* buf, size_t max_len, uint32_t timeout_ms) {
    if (!is_open()) throw DeviceNotConnectedError("Serial port not open");

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd_, &fds);

    struct timeval tv;
    tv.tv_sec  = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    int ret = select(fd_ + 1, &fds, nullptr, nullptr, &tv);
    if (ret <= 0) return 0;

    ssize_t n = ::read(fd_, buf, max_len);
    return (n > 0) ? static_cast<size_t>(n) : 0;
}

std::vector<uint8_t> SerialPort::read_exact(size_t count, uint32_t timeout_ms) {
    std::vector<uint8_t> result(count);
    size_t total = 0;
    while (total < count) {
        size_t n = read(result.data() + total, count - total, timeout_ms);
        if (n == 0)
            throw DeviceTimeoutError("Serial read timed out");
        total += n;
    }
    return result;
}

void SerialPort::flush() {
    if (fd_ >= 0) tcdrain(fd_);
}

std::vector<std::string> SerialPort::list_ports() {
    std::vector<std::string> ports;
    DIR* dir = opendir("/dev");
    if (!dir) return ports;
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name.find("ttyUSB") == 0 || name.find("ttyACM") == 0 ||
            name.find("tty.usb") == 0 || name.find("cu.usb") == 0) {
            ports.push_back("/dev/" + name);
        }
    }
    closedir(dir);
    return ports;
}

SerialPort::~SerialPort() { close(); }

} // namespace im

#endif
