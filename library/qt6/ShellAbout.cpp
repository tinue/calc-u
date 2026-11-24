#include "qt6/ShellAbout.hpp"
#include "qt6/HostInfo.hpp"
#include "shell/ObjectNames.hpp"
#include <QApplication>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QIcon>
#include <QLabel>
#include <QVBoxLayout>

namespace calcu::qt6 {

ShellAbout::ShellAbout(const shell::ShellProduct& product, QWidget* parent) : QDialog(parent) {
    const shell::AboutModel a = shell::about(product, hostSupportRows());
    setObjectName(shell::names::kAboutDialog);
    setWindowTitle(QString::fromStdString(a.title));
    setWindowModality(Qt::WindowModal);
    auto* l = new QVBoxLayout(this);
    // The application's window icon above the name, when it has one.
    if (const QIcon icon = QApplication::windowIcon(); !icon.isNull()) {
        auto* picture = new QLabel(this);
        picture->setObjectName(QStringLiteral("about-icon"));
        picture->setPixmap(icon.pixmap(64, 64));
        l->addWidget(picture, 0, Qt::AlignHCenter);
    }
    auto* name = new QLabel(QStringLiteral("<b>%1</b>").arg(QString::fromStdString(a.name).toHtmlEscaped()), this);
    name->setObjectName(QStringLiteral("about-name"));
    l->addWidget(name);
    auto* version = new QLabel(QString::fromStdString(a.version), this);
    version->setObjectName(QStringLiteral("about-version"));
    l->addWidget(version);
    for (const auto& line : a.lines) {
        auto* t = new QLabel(QString::fromStdString(line), this);
        t->setWordWrap(true);
        l->addWidget(t);
    }
    for (const auto& [label, url] : a.links) {
        auto* link = new QLabel(QStringLiteral("<a href=\"%1\">%2</a>")
                                    .arg(QString::fromStdString(url).toHtmlEscaped(),
                                         QString::fromStdString(label).toHtmlEscaped()),
                                this);
        link->setOpenExternalLinks(true);
        link->setTextInteractionFlags(Qt::TextBrowserInteraction);
        l->addWidget(link);
    }
    if (!a.support.empty()) {
        // Selectable, so a part can be copied too; Copy takes all of it.
        auto* heading = new QLabel(QStringLiteral("<b>%1</b>").arg(QString::fromUtf8(shell::kSupportHeading)), this);
        heading->setObjectName(QStringLiteral("about-support"));
        l->addSpacing(heading->fontMetrics().height() / 2);
        l->addWidget(heading);
        for (const auto& [label, value] : a.support) {
            auto* row = new QLabel(QString::fromStdString(label + ": " + value), this);
            row->setWordWrap(true);
            row->setTextInteractionFlags(Qt::TextSelectableByMouse);
            l->addWidget(row);
        }
    }
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);   // OK closes it
    buttons->setObjectName(QStringLiteral("about-close"));
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    // Copy: the dialog's texts as plain text, for a bug report.
    auto* copy = buttons->addButton(QStringLiteral("Copy"), QDialogButtonBox::ActionRole);
    copy->setObjectName(QStringLiteral("about-copy"));
    copy->setAutoDefault(false);
    const QString text = QString::fromStdString(shell::aboutText(a));
    connect(copy, &QPushButton::clicked, this, [text] { QGuiApplication::clipboard()->setText(text); });
    l->addWidget(buttons);
}

} // namespace calcu::qt6
