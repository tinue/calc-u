#pragma once
#include "sessions/HostServices.hpp"
#include "sessions/Messages.hpp"
#include <QString>
#include <functional>

class QClipboard;
class QMimeData;

// The clipboard host services of a Qt6 frontend (LIB-8 design §2, §3.6;
// spec AC-16; LIB-3 AC-26, AC-27): answers `clipboard-image` ({bytes, text?,
// name, dotsPerInch}: one clipboard entry with the picture, its resolution
// set on the image, and the text when there is one) and `clipboard-text`
// ({text}: copied; without text: the clipboard's text, {text}). From M-43
// LIB-14's SessionHost asks it through its service handler.

namespace calcu::qt6 {

class ClipboardService {
public:
    // The clipboard the entries go to; nullptr: QGuiApplication::clipboard().
    explicit ClipboardService(QClipboard* clipboard = nullptr);

    static bool handles(sessions::HostService s) {
        return s == sessions::HostService::ClipboardImage || s == sessions::HostService::ClipboardText;
    }
    // The answer to an ask of a service it handles; any other: not given.
    sessions::ServiceAnswer answer(const sessions::ServiceAsk&);

    // The seam (design §8): the entry an ask of `clipboard-image` puts on the
    // clipboard, or nullptr with `problem` set. The caller owns it.
    static QMimeData* imageEntry(const sessions::Value& args, QString* problem = nullptr);
    // Where the entries go instead of the clipboard (the tests); it takes the
    // entry over. Unset: the clipboard.
    void setSink(std::function<void(QMimeData*)> sink) { sink_ = std::move(sink); }

private:
    QClipboard* clipboard() const;
    void put(QMimeData*);

    QClipboard* clipboard_;
    std::function<void(QMimeData*)> sink_;
};

} // namespace calcu::qt6
