# InputManager

SDK C++ unifié pour contrôler des périphériques d'entrée matériels : **KMBox B**, **KMBox Net** et **Makcu**.

## Appareils supportés

| Appareil | Transport | Protocole | Baud / Port |
|-----------|-----------|-----------|-------------|
| KMBox B | USB Serial | Binaire `[0x57][0xAB][cmd][len][data][sum]` | 115200 |
| KMBox Net | UDP | Binaire `[MAGIC][cmd][rand][len][data]` + AES-128-ECB | IP:16820 |
| Makcu | USB Serial | Binaire `[0xFA][cmd][len][data][xor]` | 128000 |

## Build

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## API rapide

### Un seul appareil

```cpp
#include <input_manager/input_manager.hpp>

// KMBox B (série)
im::KMBoxB device("COM3");
device.connect();
device.mouse_move(100, 50);
device.mouse_click();
device.key_tap(im::KeyCode::A);
device.type_string("Hello!");
device.disconnect();

// KMBox Net (réseau)
im::KMBoxNet net("192.168.1.100", 16820, "uuid");
net.set_encryption(true);
net.connect();
net.mouse_move_absolute(960, 540);
net.mouse_move_smooth(200, 100, 500);
net.disconnect();

// Makcu (série)
im::Makcu makcu("/dev/ttyUSB0", 128000);
makcu.connect();
makcu.set_dpi(800);
makcu.mouse_move(50, -30);
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
mgr.get_as<im::Makcu>("makcu").set_dpi(1600);

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

### Fonctions spécifiques

| KMBox B | KMBox Net | Makcu |
|---------|-----------|-------|
| `set_mouse_mask(x, y)` | `set_encryption(bool)` | `set_dpi(dpi)` |
| | `set_monitor_resolution(w, h)` | `set_poll_rate(hz)` |
| | `monitor_capture()` | |

## Types

```cpp
im::MouseButton::Left | Right | Middle | Side1 | Side2
im::KeyModifier::LeftCtrl | LeftShift | LeftAlt | LeftGui | Right...
im::KeyCode::A..Z | Num0..9 | F1..F12 | Enter | Space | ...
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
    ├── kmbox_b.hpp
    ├── kmbox_net.hpp
    └── makcu.hpp
```

## Dépendances

Aucune dépendance externe. Le SDK est auto-contenu avec :
- Implémentation AES-128-ECB intégrée pour KMBox Net
- Communication série POSIX/Win32 native
- Sockets UDP POSIX/Winsock natifs

## Licence

MIT
