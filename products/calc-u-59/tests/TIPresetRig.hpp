#pragma once
#include "../app/HarnessProduct.hpp"
#include "../app/TIPresetRules.hpp"
#include "../app/TITyping.hpp"
#include "../core/TICatalogue.hpp"
#include "TempFolder.hpp"
#include "harness/HarnessHost.hpp"
#include "harness/Run.hpp"
#include "presets/Reader.hpp"
#include "presets/Source.hpp"
#include "presets/Vocabulary.hpp"
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <variant>

// The rigs of the M-54 tests (C59-2, LIB-6 59, LIB-18 59, C59-3): a preset
// read with TIPresetRules (no machine), and Calc-U-59's harness entry in
// virtual time (`calcu --product 59` in-process, as tests/harness_tests.cpp).

namespace tirig {

inline bool haveRoms() { return std::filesystem::exists("roms/TMC0571B.txt"); }

struct PresetRead {
    calcu::testing::TempFolder folder{"calcu59-preset-read"};
    cu59::TIPresetRules rules;
    calcu::presets::Vocabulary vocabulary;
    PresetRead() {
        calcu::presets::addLibraryKeywords(vocabulary);
        rules.addKeywords(vocabulary);
    }
    calcu::presets::ReadSetup setup() {
        return calcu::presets::ReadSetup{vocabulary, rules, tiCatalogue(), &cu59::typingProduct()};
    }
    std::filesystem::path write(const std::string& name, const std::string& bytes) {
        const std::filesystem::path p = folder.path() / name;
        std::filesystem::create_directories(p.parent_path());
        std::ofstream(p, std::ios::binary) << bytes;
        return p;
    }
    std::variant<calcu::presets::LoadPlan, calcu::presets::ReadError> read(const std::string& text,
                                                                          const std::string& name = "t.ti59") {
        const auto source = calcu::presets::folderSource(write(name, text), {});
        return calcu::presets::readPreset(*source, setup());
    }
    /// The first error's message, or empty when the preset reads.
    std::string error(const std::string& text, const std::string& name = "t.ti59") {
        const auto r = read(text, name);
        if (const auto* e = std::get_if<calcu::presets::ReadError>(&r)) return e->message;
        return {};
    }
};

/// Calc-U-59's harness entry, in-process, in virtual time.
struct Calcu {
    std::unique_ptr<calcu::harness::HarnessProduct> product = cu59::makeHarnessProduct();
    std::unique_ptr<calcu::harness::Server> server;
    calcu::testing::TempFolder folder{"calcu59-harness"};
    explicit Calcu(calcu::harness::RunOptions o = {}) {
        if (!o.outputFolder) o.outputFolder = folder.path().string();
        std::string error;
        server = calcu::harness::Server::start(*product, o, &error);
    }
    ~Calcu() {
        if (server) server->finish();
    }
    explicit operator bool() const { return server != nullptr; }
    std::string operator()(const std::string& line) { return server->line(line).text; }
    calcu::machine::Machine* machine() { return server->host().session().context().machine(); }
    /// A preset written into the rig's folder and loaded with `preset`.
    std::string preset(const std::string& text, const std::string& name = "t.ti59") {
        const std::filesystem::path p = folder.path() / name;
        std::ofstream(p, std::ios::binary) << text;
        return (*this)("preset " + p.string());
    }
};

} // namespace tirig
