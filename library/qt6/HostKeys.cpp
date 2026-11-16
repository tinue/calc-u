#include "qt6/HostKeys.hpp"

#include <QKeyEvent>
#include <QList>

namespace calcu::qt6 {

namespace {

using typing::NamedKey;

NamedKey namedKey(Qt::Key key, quint32 nativeVirtualKey) {
#ifdef Q_OS_MACOS
    // macOS reports a PC keyboard's Scroll Lock as kVK_ContextualMenu
    // (0x6E), which Qt has no Qt::Key for (it hands over the raw 0x10
    // character code instead) -- match the physical key code directly.
    if (nativeVirtualKey == 0x6E) return NamedKey::ScrollLock;
#else
    (void)nativeVirtualKey;
#endif
    switch (key) {
    case Qt::Key_Escape: return NamedKey::Escape;
    case Qt::Key_Backspace: return NamedKey::Backspace;
    case Qt::Key_Return: return NamedKey::Return;
    case Qt::Key_Enter: return NamedKey::Enter;
    case Qt::Key_Tab: return NamedKey::Tab;
    case Qt::Key_Delete: return NamedKey::Delete;
    case Qt::Key_Up: return NamedKey::Up;
    case Qt::Key_Down: return NamedKey::Down;
    case Qt::Key_Left: return NamedKey::Left;
    case Qt::Key_Right: return NamedKey::Right;
    case Qt::Key_F1: return NamedKey::F1;
    case Qt::Key_F2: return NamedKey::F2;
    case Qt::Key_F3: return NamedKey::F3;
    case Qt::Key_F4: return NamedKey::F4;
    case Qt::Key_F5: return NamedKey::F5;
    case Qt::Key_F6: return NamedKey::F6;
    case Qt::Key_ScrollLock: return NamedKey::ScrollLock;
#ifndef Q_OS_MACOS
    // On macOS, Insert arrives as Key_Help and stays unnamed.
    case Qt::Key_Insert: return NamedKey::Insert;
#endif
    case Qt::Key_Home: return NamedKey::Home;
    case Qt::Key_End: return NamedKey::End;
    case Qt::Key_PageUp: return NamedKey::PageUp;
    case Qt::Key_PageDown: return NamedKey::PageDown;
    default: return NamedKey::None;
    }
}

} // namespace

typing::HostKeyEvent hostKeyEvent(Qt::Key key, Qt::KeyboardModifiers modifiers, const QString& text,
                                  quint32 nativeVirtualKey, const std::string& physical) {
    typing::HostKeyEvent e;
    e.physical = physical;
    e.named = namedKey(key, nativeVirtualKey);
    e.mods.shift = modifiers & Qt::ShiftModifier;
    e.mods.alt = modifiers & Qt::AltModifier;
#ifdef Q_OS_MACOS
    // Qt maps Cmd to ControlModifier and the Control key to MetaModifier.
    e.mods.command = modifiers & Qt::ControlModifier;
    e.mods.control = modifiers & Qt::MetaModifier;
    e.mods.macHost = true;
#else
    e.mods.control = modifiers & Qt::ControlModifier;
    e.mods.meta = modifiers & Qt::MetaModifier;
#endif
    const QList<uint> chars = text.toUcs4();
    if (chars.size() == 1) {
        e.produced = static_cast<char32_t>(chars.at(0));
        const QString lower = text.toLower();
        if (text.at(0).isLetter() && lower.toUcs4().size() == 1) e.carried = static_cast<char32_t>(lower.toUcs4().at(0));
    }
    return e;
}

bool commandModifier(Qt::KeyboardModifiers modifiers) {
#ifdef Q_OS_MACOS
    return modifiers & (Qt::ControlModifier | Qt::MetaModifier);
#else
    const bool control = modifiers & Qt::ControlModifier, alt = modifiers & Qt::AltModifier;
    return (modifiers & Qt::MetaModifier) || control != alt;
#endif
}

quint32 physicalKeyId(const QKeyEvent& event) {
#ifdef Q_OS_MACOS
    // nativeScanCode() carries nothing on macOS; the virtual key code is
    // the physical key position (kVK_*), independent of the layout.
    return event.nativeVirtualKey();
#else
    if (event.nativeScanCode() != 0) return event.nativeScanCode();
    return static_cast<quint32>(event.key());
#endif
}

std::optional<typing::HostKeyEvent> HostKeys::press(const QKeyEvent& event) {
    const Qt::Key key = static_cast<Qt::Key>(event.key());
    if (key == Qt::Key_Control || key == Qt::Key_Meta || key == Qt::Key_Alt || key == Qt::Key_AltGr) return std::nullopt;
    const quint32 id = physicalKeyId(event);
    const std::string physical = std::to_string(id);
    down_.insert(id, physical);   // its key-up goes to the session
    return hostKeyEvent(key, event.modifiers(), event.text(), event.nativeVirtualKey(), physical);
}

std::optional<std::string> HostKeys::release(const QKeyEvent& event) {
    auto it = down_.find(physicalKeyId(event));
    if (it == down_.end()) return std::nullopt;
    std::string physical = it.value();
    down_.erase(it);
    return physical;
}

} // namespace calcu::qt6
