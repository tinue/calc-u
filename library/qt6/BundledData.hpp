#pragma once
#include <QString>

// Where a Qt6 app finds the files it carries (its examples, module
// definitions, ROM images, media templates): the one rule both products'
// AppPaths use. The places are the ones calcu_qt6_app gives them
// (library/qt6/CMakeLists.txt), in the build tree and in an install.

namespace calcu::qt6 {

// How the program and its files lie.
enum class BundleLayout {
    MacApp,         // a .app: Contents/MacOS/<program>, the files in Contents/Resources
    BesideProgram,  // Windows: resources\ beside the program, built or installed
    Unix,           // Linux: <prefix>/bin/<program> with <prefix>/share/<app>/
                    // when installed (an AppImage too), else resources/ beside it
};

// The layout of this platform.
BundleLayout platformBundleLayout();

// The folder of the bundled files for a program in `programDir`, with
// `appFolder` the app's folder under share/ ("calc-u-59"). Unix takes
// <programDir>/../share/<appFolder> when it is a folder, else
// <programDir>/resources. Read only; a missing folder is returned all
// the same (the lookups then find nothing in it).
QString bundledDataDir(const QString& programDir, const QString& appFolder, BundleLayout layout);

// The same for this run: the running program's folder and this platform.
QString bundledDataDir(const QString& appFolder);

} // namespace calcu::qt6
