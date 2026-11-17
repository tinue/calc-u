#include "QtResourceBundle.hpp"
#include <QDir>
#include <QFile>

namespace {
std::string& hidden() {
    static std::string name;
    return name;
}
} // namespace

void QtResourceBundle::hideForTests(std::string fileName) { hidden() = std::move(fileName); }

std::optional<std::vector<std::uint8_t>> QtResourceBundle::read(std::string_view fileName) const {
    if (!hidden().empty() && fileName == hidden()) return std::nullopt;
    const QString name = QString::fromUtf8(fileName.data(), static_cast<qsizetype>(fileName.size()));
    if (!QDir(QStringLiteral(":/roms")).entryList(QDir::Files).contains(name, Qt::CaseSensitive)) return std::nullopt;
    QFile f(QStringLiteral(":/roms/") + name);
    if (!f.open(QIODevice::ReadOnly)) return std::nullopt;
    const QByteArray b = f.readAll();
    const auto* data = reinterpret_cast<const std::uint8_t*>(b.constData());
    return std::vector<std::uint8_t>(data, data + b.size());
}

std::string QtResourceBundle::where() const { return "the app's bundled ROM files (:/roms/)"; }
