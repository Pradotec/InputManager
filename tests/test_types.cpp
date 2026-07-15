#include "input_manager/core/types.hpp"
#include <cassert>
#include <iostream>

int main() {
    // MouseButton flags
    auto combined = im::MouseButton::Left | im::MouseButton::Right;
    assert(im::has_flag(combined, im::MouseButton::Left));
    assert(im::has_flag(combined, im::MouseButton::Right));
    assert(!im::has_flag(combined, im::MouseButton::Middle));

    // KeyModifier flags
    auto mods = im::KeyModifier::LeftCtrl | im::KeyModifier::LeftShift;
    assert(static_cast<uint8_t>(mods) == 0x03);

    // char_map lookup
    const auto& cmap = im::char_map();
    auto it = cmap.find('a');
    assert(it != cmap.end());
    assert(it->second.key == im::KeyCode::A);
    assert(it->second.modifier == im::KeyModifier::None);

    it = cmap.find('A');
    assert(it != cmap.end());
    assert(it->second.key == im::KeyCode::A);
    assert(it->second.modifier == im::KeyModifier::LeftShift);

    it = cmap.find('!');
    assert(it != cmap.end());
    assert(it->second.key == im::KeyCode::Num1);
    assert(it->second.modifier == im::KeyModifier::LeftShift);

    it = cmap.find(' ');
    assert(it != cmap.end());
    assert(it->second.key == im::KeyCode::Space);

    std::cout << "All type tests passed.\n";
    return 0;
}
