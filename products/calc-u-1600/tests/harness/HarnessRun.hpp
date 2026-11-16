#pragma once
// One `calcu --product 1600` run for the tests of tests/harness/ (C1600-5
// design §6; LIB-11 Part D, D.6): the product's harness entry and a server
// started on it, the replies as `calcu serve` writes them.
#include <filesystem>
#include <memory>
#include <string>

#include "../../app/HarnessProduct.hpp"
#include "TempFolder.hpp"
#include "TestHarness.hpp"
#include "harness/Run.hpp"
#include "sessions/AppSession.hpp"

namespace harnesstest {

namespace fs = std::filesystem;

inline bool romsPresent() {
    return fs::exists("roms/PC-1500_A04.ROM") && fs::exists("roms/PC1600-P0-B0-new.bin") &&
           fs::exists("roms/PC1600-P0-B0-old.bin");
}

struct Run {
    std::unique_ptr<calcu::harness::HarnessProduct> product = shared::makeHarnessProduct();
    std::unique_ptr<calcu::harness::Server> server;
    std::string error;
    explicit Run(calcu::harness::RunOptions o = {}) {
        server = calcu::harness::Server::start(*product, o, &error);
        if (!server) std::fprintf(stderr, "calcu did not start: %s\n", error.c_str());
    }
    ~Run() {
        if (server) server->finish();
    }
    bool ok() const { return server != nullptr; }
    // The reply to one line, as written.
    std::string line(const std::string& l) { return server->line(l).text; }
    calcu::machine::Machine* machine() { return server->host().session().context().machine(); }
    std::string saveFolder() { return server->setup().saveFolder(); }
};

inline bool starts(const std::string& s, const std::string& prefix) { return s.rfind(prefix, 0) == 0; }
inline bool has(const std::string& s, const std::string& part) { return s.find(part) != std::string::npos; }

} // namespace harnesstest
