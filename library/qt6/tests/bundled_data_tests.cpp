// The bundled files' folder of a Qt6 app (library/qt6/BundledData) in each
// layout, on folders made for the test: the Linux and Windows rules run on
// every host.
#include "TestHarness.hpp"
#include "qt6/BundledData.hpp"
#include <QDir>
#include <QTemporaryDir>
#include <string>

namespace {

using calcu::qt6::BundleLayout;
using calcu::qt6::bundledDataDir;

std::string s(const QString& q) { return q.toStdString(); }

} // namespace

TEST(qt_bundled_data_in_a_mac_app_is_contents_resources) {
    CHECK_EQ(s(bundledDataDir(QStringLiteral("/Apps/Some-App.app/Contents/MacOS"), QStringLiteral("some-app"),
                              BundleLayout::MacApp)),
             std::string("/Apps/Some-App.app/Contents/Resources"));
}

TEST(qt_bundled_data_on_windows_is_resources_beside_the_program) {
    QTemporaryDir root;
    QDir(root.path()).mkpath(QStringLiteral("share/some-app"));   // not looked at on Windows
    const QString programDir = root.filePath(QStringLiteral("Some-App"));
    CHECK_EQ(s(bundledDataDir(programDir, QStringLiteral("some-app"), BundleLayout::BesideProgram)),
             s(programDir + QStringLiteral("/resources")));
}

TEST(qt_bundled_data_installed_on_linux_is_share_app) {
    QTemporaryDir root;
    QDir(root.path()).mkpath(QStringLiteral("bin/resources"));   // the installed folder comes first
    QDir(root.path()).mkpath(QStringLiteral("share/other-app"));
    CHECK_EQ(s(bundledDataDir(root.filePath(QStringLiteral("bin")), QStringLiteral("other-app"), BundleLayout::Unix)),
             s(QDir::cleanPath(root.filePath(QStringLiteral("share/other-app")))));
}

TEST(qt_bundled_data_in_a_linux_build_tree_is_resources) {
    QTemporaryDir root;
    QDir(root.path()).mkpath(QStringLiteral("qt6/resources"));
    QDir(root.path()).mkpath(QStringLiteral("share/other-app"));   // another app's folder is not taken
    const QString programDir = root.filePath(QStringLiteral("qt6"));
    CHECK_EQ(s(bundledDataDir(programDir, QStringLiteral("some-app"), BundleLayout::Unix)),
             s(QDir::cleanPath(programDir + QStringLiteral("/resources"))));
}
