#pragma once
#include "media/MediaTopics.hpp"
#include "sessions/Messages.hpp"
#include "sessions/Value.hpp"
#include <QComboBox>
#include <functional>
#include <string>
#include <vector>

// The picker of a module slot or a holding media place in the Qt6 component
// set (LIB-7 design §2, §3.9, D-7; spec AC-8, AC-9): one control-bar picker
// drawn from the place's entries in the topic `media`, as they are; no rule
// lives here (LIB-4 TR). An entry shows its label and its description as its
// tooltip; an entry that cannot be chosen (outdated, AC-9; not now) is shown
// disabled, marked "(outdated)" when it has the outdated hint, with the hint
// as its tooltip. The entry of what the place holds is the current one.
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
    void setSender(Sender s) { sender_ = std::move(s); }

    // What the picker shows now (the tests, the shell).
    QString entryLabel(int i) const;
    QString entryTooltip(int i) const;
    bool entryEnabled(int i) const;
    std::string entryId(int i) const;
    static QString emptyLabel() { return QStringLiteral("(empty)"); }

private:
    void chosen(int index);
    std::string place_;
    std::vector<std::string> ids_;
    Sender sender_;
    bool filling_ = false;
};

} // namespace calcu::qt6
