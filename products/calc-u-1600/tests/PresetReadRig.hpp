#pragma once
#include <filesystem>
#include <fstream>
#include <string>
#include <variant>

#include "../app/SharpPresetRules.hpp"
#include "../app/SharpTyping.hpp"
#include "../core/Machine/SharpCatalogue.hpp"
#include "TempFolder.hpp"
#include "debugger/DebuggerPart.hpp"
#include "paper/PaperPart.hpp"
#include "presets/Reader.hpp"
#include "presets/Source.hpp"
#include "presets/Vocabulary.hpp"

// Reading a preset with Calc-U-1600's rules and the library's reader, as
// the product's PresetsPart does (LIB-6 §10.6, C1600-4 §6), without a
// machine: for the criteria checked when a preset is read. The preset's
// text and the files it names go into a temporary folder.

namespace presettest {

struct PresetReadRig {
    calcu::testing::TempFolder folder{"calcu1600-preset-read"};
    calcu::machine::Catalogue catalogue = sharp::sharpCatalogue();
    shared::SharpPresetRules rules;
    calcu::presets::Vocabulary vocabulary;

    explicit PresetReadRig(shared::SharpKeywordSetup setup = {}) : rules(std::move(setup)) {
        sharp::addModuleDefinitions(catalogue, "frontends/qt6/resources/cards");
        calcu::presets::addLibraryKeywords(vocabulary);
        rules.addKeywords(vocabulary);
    }
    calcu::presets::ReadSetup setup() {
        return calcu::presets::ReadSetup{vocabulary, rules, catalogue, &shared::typingProduct()};
    }
    std::filesystem::path write(const std::string& name, const std::string& bytes) {
        const std::filesystem::path p = folder.path() / name;
        std::filesystem::create_directories(p.parent_path());
        std::ofstream out(p, std::ios::binary);
        out << bytes;
        return p;
    }
    // The plan, or the error's message.
    std::variant<calcu::presets::LoadPlan, calcu::presets::ReadError> read(const std::string& text,
                                                                            const std::string& name = "t.pc1600") {
        const auto source = calcu::presets::folderSource(write(name, text), {});
        return calcu::presets::readPreset(*source, setup());
    }
    // Without its folder (LIB-6 EC-5).
    std::variant<calcu::presets::LoadPlan, calcu::presets::ReadError> readContent(const std::string& text) {
        const auto source = calcu::presets::contentSource(text, "dropped.pc1600", {});
        return calcu::presets::readPreset(*source, setup());
    }
    bool ok(const std::string& text, const std::string& name = "t.pc1600") {
        const auto r = read(text, name);
        if (const auto* e = std::get_if<calcu::presets::ReadError>(&r)) std::fprintf(stderr, "  read: %s\n", e->message.c_str());
        return std::holds_alternative<calcu::presets::LoadPlan>(r);
    }
    // The error's message ("" when the preset is read).
    std::string error(const std::string& text, const std::string& name = "t.pc1600") {
        const auto r = read(text, name);
        const auto* e = std::get_if<calcu::presets::ReadError>(&r);
        return e ? e->message : std::string();
    }
};

} // namespace presettest
