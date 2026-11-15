#include "qt6/ShellAbout.hpp"
#include "shell/ObjectNames.hpp"
#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>

namespace calcu::qt6 {

ShellAbout::ShellAbout(const shell::ShellProduct& product, QWidget* parent) : QDialog(parent) {
    const shell::AboutModel a = shell::about(product);
    setObjectName(shell::names::kAboutDialog);
    setWindowTitle(QString::fromStdString(a.title));
    setWindowModality(Qt::WindowModal);
    auto* l = new QVBoxLayout(this);
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
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);   // one button to close it
    buttons->setObjectName(QStringLiteral("about-close"));
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    l->addWidget(buttons);
}

} // namespace calcu::qt6
