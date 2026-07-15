#include <input_manager/input_manager.hpp>
#include <iostream>
#include <thread>

int main() {
    try {
        im::KMBoxNet device("192.168.1.100", 16820, "your-device-uuid");

        // optional: enable AES encryption
        device.set_encryption(true);

        device.connect();

        auto info = device.get_info();
        std::cout << "Connected to " << info.device_type
                  << " (firmware " << info.firmware_version << ")\n";

        // relative mouse move
        device.mouse_move(200, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // absolute position
        device.mouse_move_absolute(960, 540);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // smooth bezier-like movement
        device.mouse_move_smooth(300, 200, 1000);
        std::this_thread::sleep_for(std::chrono::milliseconds(1100));

        // right click
        device.mouse_click(im::MouseButton::Right);

        // scroll up
        device.mouse_scroll(3);

        // type text
        device.type_string("KMBox Net connected!", 25);

        // key combo: Alt+F4
        device.key_tap(im::KeyCode::F4, im::KeyModifier::LeftAlt);

        device.disconnect();
        std::cout << "Done.\n";

    } catch (const im::InputManagerError& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
