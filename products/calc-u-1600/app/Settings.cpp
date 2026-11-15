#include "app/Settings.hpp"

#include <filesystem>
#include <system_error>

#include "app/Utf8Path.hpp"
#include "settings/OpenFolders.hpp"
#include "settings/Startup.hpp"

namespace shared {

namespace fs = std::filesystem;
namespace cs = calcu::settings;

namespace {

std::string absolutePath(const std::string& path) {
    std::error_code ec;
    fs::path abs = fs::absolute(toPath(path), ec);
    if (ec) return path;
    abs = abs.lexically_normal();
    // No trailing separator ("/a/b/" -> "/a/b"), like QDir::absolutePath().
    if (!abs.has_filename() && abs.has_relative_path()) abs = abs.parent_path();
    return fromPath(abs);
}

cs::ProductDecl makeDecl() {
    cs::ProductDecl d;
    d.productName = "Calc-U-1600";
    d.models = {"PC-1500", "PC-1500A", "PC-1600"};
    d.fallbackModel = "PC-1500A";  // AC-27
    d.keys.push_back(cs::family::startupModel());
    d.keys.push_back(cs::family::startupLastModel());
    for (const std::string& m : d.models) d.keys.push_back(cs::family::presetFor(m));
    d.keys.push_back(cs::family::saveFolder());
    d.keys.push_back(cs::family::tapeFolder());
    d.keys.push_back(cs::family::traceFolder());
    d.keys.push_back(cs::family::traceMaxFileSizeMb());

    cs::KeyDecl dapEnabled;
    dapEnabled.section = "debug";
    dapEnabled.key = "dap-enabled";
    dapEnabled.kind = cs::ValueKind::Boolean;
    dapEnabled.defaultValue = "false";
    dapEnabled.help = "The debug server for VS Code (DAP, on 127.0.0.1 only): true or false.";
    d.keys.push_back(dapEnabled);
    cs::KeyDecl dapPort;
    dapPort.section = "debug";
    dapPort.key = "dap-port";
    dapPort.kind = cs::ValueKind::Integer;
    dapPort.defaultValue = std::to_string(Settings::kDefaultDapPort);
    dapPort.min = 1024;  // AC-28
    dapPort.max = 65535;
    dapPort.help = "The port of the debug server, 1024 to 65535; else 32168.";
    d.keys.push_back(dapPort);

    d.keys.push_back(cs::family::serialLinkFolder());
    for (OpenFolder f : {OpenFolder::Samples, OpenFolder::Basic, OpenFolder::Assembly, OpenFolder::HostDrive})
        d.keys.push_back(cs::family::openFolder(openFolderKind(f)));
    d.keys.push_back(cs::family::windowNumber("width", Settings::kDefaultWindowWidth));
    d.keys.push_back(cs::family::windowNumber("height", Settings::kDefaultWindowHeight));
    return d;
}

} // namespace

const cs::ProductDecl& settingsDecl() {
    static const cs::ProductDecl decl = makeDecl();
    return decl;
}

const char* settingsModelName(Model model) {
    switch (model) {
        case Model::PC1500: return "PC-1500";
        case Model::PC1500A: return "PC-1500A";
        case Model::PC1600: return "PC-1600";
    }
    return "";
}

std::optional<Model> modelFromSettingsName(const std::string& name) {
    for (Model m : {Model::PC1500, Model::PC1500A, Model::PC1600})
        if (name == settingsModelName(m)) return m;
    return std::nullopt;
}

const char* openFolderKind(OpenFolder folder) {
    switch (folder) {
        case OpenFolder::Samples: return "presets";
        case OpenFolder::Basic: return "basic-programs";
        case OpenFolder::Assembly: return "machine-code";
        case OpenFolder::HostDrive: return "host-drive";
    }
    return "presets";
}

std::string Settings::openDir(OpenFolder folder) const { return cs::fixedOpenFolder(m_file, openFolderKind(folder)); }

void Settings::setOpenDir(OpenFolder folder, const std::string& dir) {
    cs::setFixedOpenFolder(m_file, openFolderKind(folder), dir);   // "": back to the folder last used
}

std::string Settings::lastOpenDir(OpenFolder folder) const {
    if (cs::hasFixedOpenFolder(m_file, openFolderKind(folder))) return {};
    return m_file.text("folders", openFolderKind(folder));
}

void Settings::rememberOpenDir(OpenFolder folder, const std::string& dir) {
    cs::rememberOpenFolder(m_file, openFolderKind(folder), absolutePath(dir));
}

void Settings::rememberOpenFile(OpenFolder folder, const std::string& filePath) {
    rememberOpenDir(folder, fromPath(toPath(absolutePath(filePath)).parent_path()));
}

std::string Settings::openStartDir(OpenFolder folder, const std::string& homeDir) const {
    return cs::openStartFolder(m_file, openFolderKind(folder), homeDir);
}

Model Settings::startupModel() const {
    return modelFromSettingsName(cs::startupModel(m_file)).value_or(Model::PC1500A);
}

} // namespace shared
