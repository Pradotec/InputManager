#include <input_manager/input_manager.hpp>
#include <iostream>
#include <thread>

int main() {
    try {
        im::KMBoxB device("COM3"); // Linux: "/dev/ttyUSB0"
        device.connect();

        auto info = device.get_info();
        std::cout << "Connected to " << info.device_type
                  << " (firmware " << info.firmware_version << ")\n";

        // move mouse 100px right, 50px down
        device.mouse_move(100, 50);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // smooth movement over 500ms
        device.mouse_move_smooth(-100, -50, 500);
        std::this_thread::sleep_for(std::chrono::milliseconds(600));

        // click
        device.mouse_click(im::MouseButton::Left);

        // type text
        device.type_string("Hello from KMBox B!", 30);

        // key combo: Ctrl+S
        device.key_tap(im::KeyCode::S, im::KeyModifier::LeftCtrl);

        device.disconnect();
        std::cout << "Done.\n";

    } catch (const im::InputManagerError& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
