#pragma once
#include <QString>
#include <Qt>

#include "typing/HostKey.hpp"

// A Qt key event as the library's typing engine takes it (LIB-5 AC-7):
// which key (`physical`, `named`), which modifiers by what the user pressed,
// the character the layout produced and, for a letter, the letter the key
// carries. Nothing is mapped here: the product's keyboard map in the session
// decides what the key types (app/SharpTyping).
namespace QtHostKeys {

// `key`/`modifiers`/`text` as plain values (not QKeyEvent*), so this stays a
// small, directly testable function. `nativeVirtualKey` is
// QKeyEvent::nativeVirtualKey(), only read on macOS for keys Qt has no
// Qt::Key for (a PC keyboard's Scroll Lock); `physical` pairs a key-down
// with its key-up.
calcu::typing::HostKeyEvent hostKeyEvent(Qt::Key key, Qt::KeyboardModifiers modifiers, const QString& text,
                                         quint32 nativeVirtualKey, const std::string& physical);

// A command modifier is held (Command or Control on a Mac; Control alone,
// Alt alone or the Windows key elsewhere): the event is also the window's
// (a shortcut no action claimed).
bool commandModifier(Qt::KeyboardModifiers modifiers);

} // namespace QtHostKeys
