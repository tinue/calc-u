#pragma once
#include <QHash>
#include <QString>
#include <Qt>
#include <optional>
#include <string>

#include "typing/HostKey.hpp"

class QKeyEvent;

// A Qt key event as the library's typing engine takes it (the
// family's rule): which key (`physical`, `named`), which modifiers by what
// the user pressed, the character the layout produced and, for a letter,
// the letter the key carries. Nothing is mapped here: the product's
// keyboard map in the session decides what the key types. HostKeys keeps
// which physical keys are down, so each key-up goes to the session as
// `host-key-up {physical}` and a focus loss forgets them.

namespace calcu::qt6 {

// `key`/`modifiers`/`text` as plain values (not QKeyEvent*), so this stays a
// small, directly testable function. `nativeVirtualKey` is
// QKeyEvent::nativeVirtualKey(), only read on macOS for keys Qt has no
// Qt::Key for (a PC keyboard's Scroll Lock); `physical` pairs a key-down
// with its key-up.
typing::HostKeyEvent hostKeyEvent(Qt::Key key, Qt::KeyboardModifiers modifiers, const QString& text,
                                  quint32 nativeVirtualKey, const std::string& physical);

// A command modifier is held (Command or Control on a Mac; Control alone,
// Alt alone or the Windows key elsewhere): the event is also the window's
// (a shortcut no action claimed).
bool commandModifier(Qt::KeyboardModifiers modifiers);

// The host keys a window holds down. The window keeps auto-repeat and its
// product's own keys (Calc-U-1600's SHIFT tap) before these calls.
class HostKeys {
public:
    // The `host-key-down` of a press, recorded as held; none for a bare
    // modifier (Control, Meta, Alt, AltGr: no key, it does not stop a paste).
    std::optional<typing::HostKeyEvent> press(const QKeyEvent&);
    // The `physical` of the `host-key-up` of a release; none when the press
    // was not one of press()'s.
    std::optional<std::string> release(const QKeyEvent&);
    // Focus lost (the window sends `focus-lost`): nothing is held any more.
    void clear() { down_.clear(); }

private:
    QHash<quint32, std::string> down_;
};

} // namespace calcu::qt6
