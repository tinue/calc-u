#include "app/AppPaths.hpp"

#include <filesystem>
#include <fstream>
#include <system_error>

#include "app/Utf8Path.hpp"

#include "Connector/FloppyImageFile.hpp"
#include "Connector/MemoryCardCatalog.hpp"
#include "Connector/NamedFileCatalog.hpp"

namespace shared {

namespace fs = std::filesystem;

namespace {

void makeDirs(const std::string& dir) {
    std::error_code ec;
    fs::create_directories(toPath(dir), ec);
}

// `path` with the leading `dir` replaced by `shown`, or empty when `path`
// isn't `dir` itself or inside it.
std::string replacePrefix(const std::string& path, const std::string& dir, const std::string& shown) {
    if (dir.empty() || path.compare(0, dir.size(), dir) != 0) return {};
    if (path.size() > dir.size() && path[dir.size()] != '/') return {};
    return shown + path.substr(dir.size());
}

} // namespace

std::string mediaSaveRefusalMessage(MediaSaveRefusal refusal, const std::string& name, const std::string& path) {
    switch (refusal) {
        case MediaSaveRefusal::None: return {};
        case MediaSaveRefusal::EmptyName: return "Name cannot be empty.";
        case MediaSaveRefusal::QuoteInName: return "Name cannot contain '\"'.";
        case MediaSaveRefusal::NoSaveFolder: return "There is no save folder.";
        case MediaSaveRefusal::TemplateName: return "\"" + name + "\" is a template's name. Choose a different name.";
        case MediaSaveRefusal::TemplateFile:
            return "\"" + path + "\" is a template file and is never overwritten. Choose a different name.";
    }
    return {};
}

std::string joinPath(const std::string& dir, const std::string& name) {
    return fromPath(toPath(dir) / toPath(name));
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

std::string AppPaths::instancePathFor(const std::string& instanceName) const {
    return joinPath(instanceDir(), namedFileName(instanceName, kCardFileSuffix));
}

std::string AppPaths::floppyInstancePathFor(const std::string& diskName) const {
    return joinPath(instanceDir(), namedFileName(diskName, kFloppyFileSuffix));
}

std::string AppPaths::tapeDir() const {
    const std::string dir = m_paths.tapeFolder();
    makeDirs(dir);
    return dir;
}

std::string AppPaths::tapePathFor(const std::string& tapeName) const {
    return joinPath(tapeDir(), namedFileName(tapeName, ".wav"));
}

std::string AppPaths::traceDir() const {
    // The Web: no trace folder (a download, LIB-10); the save folder until then (A-3).
    if (const auto dir = m_paths.traceFolder()) return *dir;
    return instanceDir();
}

std::string AppPaths::serialLinkDir() const {
    if (const auto dir = m_paths.serialLinkFolder()) return *dir;
    return instanceDir();
}

MediaSaveRefusal AppPaths::planNamedSave(MediaKind kind, const std::string& name, const std::string& explicitPath,
                                         bool asTemplate, std::string* path) const {
    return planMediaSave(kind, name, explicitPath, asTemplate, {m_bundle.bundledResources, instanceDir()}, path);
}

bool isUnderDir(const std::string& path, const std::string& dir) {
    if (path.empty() || dir.empty()) return false;
    std::error_code ec1, ec2;
    const fs::path a = fs::canonical(toPath(path), ec1);
    const fs::path b = fs::canonical(toPath(dir), ec2);
    if (ec1 || ec2) return false;
    auto ai = a.begin();
    for (auto bi = b.begin(); bi != b.end(); ++bi, ++ai) {
        if (ai == a.end() || *ai != *bi) return false;
    }
    return true;
}

bool atomicWriteFile(const std::string& path, const std::string& text) {
    const fs::path target = toPath(path);
    fs::path temp = target;
    temp += ".tmp-save";
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        out.write(text.data(), static_cast<std::streamsize>(text.size()));
        out.flush();
        if (!out) {
            out.close();
            std::error_code ec;
            fs::remove(temp, ec);
            return false;
        }
    }
    std::error_code ec;
    fs::rename(temp, target, ec);  // replaces an existing file (MoveFileEx on Windows)
    if (ec) {
        fs::remove(temp, ec);
        return false;
    }
    return true;
}

} // namespace shared
