#include <input_manager/input_manager.hpp>
#include <iostream>
#include <thread>

int main() {
    try {
        im::InputManager mgr;

        // register multiple devices
        mgr.add_kmbox_b("kb",   "COM3");
        mgr.add_kmbox_net("net", "192.168.1.100", 16820, "my-uuid");
        mgr.add_makcu("makcu",  "COM5");

        // connect all at once
        mgr.connect_all();

        std::cout << "Connected " << mgr.device_count() << " devices:\n";
        for (const auto& name : mgr.device_names())
            std::cout << "  - " << name << " (" << mgr.get(name).device_name() << ")\n";

        // use the active device (first registered by default)
        mgr.active().mouse_move(50, 50);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // switch active device
        mgr.set_active("net");
        mgr.active().mouse_click(im::MouseButton::Left);

        // access devices by name
        mgr.get("makcu").type_string("Hello!", 30);

        // typed access for device-specific features

        // KMBox B: monitor, mask, isdown, lcd
        auto& kb = mgr.get_as<im::KMBoxB>("kb");
        kb.set_mouse_mask(5, 5);
        kb.monitor(1);
        std::cout << "KB left down: " << kb.isdown_left() << "\n";
        kb.monitor(0);
        kb.clear_mouse_mask();

        // KMBox Net: bezier move, encrypted calls, monitor
        auto& net = mgr.get_as<im::KMBoxNet>("net");
        net.set_encryption(true);
        net.move_beizer(200, 100, 500, 50, 100, 150, 50);
        net.enc_move(10, 10);

        // Makcu: turbo, lock
        auto& makcu = mgr.get_as<im::Makcu>("makcu");
        makcu.turbo(im::MakcuButton::Left, 200);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        makcu.turbo_disable_all();

        // move all mice simultaneously
        mgr.mouse_move_all(10, 0);

        // release all keys on all devices
        mgr.key_release_all_devices();

        mgr.disconnect_all();
        std::cout << "All devices disconnected.\n";

    } catch (const im::InputManagerError& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
