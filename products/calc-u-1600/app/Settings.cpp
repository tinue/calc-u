#include "app/Settings.hpp"

#include "settings/Startup.hpp"

namespace cu1600 {

namespace cs = calcu::settings;

namespace {

cs::ProductDecl makeDecl() {
    cs::ProductDecl d;
    d.productName = "Calc-U-1600";
    d.models = {"PC-1500", "PC-1500A", "PC-1600"};
    d.fallbackModel = "PC-1500A";
    d.keys.push_back(cs::family::startupModel());
    d.keys.push_back(cs::family::startupLastModel());
    for (const std::string& m : d.models) d.keys.push_back(cs::family::presetFor(m));
    d.keys.push_back(cs::family::saveFolder());
    d.keys.push_back(cs::family::cassetteFolder());
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
    dapPort.min = 1024;
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

const std::vector<calcu::machine::ModelId>& sharpModels() {
    static const std::vector<calcu::machine::ModelId> models{calcu::machine::ModelId{"PC-1500"},
                                                             calcu::machine::ModelId{"PC-1500A"},
                                                             calcu::machine::ModelId{"PC-1600"}};
    return models;
}

std::optional<calcu::machine::ModelId> sharpModel(const std::string& name) {
    for (const auto& m : sharpModels())
        if (name == m.value) return m;
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

calcu::machine::ModelId Settings::startupModel() const {
    return sharpModel(cs::startupModel(m_file)).value_or(calcu::machine::ModelId{"PC-1500A"});
}

} // namespace cu1600
