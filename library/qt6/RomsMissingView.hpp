#pragma once
#include <QLabel>

// In place of the faceplate when the product could not start (LIB-14
// design §2.2; spec AC-19): the text of the `roms-missing` topic (LIB-12
// AC-5), selectable, wrapped. Object name `roms-missing`.

namespace calcu::qt6 {

class RomsMissingView : public QLabel {
    Q_OBJECT
public:
    explicit RomsMissingView(QWidget* parent = nullptr);
    void setMessage(const QString&);
};

} // namespace calcu::qt6
