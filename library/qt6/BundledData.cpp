#include "qt6/BundledData.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

namespace calcu::qt6 {

BundleLayout platformBundleLayout() {
#if defined(Q_OS_MACOS)
    return BundleLayout::MacApp;
#elif defined(Q_OS_WIN)
    return BundleLayout::BesideProgram;
#else
    return BundleLayout::Unix;
#endif
}

QString bundledDataDir(const QString& programDir, const QString& appFolder, BundleLayout layout) {
    const QDir dir(programDir);
    switch (layout) {
    case BundleLayout::MacApp:
        return QDir::cleanPath(dir.filePath(QStringLiteral("../Resources")));
    case BundleLayout::BesideProgram:
        break;
    case BundleLayout::Unix: {
        // The installed layout first: a build tree has no share/ beside
        // its program's folder.
        const QString shared = QDir::cleanPath(dir.filePath(QStringLiteral("../share/") + appFolder));
        if (QFileInfo(shared).isDir()) return shared;
        break;
    }
    }
    return QDir::cleanPath(dir.filePath(QStringLiteral("resources")));
}

QString bundledDataDir(const QString& appFolder) {
    return bundledDataDir(QCoreApplication::applicationDirPath(), appFolder, platformBundleLayout());
}

} // namespace calcu::qt6
