#pragma once
#include "media/MediaTopics.hpp"
#include "sessions/Messages.hpp"
#include "sessions/Value.hpp"
#include <QComboBox>
#include <functional>
#include <string>
#include <vector>

// The picker of a module slot or a holding media place in the Qt6 component
// set: one control-bar picker drawn from the place's entries in the topic
// `media`, as they are; no rule lives here (the control state has them). An
// entry shows its label and its description as its tooltip; an entry that
// cannot be chosen now is shown disabled, with its hint as its tooltip. The
// entry the topic marks `chosen` is the current one.
// Choosing an entry sends `media.choose {place, entry}`.
//
// Object name: media-picker-<place>.

namespace calcu::qt6 {

class MediaPicker : public QComboBox {
    Q_OBJECT
public:
    using Sender = std::function<void(const sessions::CommandName&, sessions::Value)>;
    explicit MediaPicker(QWidget* parent = nullptr);

    void setPlace(const media::PlaceState&);
    // Closed, the picker shows the chosen entry's short label ("ML") and
    // is as wide as that; open, the entries' labels.
    void setShowShort(bool on);
    void setSender(Sender s) { sender_ = std::move(s); }

    // What the picker shows now (the tests, the shell).
    QString entryLabel(int i) const;
    QString entryTooltip(int i) const;
    bool entryEnabled(int i) const;
    std::string entryId(int i) const;
    static QString emptyLabel() { return QStringLiteral("(empty)"); }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent*) override;

private:
    QString closedText() const;
    void chosen(int index);
    std::string place_;
    std::vector<std::string> ids_;
    std::vector<QString> shorts_;
    bool showShort_ = false;
    Sender sender_;
    bool filling_ = false;
};

} // namespace calcu::qt6
