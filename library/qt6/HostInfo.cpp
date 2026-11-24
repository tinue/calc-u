#include "qt6/HostInfo.hpp"

#include <QFile>
#include <QGuiApplication>
#include <QScreen>
#include <QSettings>
#include <QSysInfo>
#include <QWidget>
#include <QtGlobal>
#include <string>

#ifdef Q_OS_MACOS
#include "qt6/MacDisplay.h"
#include <sys/sysctl.h>
#endif

namespace calcu::qt6 {

namespace {

std::string os() {
    const std::string arch = QSysInfo::currentCpuArchitecture().toStdString();
#if defined(Q_OS_MACOS)
    return "Darwin " + arch + " " + QSysInfo::kernelVersion().toStdString();
#elif defined(Q_OS_WIN)
    // ProductName says "Windows 10" on Windows 11 too; Qt's productVersion
    // knows the difference.
    const QSettings nt(QStringLiteral(R"(HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion)"),
                       QSettings::NativeFormat);
    std::string out = "Windows " + QSysInfo::productVersion().toStdString();
    const QString display = nt.value(QStringLiteral("DisplayVersion")).toString();
    if (!display.isEmpty()) out += " " + display.toStdString();
    const QString build = nt.value(QStringLiteral("CurrentBuild")).toString();
    if (!build.isEmpty()) {
        out += " (build " + build.toStdString();
        if (const QVariant ubr = nt.value(QStringLiteral("UBR")); ubr.isValid()) out += "." + ubr.toString().toStdString();
        out += ")";
    }
    return out;
#else
    return QSysInfo::prettyProductName().toStdString() + " (" + QSysInfo::kernelType().toStdString() + " " +
           QSysInfo::kernelVersion().toStdString() + ")";
#endif
}

#if defined(Q_OS_LINUX)
// /proc/cpuinfo's "model name" (x86, some ARM kernels), else the board's
// name from the device tree (a Raspberry Pi).
std::string linuxCpu() {
    QFile info(QStringLiteral("/proc/cpuinfo"));
    if (info.open(QIODevice::ReadOnly | QIODevice::Text))
        while (!info.atEnd()) {
            const QString line = QString::fromUtf8(info.readLine());
            if (line.startsWith(QStringLiteral("model name"))) return line.section(QLatin1Char(':'), 1).trimmed().toStdString();
        }
    QFile board(QStringLiteral("/sys/firmware/devicetree/base/model"));
    if (board.open(QIODevice::ReadOnly)) return QString::fromUtf8(board.readAll()).remove(QChar(0)).trimmed().toStdString();
    return {};
}
#endif

std::string hardware() {
#if defined(Q_OS_MACOS)
    char model[256] = {};
    std::size_t size = sizeof model;
    return sysctlbyname("hw.model", model, &size, nullptr, 0) == 0 ? std::string(model) : std::string();
#else
#if defined(Q_OS_WIN)
    const QSettings cpu(QStringLiteral(R"(HKEY_LOCAL_MACHINE\HARDWARE\DESCRIPTION\System\CentralProcessor\0)"),
                        QSettings::NativeFormat);
    const std::string name = cpu.value(QStringLiteral("ProcessorNameString")).toString().trimmed().toStdString();
#elif defined(Q_OS_LINUX)
    const std::string name = linuxCpu();
#else
    const std::string name;
#endif
    const std::string arch = QSysInfo::currentCpuArchitecture().toStdString();
    return name.empty() ? arch : name + ", " + arch;
#endif
}

} // namespace

shell::SupportRows hostSupportRows() { return {{"OS", os()}, {"Hardware", hardware()}}; }

shell::DisplayFacts displayFacts(const QWidget& window) {
    shell::DisplayFacts d;
#if defined(Q_OS_LINUX)
    const QString platform = QGuiApplication::platformName();
    if (platform == QStringLiteral("wayland")) d.windowing = "Wayland";
    else if (platform == QStringLiteral("xcb")) d.windowing = qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY") ? "X11" : "X11 on Wayland";
    else d.windowing = platform.toStdString();
#endif
    d.window = {window.width(), window.height()};
    const QScreen* screen = window.screen();
    if (!screen) return d;
    d.screen = screen->name().toStdString();
    d.reported = {screen->geometry().width(), screen->geometry().height()};
    d.scale = screen->devicePixelRatio();
#if defined(Q_OS_MACOS)
    int w = 0, h = 0;
    if (macPanelSize(*screen, &w, &h)) d.panel = {w, h};
#endif
    return d;
}

} // namespace calcu::qt6
