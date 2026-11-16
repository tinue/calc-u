#include "PacingProbe.hpp"
#include "AttachLineClient.hpp"
#include "../MainWindow.hpp"

#include <QApplication>
#include <QElapsedTimer>
#include <QTimer>
#include <cmath>
#include <cstdio>

namespace {

double secondsOf(const AttachLineClient::Reply& r) {
    for (const QString& l : r.lines)
        if (l.startsWith(QLatin1String("seconds: "))) return l.mid(9).toDouble();
    return -1;
}

} // namespace

int runPacingProbe(QApplication& app, MainWindow& window, std::uint16_t attachPort, double seconds) {
    AttachLineClient client;
    if (!client.connectTo(attachPort)) {
        std::printf("pacing-probe: attach is not open on port %u\n", attachPort);
        return 1;
    }
    int result = 1;
    double t0 = -1;
    QElapsedTimer wall;
    client.send(QStringLiteral("wait"), [&](const AttachLineClient::Reply&) {
        client.send(QStringLiteral("ticks"), [&](const AttachLineClient::Reply& r) {
            t0 = secondsOf(r);
            wall.start();
            QTimer::singleShot(static_cast<int>(seconds * 1000), &app, [&] {
                client.send(QStringLiteral("ticks"), [&](const AttachLineClient::Reply& r2) {
                    const double w = static_cast<double>(wall.nsecsElapsed()) * 1e-9;
                    const double e = secondsOf(r2) - t0;
                    result = t0 >= 0 && std::fabs(e - w) <= 0.025 ? 0 : 1;
                    MachineController* c = window.controller();
                    std::printf("pacing-probe: wall %.3f s, emulated %.3f s, difference %+.4f s, %llu host calls, "
                                "longest gap %.1f ms: %s\n",
                                w, e, e - w, static_cast<unsigned long long>(c->calls()), c->longestGapMs(),
                                result == 0 ? "ok" : "beyond the bound");
                    app.quit();
                });
            });
        });
    });
    app.exec();
    return result;
}
