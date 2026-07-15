#include <input_manager/input_manager.hpp>
#include <iostream>
#include <thread>

int main() {
    try {
        im::KMBoxNet device("192.168.1.100", 16820, "your-device-uuid");

        // optional: enable AES-128-ECB encryption for all commands
        device.set_encryption(true);

        device.connect();

        auto info = device.get_info();
        std::cout << "Connected to " << info.device_type
                  << " (firmware " << info.firmware_version << ")\n";

        // -- mouse: move(x, y) --
        device.mouse_move(200, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // smooth movement: move_auto(x, y, duration_ms)
        device.move_auto(300, 200, 1000);
        std::this_thread::sleep_for(std::chrono::milliseconds(1100));

        // bezier curve: move_beizer(x, y, duration, cx1, cy1, cx2, cy2)
        device.move_beizer(400, 300, 800, 100, 200, 300, 100);
        std::this_thread::sleep_for(std::chrono::milliseconds(900));

        // -- buttons: left/right/middle/side1/side2(state) --
        device.left(1);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        device.left(0);

        // or use the generic interface
        device.mouse_click(im::MouseButton::Right);

        // combined mouse report: mouse(buttons, x, y, wheel)
        device.mouse_combined(0x01, 50, 50, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        device.mouse_combined(0x00, 0, 0, 0);

        // scroll: wheel(direction) — 1=up, -1=down
        device.wheel(3);

        // -- encrypted variants — force AES for these calls --
        device.enc_move(100, -50);
        device.enc_left(1);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        device.enc_left(0);

        // -- monitor physical input --
        device.monitor(1);
        std::cout << "Physical left down: " << device.isdown_left() << "\n";
        std::cout << "Physical right down: " << device.isdown_right() << "\n";
        device.monitor(0);

        // -- mouse mask --
        device.mask_mouse(10, 5);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        device.unmask_mouse();

        // -- keyboard: keydown/keyup with USB HID codes --
        device.keydown(0x04); // 'A'
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        device.keyup(0x04);

        // or use the generic interface
        device.type_string("KMBox Net connected!", 25);
        device.key_tap(im::KeyCode::Enter);

        // keyboard mask
        device.mask_keyboard(0x04);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        device.unmask_keyboard();

        device.disconnect();
        std::cout << "Done.\n";

    } catch (const im::InputManagerError& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
