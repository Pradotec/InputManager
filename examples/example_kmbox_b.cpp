#include <input_manager/input_manager.hpp>
#include <iostream>
#include <thread>

int main() {
    try {
        // CH340 USB-serial, default 115200 baud
        // Device discovery: look for "USB-SERIAL CH340" COM port
        im::KMBoxB device("COM3"); // Linux: "/dev/ttyUSB0"
        device.connect();

        std::cout << "Connected to " << device.device_name() << "\n";

        // -- mouse --
        device.mouse_move(100, 50);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // move with speed parameter (KMBox B specific)
        device.mouse_move_speed(-100, -50, 10);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // smooth movement (interpolated)
        device.mouse_move_smooth(200, 100, 500);
        std::this_thread::sleep_for(std::chrono::milliseconds(600));

        // buttons — km.left(state), km.right(state), etc.
        device.mouse_click(im::MouseButton::Left);
        device.mouse_double_click(im::MouseButton::Left);

        // scroll
        device.mouse_scroll(3);
        device.mouse_scroll(-3);

        // -- mouse mask --
        device.set_mouse_mask(5, 5);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        device.clear_mouse_mask();

        // -- monitor physical input --
        device.monitor(1);
        std::cout << "Left button down: " << device.isdown_left() << "\n";
        std::cout << "Right button down: " << device.isdown_right() << "\n";
        device.monitor(0);

        // -- keyboard --
        device.type_string("Hello from KMBox B!", 30);

        // key combo: Ctrl+S (using USB HID codes)
        device.key_tap(im::KeyCode::S, im::KeyModifier::LeftCtrl);

        // -- LCD display (B Pro only) --
        device.lcd("Hello!");

        // -- USB identity (takes effect after reboot) --
        // device.set_vid("046D");
        // device.set_pid("C077");
        // device.set_baud(256000);

        device.disconnect();
        std::cout << "Done.\n";

    } catch (const im::InputManagerError& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
