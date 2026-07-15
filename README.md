# InputManager

SDK C++ unifié pour contrôler des périphériques d'entrée matériels : **KMBox B** (B+/B Pro), **KMBox Net** et **Makcu**.

## Appareils supportés

| Appareil | Transport | Protocole | Puce / Port |
|-----------|-----------|-----------|-------------|
| KMBox B / B+ / B Pro | USB Serial (CH340) | ASCII `km.<cmd>(<args>)\r\n` | 115200 baud |
| KMBox Net | UDP | Binaire + AES-128-ECB | IP:16820 |
| Makcu | USB Serial (CH343) | ASCII `km.<cmd>(<args>)\r\n` | 115200 (jusqu'à 4 Mbps) |

## Build

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## API rapide

### KMBox B / B+ / B Pro (série CH340, ASCII)

```cpp
#include <input_manager/input_manager.hpp>

im::KMBoxB device("COM3"); // Linux: "/dev/ttyUSB0"
device.connect();

// mouse
device.mouse_move(100, 50);
device.mouse_move_speed(200, 100, 10); // with speed param
device.mouse_click();
device.mouse_scroll(-3);

// buttons — km.left(state), km.right(state), etc.
device.mouse_press(im::MouseButton::Left);
device.mouse_release(im::MouseButton::Left);

// keyboard — km.keydown(hid_code), km.keyup(hid_code)
device.key_tap(im::KeyCode::A);
device.type_string("Hello!");

// monitor physical input
device.monitor(1);
bool pressed = device.isdown_left();
device.monitor(0);

// mouse mask
device.set_mouse_mask(5, 5);
device.clear_mouse_mask();

device.disconnect();
```

### KMBox Net (UDP, AES-128-ECB)

```cpp
im::KMBoxNet net("192.168.1.100", 16820, "uuid");
net.set_encryption(true);
net.connect();

// mouse: move, move_auto, move_beizer
net.mouse_move(200, 100);
net.move_auto(300, 200, 500);
net.move_beizer(400, 300, 800, 100, 200, 300, 100);

// buttons: left/right/middle/side1/side2(state)
net.left(1);  // press
net.left(0);  // release

// combined mouse report
net.mouse_combined(0x01, 50, 50, 0);

// encrypted variants
net.enc_move(100, -50);
net.enc_left(1);

// monitor physical input
net.monitor(1);
bool down = net.isdown_left();
net.monitor(0);

// mouse/keyboard mask
net.mask_mouse(10, 5);
net.unmask_mouse();

net.disconnect();
```

### Makcu (série CH343, ASCII — protocole km.*)

```cpp
im::Makcu makcu("/dev/ttyUSB0"); // VID:PID 1A86:55D3
makcu.connect();

// firmware & info
std::cout << makcu.firmware_version() << "\n";
auto info = makcu.device_info_full(); // MAC, CPU, TEMP, RAM...

// mouse
makcu.mouse_move(100, -50);
makcu.click(im::MakcuButton::Left, 2, 50); // double-click, 50ms delay
makcu.silent_move(30, 20);  // left-down -> move -> left-up
makcu.mouse_scroll(-3);

// turbo mode
makcu.turbo(im::MakcuButton::Left, 100); // rapid-fire 100ms
makcu.turbo_disable_all();

// lock/unlock axes & buttons
makcu.lock(im::MakcuLockTarget::MX);
makcu.unlock(im::MakcuLockTarget::MX);

// keyboard
makcu.type_string("Hello!");           // km.type() with auto-shift
makcu.key_press_name("enter");         // by name
makcu.key_down("lctrl");              // modifier down
makcu.key_press_name("s");            // Ctrl+S
makcu.key_up("lctrl");

// streaming mouse data
makcu.stream_set(im::MakcuStreamMode::Raw, 100);

makcu.disconnect();
```

### Plusieurs appareils

```cpp
im::InputManager mgr;

mgr.add_kmbox_b("kb", "COM3");
mgr.add_kmbox_net("net", "192.168.1.100", 16820, "uuid");
mgr.add_makcu("makcu", "COM5");

mgr.connect_all();

// appareil actif
mgr.set_active("net");
mgr.active().mouse_click();

// par nom
mgr["kb"].type_string("Hello");

// accès typé pour fonctions spécifiques
auto& kb = mgr.get_as<im::KMBoxB>("kb");
kb.monitor(1);
kb.set_mouse_mask(5, 5);

auto& net = mgr.get_as<im::KMBoxNet>("net");
net.set_encryption(true);
net.move_beizer(200, 100, 500, 50, 100, 150, 50);
net.enc_move(10, 10);

auto& makcu = mgr.get_as<im::Makcu>("makcu");
makcu.turbo(im::MakcuButton::Left, 100);

// opérations groupées
mgr.mouse_move_all(10, 0);
mgr.key_release_all_devices();
mgr.disconnect_all();
```

## Interface commune (`InputDevice`)

Tous les appareils partagent la même interface :

```
Mouse                          Keyboard                    Device
─────                          ────────                    ──────
mouse_move(dx, dy)             key_press(key, mods)        connect()
mouse_move_absolute(x, y)      key_release(key)            disconnect()
mouse_move_smooth(dx, dy, ms)  key_tap(key, mods)          is_connected()
mouse_press(button)            key_release_all()           device_name()
mouse_release(button)          type_string(text, delay)    get_info()
mouse_click(button)                                        reboot()
mouse_double_click(button)
mouse_scroll(delta)
```

### Fonctions spécifiques par appareil

| KMBox B / B+ / B Pro | KMBox Net | Makcu |
|---------|-----------|-------|
| `mouse_move_speed(dx, dy, speed)` | `set_encryption(bool)` / `encryption_enabled()` | `click(btn, count, delay_ms)` |
| `set_mouse_mask(x, y)` / `clear_mouse_mask()` | `move_auto(x, y, ms)` | `silent_move(dx, dy)` |
| `monitor(port)` | `move_beizer(x, y, ms, cx1, cy1, cx2, cy2)` | `turbo(btn, ms)` / `turbo_disable_all()` |
| `isdown_left/right/middle/side1/side2()` | `mouse_combined(btns, x, y, wheel)` | `lock(target)` / `unlock(target)` / `lock_state(target)` |
| `set_baud(rate)` | `left/right/middle/side1/side2(state)` | `stream_set(mode, period_ms)` / `stream_mode()` |
| `lcd(text)` (B Pro) | `wheel(direction)` | `echo(bool)` |
| `set_vid(vid)` / `set_pid(pid)` | `keydown/keyup(hid_code)` | `serial_number()` / `set_serial(s)` |
| | `monitor(port)` | `device_info_full()` → MAC, CPU, TEMP, RAM... |
| | `isdown_left/right/middle/side1/side2()` | `firmware_version()` |
| | `enc_move/left/right/middle/side1/side2/wheel()` | `key_down/up/press_name(name)` |
| | `mask_mouse(x, y)` / `unmask_mouse()` | |
| | `mask_keyboard(key)` / `unmask_keyboard()` | |

## Types

```cpp
// Commun
im::MouseButton::Left | Right | Middle | Side1 | Side2
im::KeyModifier::LeftCtrl | LeftShift | LeftAlt | LeftGui | Right...
im::KeyCode::A..Z | Num0..9 | F1..F12 | Enter | Space | ...

// Makcu spécifique
im::MakcuButton::Left | Right | Middle | Mouse4 | Mouse5
im::MakcuLockTarget::MX | MY | MW | ML | MM | MR | MS1 | MS2
im::MakcuStreamMode::Off | Raw | Mut
```

## Structure du projet

```
include/input_manager/
├── input_manager.hpp          ← include principal
├── core/
│   ├── types.hpp              ← MouseButton, KeyCode, KeyModifier
│   ├── errors.hpp             ← exceptions
│   └── device.hpp             ← interface abstraite InputDevice
├── transport/
│   ├── serial_port.hpp        ← communication série cross-platform
│   └── udp_client.hpp         ← client UDP cross-platform
└── devices/
    ├── kmbox_b.hpp            ← KMBox B/B+/B Pro (série CH340, ASCII km.*)
    ├── kmbox_net.hpp          ← KMBox Net (UDP, AES-128-ECB)
    └── makcu.hpp              ← Makcu (série CH343, ASCII km.*)
```

## Dépendances

Aucune dépendance externe. Le SDK est auto-contenu avec :
- Implémentation AES-128-ECB intégrée pour KMBox Net
- Communication série POSIX/Win32 native
- Sockets UDP POSIX/Winsock natifs

## Licence

MIT
