#pragma once
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include "../app/HarnessProduct.hpp"
#include "TempFolder.hpp"
#include "harness/HarnessHost.hpp"
#include "harness/Run.hpp"
#include "machine/Machine.hpp"

// A preset of format 2 loaded through Calc-U-1600's harness entry (the
// library's loader in emulated time; LIB-6 §10.6), for the core tests that
// set a machine up by a preset (M-46: in place of the version 1 loaders).
// The preset's text goes into a temporary folder with the files a test
// adds; `load()` sends `preset <file>` and returns the reply.

namespace presettest {

namespace fs = std::filesystem;

struct HarnessPreset {
    calcu::testing::TempFolder folder{"calcu1600-preset-test"};
    std::unique_ptr<calcu::harness::HarnessProduct> product = cu1600::makeHarnessProduct();
    std::unique_ptr<calcu::harness::Server> server;
    std::string startError;

    HarnessPreset() {
        calcu::harness::RunOptions o;
        o.outputFolder = folder.str();
        server = calcu::harness::Server::start(*product, o, &startError);
    }
    ~HarnessPreset() {
        if (server) server->finish();
    }
    bool ok() const { return server != nullptr; }

    // A file of the test's folder (a program, a ROM image).
    fs::path write(const std::string& name, const std::string& bytes) {
        const fs::path p = folder.path() / name;
        std::ofstream out(p, std::ios::binary);
        out << bytes;
        return p;
    }
    // The reply of one command (`done …` or `refused: …`).
    std::string line(const std::string& command) { return server ? server->line(command).text : std::string(); }
    // Writes the preset as `name` and loads it; the reply.
    std::string load(const std::string& text, const std::string& name = "t.pc1600") {
        return line("preset " + write(name, text).generic_string());
    }
    // `load()`, then the harness `wait` (until idle); true when both are done.
    bool loadIdle(const std::string& text, const std::string& name = "t.pc1600") {
        const std::string r = load(text, name);
        if (r.rfind("done", 0) != 0) {
            std::fprintf(stderr, "  preset refused: %s", r.c_str());
            return false;
        }
        return line("wait").rfind("done", 0) == 0;
    }
    calcu::machine::Machine* machine() { return server ? server->host().session().context().machine() : nullptr; }
};

} // namespace presettest
