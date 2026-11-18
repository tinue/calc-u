#include "qt6/ClipboardService.hpp"
#include "qt6/MacClipboard.h"
#include "sessions/Args.hpp"
#include <QClipboard>
#include <QGuiApplication>
#include <QImage>
#include <QMimeData>
#include <cmath>

namespace calcu::qt6 {

using namespace calcu::sessions;

ClipboardService::ClipboardService(QClipboard* clipboard) : clipboard_(clipboard) {}

QClipboard* ClipboardService::clipboard() const { return clipboard_ ? clipboard_ : QGuiApplication::clipboard(); }

void ClipboardService::put(QMimeData* m) {
    if (sink_) sink_(m);
    else if (QClipboard* c = clipboard()) c->setMimeData(m);
    else delete m;
}

QMimeData* ClipboardService::imageEntry(const Value& args, QString* problem) {
    const auto bytes = bytesArg(args, "bytes");
    QImage image;
    if (bytes) image.loadFromData(bytes->data(), static_cast<int>(bytes->size()), "PNG");
    if (image.isNull()) {
        if (problem) *problem = QStringLiteral("The picture could not be read.");
        return nullptr;
    }
    const double dpi = realArg(args, "dotsPerInch", 0);
    if (dpi > 0) {
        const int perMetre = static_cast<int>(std::lround(dpi / 0.0254));
        image.setDotsPerMeterX(perMetre);
        image.setDotsPerMeterY(perMetre);
    }
    auto* m = new QMimeData;
    m->setImageData(image);
    if (const Value* t = args.find("text"); t && t->string()) m->setText(QString::fromStdString(*t->string()));
    return m;
}

ServiceAnswer ClipboardService::answer(const ServiceAsk& ask) {
    if (ask.service == HostService::ClipboardImage) {
        QString problem;
        QMimeData* m = imageEntry(ask.args, &problem);
        if (!m) return ServiceAnswer{false, Value(), problem.toStdString()};
#ifdef Q_OS_MACOS
        // Cocoa keeps the physical size that Qt's clipboard drops. Only
        // on the Cocoa platform (the offscreen one of the tests keeps its own
        // clipboard) and not for a sink of the tests.
        const double dpi = realArg(ask.args, "dotsPerInch", 0);
        if (!sink_ && !clipboard_ && dpi > 0 && QGuiApplication::platformName() == QLatin1String("cocoa")) {
            const QImage image = qvariant_cast<QImage>(m->imageData());
            const QString text = m->hasText() ? m->text() : QString();
            delete m;
            if (macSetClipboardImage(image, image.width() * 72.0 / dpi, image.height() * 72.0 / dpi, text))
                return ServiceAnswer{true, Value(), {}};
            m = imageEntry(ask.args, &problem);
            if (!m) return ServiceAnswer{false, Value(), problem.toStdString()};
        }
#endif
        put(m);
        return ServiceAnswer{true, Value(), {}};
    }
    if (ask.service == HostService::ClipboardText) {
        if (const Value* t = ask.args.find("text"); t && t->string()) {
            auto* m = new QMimeData;
            m->setText(QString::fromStdString(*t->string()));
            put(m);
            return ServiceAnswer{true, Value(), {}};
        }
        QClipboard* c = clipboard();
        const std::string text = c ? c->text().toStdString() : std::string();
        return ServiceAnswer{true, Value::object({{"text", text}}), {}};
    }
    return ServiceAnswer{false, Value(), "This frontend does not offer " + std::string(serviceName(ask.service)) + "."};
}

} // namespace calcu::qt6
