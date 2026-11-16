#pragma once
#include "media/MediaTopics.hpp"
#include <QDialog>
#include <vector>

class QLineEdit;
class QListWidget;
class QPushButton;

// The card dialogs of C59-7 (AC-14, AC-15; LIB-7 59): Arm Card lists the
// saved magnetic cards of the library's media topic by name (an outdated one
// as an entry that cannot be chosen, its hint as tooltip; "No cards found"
// when none) and gives back the chosen entry for `media.choose`; Arm Fresh
// Card asks a name, shows the saved cards, and gives back the name for
// `media.start-new` (the replace question is the session's `asks:`,
// confirmReplace). It reads no folder and decides no name rule. The
// counterpart of App/Views/CardPickerView.swift.
class CardPickerDialog : public QDialog {
    Q_OBJECT
public:
    enum class Mode { Arm, ArmFresh };
    /// `cards`: the card reader's saved cards (EmulatorController::savedCards);
    /// `folder`: the save folder, shown in the footer.
    CardPickerDialog(Mode mode, const std::vector<calcu::media::PickerEntry>& cards, const QString& folder,
                     QWidget* parent = nullptr);

    /// Arm Card: the chosen entry's id (empty if cancelled).
    QString selectedEntry() const { return m_selected; }
    /// Arm Fresh Card: the name typed.
    QString freshName() const;
    /// The session's replace question with Replace and Cancel; true: Replace.
    static bool confirmReplace(const QString& question, QWidget* parent);

    QListWidget* list() const { return m_list; }   // the tests
    void accept() override;

private:

    Mode m_mode;
    QString m_selected;
    QLineEdit* m_name = nullptr;
    QListWidget* m_list;
    QPushButton* m_ok = nullptr;
};
