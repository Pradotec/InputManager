#include <input_manager/input_manager.hpp>
#include <iostream>
#include <thread>

int main() {
    try {
        // CH343 USB-serial, default 115200 baud (VID:PID 1A86:55D3)
        im::Makcu device("COM5"); // Linux: "/dev/ttyUSB0"
        device.connect();

        // firmware version
        std::cout << "Firmware: " << device.firmware_version() << "\n";

        // full device info
        auto info = device.device_info_full();
        std::cout << "Model: " << info.model << "\n"
                  << "MAC: "   << info.mac   << "\n"
                  << "CPU: "   << info.cpu   << "\n";

        // -- mouse --
        device.mouse_move(150, -75);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // native click with count and delay
        device.click(im::MakcuButton::Left, 2, 50);

        // scroll
        device.mouse_scroll(-3);

        // silent move (left-down -> move -> left-up)
        device.silent_move(50, 30);

        // -- turbo mode --
        device.turbo(im::MakcuButton::Left, 100); // 100ms interval
        std::this_thread::sleep_for(std::chrono::seconds(2));
        device.turbo_disable_all();

        // -- lock/unlock axes --
        device.lock(im::MakcuLockTarget::MX);
        std::cout << "MX locked: " << device.lock_state(im::MakcuLockTarget::MX) << "\n";
        device.unlock(im::MakcuLockTarget::MX);

        // -- keyboard --
        device.type_string("Hello from Makcu!");

        // key by name
        device.key_press_name("enter");

        // key combo: Ctrl+S using names
        device.key_down("lctrl");
        device.key_press_name("s");
        device.key_up("lctrl");

        // -- streaming mouse data --
        device.stream_set(im::MakcuStreamMode::Raw, 100);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        device.stream_set(im::MakcuStreamMode::Off);

        device.disconnect();
        std::cout << "Done.\n";

    } catch (const im::InputManagerError& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
