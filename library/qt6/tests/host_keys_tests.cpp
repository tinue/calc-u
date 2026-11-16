// LIB-5 AC-7, AC-10, EC-1 (the family's rule): a Qt key event as the typing
// engine's `host-key-down`, and the keys a window holds down (HostKeys).
#include "TestHarness.hpp"
#include "qt6/HostKeys.hpp"
#include <QKeyEvent>

namespace {

using calcu::qt6::HostKeys;
using calcu::typing::NamedKey;

// A key event whose physical identity is `code` on every platform (the
// scan code elsewhere, the virtual key code on macOS).
QKeyEvent key(QEvent::Type type, int k, Qt::KeyboardModifiers mods, const QString& text, quint32 code) {
    return QKeyEvent(type, k, mods, code, code, 0, text);
}

} // namespace

TEST(lib5_ac7_qt_key_event_produced_and_carried) {
    const auto e = calcu::qt6::hostKeyEvent(Qt::Key_A, Qt::ShiftModifier, QStringLiteral("A"), 0, "30");
    CHECK_EQ(e.physical, std::string("30"));
    CHECK(e.named == NamedKey::None);
    CHECK(e.produced == U'A');
    CHECK(e.carried == U'a');
    CHECK(e.mods.shift && !e.mods.alt);
    const auto d = calcu::qt6::hostKeyEvent(Qt::Key_1, Qt::NoModifier, QStringLiteral("1"), 0, "2");
    CHECK(d.produced == U'1' && d.carried == 0);
    const auto esc = calcu::qt6::hostKeyEvent(Qt::Key_Escape, Qt::NoModifier, QString(QChar(0x1b)), 0, "1");
    CHECK(esc.named == NamedKey::Escape);
    const auto none = calcu::qt6::hostKeyEvent(Qt::Key_F1, Qt::NoModifier, QString(), 0, "59");
    CHECK(none.named == NamedKey::F1 && none.produced == 0);
}

TEST(lib5_ac7_qt_command_modifier) {
    CHECK(!calcu::qt6::commandModifier(Qt::NoModifier));
    CHECK(!calcu::qt6::commandModifier(Qt::ShiftModifier));
    CHECK(calcu::qt6::commandModifier(Qt::ControlModifier));
}

TEST(lib5_ec1_qt_host_keys_pair_down_and_up) {
    HostKeys keys;
    const QKeyEvent down = key(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier, QStringLiteral("a"), 30);
    const auto d = keys.press(down);
    CHECK(d && d->physical == "30" && d->produced == U'a');
    // The release pairs with the press by the physical key, whatever the
    // modifiers and the character are by then.
    const QKeyEvent up = key(QEvent::KeyRelease, Qt::Key_A, Qt::ShiftModifier, QStringLiteral("A"), 30);
    const auto u = keys.release(up);
    CHECK(u && *u == "30");
    CHECK(!keys.release(up));   // once
    // A bare modifier is no key.
    const QKeyEvent control = key(QEvent::KeyPress, Qt::Key_Control, Qt::ControlModifier, QString(), 29);
    CHECK(!keys.press(control));
    CHECK(!keys.release(key(QEvent::KeyRelease, Qt::Key_Control, Qt::NoModifier, QString(), 29)));
}

TEST(lib5_ac10_qt_host_keys_forgotten_on_focus_lost) {
    HostKeys keys;
    CHECK(keys.press(key(QEvent::KeyPress, Qt::Key_B, Qt::NoModifier, QStringLiteral("b"), 48)));
    keys.clear();
    CHECK(!keys.release(key(QEvent::KeyRelease, Qt::Key_B, Qt::NoModifier, QStringLiteral("b"), 48)));
}
