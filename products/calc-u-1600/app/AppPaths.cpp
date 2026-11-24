#include "app/AppPaths.hpp"

#include <filesystem>
#include <system_error>

#include "files/Files.hpp"

namespace cu1600 {

namespace fs = std::filesystem;

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

std::string AppPaths::displayPath(const std::string& path) const {
    return calcu::settings::withTilde(path, homeDir());
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
