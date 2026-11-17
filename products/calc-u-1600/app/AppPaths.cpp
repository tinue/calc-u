#include "app/AppPaths.hpp"

#include <filesystem>
#include <system_error>

#include "files/Files.hpp"

namespace cu1600 {

namespace fs = std::filesystem;

namespace {

void makeDirs(const std::string& dir) {
    std::error_code ec;
    fs::create_directories(calcu::files::toPath(dir), ec);
}

// `path` with the leading `dir` replaced by `shown`, or empty when `path`
// isn't `dir` itself or inside it.
std::string replacePrefix(const std::string& path, const std::string& dir, const std::string& shown) {
    if (dir.empty() || path.compare(0, dir.size(), dir) != 0) return {};
    if (path.size() > dir.size() && path[dir.size()] != '/') return {};
    return shown + path.substr(dir.size());
}

} // namespace


std::string joinPath(const std::string& dir, const std::string& name) {
    return calcu::files::fromPath(calcu::files::toPath(dir) / calcu::files::toPath(name));
}

std::string AppPaths::bundledExamplesDir() const {
    return m_bundle.bundledExamples.empty() ? joinPath(m_bundle.bundledResources, "examples") : m_bundle.bundledExamples;
}

std::string AppPaths::resolvePresetRef(const std::string& ref) const {
    const std::string prefix = kExamplePresetPrefix;
    if (ref.compare(0, prefix.size(), prefix) != 0) return ref;
    return joinPath(bundledExamplesDir(), ref.substr(prefix.size()));
}

std::string AppPaths::defaultInstanceDir() const {
    const calcu::settings::HostDirs& host = m_paths.hostDirs();
    const std::string dir = host.frontend == calcu::settings::FrontendKind::Qt6
                                ? joinPath(host.home, settingsDecl().productName)
                                : host.saveFolder;
    makeDirs(dir);
    return dir;
}

std::string AppPaths::instanceDir() const {
    const std::string dir = m_paths.saveFolder();
    makeDirs(dir);
    return dir;
}

std::string AppPaths::displayPath(const std::string& path) const {
    std::string shown = replacePrefix(path, defaultInstanceDir(), "~/Calc-U-1600");
    if (shown.empty()) shown = replacePrefix(path, homeDir(), "~");
    return shown.empty() ? path : shown;
}



std::string AppPaths::cassetteDir() const {
    const std::string dir = m_paths.cassetteFolder();
    makeDirs(dir);
    return dir;
}


std::string AppPaths::traceDir() const {
    // The Web has no trace folder (LIB-9 AC-17; LIB-10 D-10: a trace fails there).
    if (const auto dir = m_paths.traceFolder()) return *dir;
    return {};
}

std::string AppPaths::serialLinkDir() const {
    if (const auto dir = m_paths.serialLinkFolder()) return *dir;
    return instanceDir();
}


bool isUnderDir(const std::string& path, const std::string& dir) {
    if (path.empty() || dir.empty()) return false;
    std::error_code ec1, ec2;
    const fs::path a = fs::canonical(calcu::files::toPath(path), ec1);
    const fs::path b = fs::canonical(calcu::files::toPath(dir), ec2);
    if (ec1 || ec2) return false;
    auto ai = a.begin();
    for (auto bi = b.begin(); bi != b.end(); ++bi, ++ai) {
        if (ai == a.end() || *ai != *bi) return false;
    }
    return true;
}


} // namespace cu1600
