#include <input_manager/input_manager.hpp>
#include <iostream>
#include <thread>

int main() {
    try {
        im::Makcu device("COM5"); // Linux: "/dev/ttyUSB0"
        device.connect();

        auto info = device.get_info();
        std::cout << "Connected to " << info.device_type
                  << " (firmware " << info.firmware_version << ")\n";

        // configure device
        device.set_dpi(800);
        device.set_poll_rate(1000);

        // move mouse
        device.mouse_move(150, -75);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // smooth movement
        device.mouse_move_smooth(200, 100, 800);
        std::this_thread::sleep_for(std::chrono::milliseconds(900));

        // double click
        device.mouse_double_click(im::MouseButton::Left);

        // scroll down
        device.mouse_scroll(-5);

        // type text
        device.type_string("Makcu works!", 20);

        // key combo: Ctrl+C
        device.key_tap(im::KeyCode::C, im::KeyModifier::LeftCtrl);

        device.disconnect();
        std::cout << "Done.\n";

    } catch (const im::InputManagerError& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
