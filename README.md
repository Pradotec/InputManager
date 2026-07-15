# InputManager

SDK C++ unifié pour contrôler des périphériques d'entrée matériels : **KMBox B**, **KMBox Net** et **Makcu**.

## Appareils supportés

| Appareil | Transport | Protocole | Baud / Port |
|-----------|-----------|-----------|-------------|
| KMBox B | USB Serial | Binaire `[0x57][0xAB][cmd][len][data][sum]` | 115200 |
| KMBox Net | UDP | Binaire `[MAGIC][cmd][rand][len][data]` + AES-128-ECB | IP:16820 |
| Makcu | CH343 USB Serial | ASCII `km.<cmd>(<args>)\r\n` | 115200 (jusqu'à 4 Mbps) |

## Build

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## API rapide

### KMBox B (série, binaire)

```cpp
#include <input_manager/input_manager.hpp>

im::KMBoxB device("COM3");
device.connect();
device.mouse_move(100, 50);
device.mouse_click();
device.key_tap(im::KeyCode::A);
device.type_string("Hello!");
device.disconnect();
```

### KMBox Net (UDP, chiffré)

```cpp
im::KMBoxNet net("192.168.1.100", 16820, "uuid");
net.set_encryption(true);
net.connect();
net.mouse_move_absolute(960, 540);
net.mouse_move_smooth(200, 100, 500);
net.disconnect();
```

### Makcu (série, ASCII — protocole km.*)

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
mgr.get_as<im::KMBoxNet>("net").set_encryption(true);
mgr.get_as<im::Makcu>("makcu").turbo(im::MakcuButton::Left, 100);

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

| KMBox B | KMBox Net | Makcu |
|---------|-----------|-------|
| `set_mouse_mask(x, y)` | `set_encryption(bool)` | `click(btn, count, delay_ms)` |
| | `set_monitor_resolution(w, h)` | `silent_move(dx, dy)` |
| | `monitor_capture()` | `turbo(btn, delay_ms)` / `turbo_disable_all()` |
| | | `lock(target)` / `unlock(target)` / `lock_state(target)` |
| | | `stream_set(mode, period_ms)` / `stream_mode()` |
| | | `echo(bool)` |
| | | `serial_number()` / `set_serial(s)` |
| | | `device_info_full()` → MAC, CPU, TEMP, RAM... |
| | | `firmware_version()` |
| | | `key_down(name)` / `key_up(name)` / `key_press_name(name)` |

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
    ├── kmbox_b.hpp            ← KMBox B (série, binaire)
    ├── kmbox_net.hpp          ← KMBox Net (UDP, AES-128-ECB)
    └── makcu.hpp              ← Makcu (série ASCII, protocole km.*)
```

## Dépendances

Aucune dépendance externe. Le SDK est auto-contenu avec :
- Implémentation AES-128-ECB intégrée pour KMBox Net
- Communication série POSIX/Win32 native
- Sockets UDP POSIX/Winsock natifs

## Licence

MIT
