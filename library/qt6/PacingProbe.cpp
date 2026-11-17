#include "qt6/PacingProbe.hpp"
#include "qt6/AttachLineClient.hpp"
#include <QApplication>
#include <QElapsedTimer>
#include <QTimer>
#include <cstdio>

namespace calcu::qt6 {

namespace {

double secondsOf(const AttachLineClient::Reply& r) {
    for (const QString& l : r.lines)
        if (l.startsWith(QLatin1String("seconds: "))) return l.mid(9).toDouble();
    return -1;
}

unsigned long long ticksOf(const AttachLineClient::Reply& r) {
    for (const QString& l : r.lines)
        if (l.startsWith(QLatin1String("ticks: "))) return l.mid(7).toULongLong();
    return 0;
}

} // namespace

int runPacingProbe(QApplication& app, std::uint16_t attachPort, double seconds,
                   const std::function<int(const PacingSample&)>& report) {
    AttachLineClient client;
    if (!client.connectTo(attachPort)) {
        std::printf("pacing-probe: attach is not open on port %u\n", attachPort);
        return 1;
    }
    int result = 1;
    PacingSample sample;
    QElapsedTimer wall;
    client.send(QStringLiteral("wait"), [&](const AttachLineClient::Reply&) {
        client.send(QStringLiteral("ticks"), [&](const AttachLineClient::Reply& r) {
            sample.ticksBefore = ticksOf(r);
            sample.secondsBefore = secondsOf(r);
            wall.start();
            QTimer::singleShot(static_cast<int>(seconds * 1000), &app, [&] {
                client.send(QStringLiteral("ticks"), [&](const AttachLineClient::Reply& r2) {
                    sample.wall = static_cast<double>(wall.nsecsElapsed()) * 1e-9;
                    sample.ticksAfter = ticksOf(r2);
                    sample.secondsAfter = secondsOf(r2);
                    result = report(sample);
                    std::fflush(stdout);
                    app.quit();
                });
            });
        });
    });
    app.exec();
    return result;
}

} // namespace calcu::qt6
