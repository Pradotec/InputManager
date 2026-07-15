#include "input_manager/input_manager.hpp"
#include <cassert>
#include <iostream>

int main() {
    im::InputManager mgr;

    // starts empty
    assert(mgr.device_count() == 0);
    assert(!mgr.has("foo"));

    // add devices (they won't connect without real hardware, but registration works)
    mgr.add_kmbox_b("kb", "COM99");
    assert(mgr.device_count() == 1);
    assert(mgr.has("kb"));
    assert(mgr.active_name() == "kb");

    mgr.add_kmbox_net("net", "127.0.0.1", 16820, "test-uuid");
    assert(mgr.device_count() == 2);

    mgr.add_makcu("mk", "COM98");
    assert(mgr.device_count() == 3);

    // device_names
    auto names = mgr.device_names();
    assert(names.size() == 3);

    // get by name
    assert(mgr.get("kb").device_name() == "KMBox B");
    assert(mgr.get("net").device_name() == "KMBox Net");
    assert(mgr.get("mk").device_name() == "Makcu");

    // operator[]
    assert(mgr["net"].device_name() == "KMBox Net");

    // get_as
    auto& net = mgr.get_as<im::KMBoxNet>("net");
    assert(net.device_name() == "KMBox Net");

    // set_active
    mgr.set_active("net");
    assert(mgr.active_name() == "net");
    assert(mgr.active().device_name() == "KMBox Net");

    // remove
    mgr.remove("kb");
    assert(mgr.device_count() == 2);
    assert(!mgr.has("kb"));

    // not connected
    assert(!mgr.get("net").is_connected());

    // DeviceNotFoundError
    bool caught = false;
    try { mgr.get("nonexistent"); }
    catch (const im::DeviceNotFoundError&) { caught = true; }
    assert(caught);

    // duplicate name
    caught = false;
    try { mgr.add_makcu("net", "COM97"); }
    catch (const im::InputManagerError&) { caught = true; }
    assert(caught);

    std::cout << "All manager tests passed.\n";
    return 0;
}
