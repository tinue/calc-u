#pragma once
#include <QLabel>

// In place of the faceplate when the product could not start: the text of
// the `roms-missing` topic, selectable, wrapped. Object name `roms-missing`.

namespace calcu::qt6 {

class RomsMissingView : public QLabel {
    Q_OBJECT
public:
    explicit RomsMissingView(QWidget* parent = nullptr);
    void setMessage(const QString&);
};

} // namespace calcu::qt6
