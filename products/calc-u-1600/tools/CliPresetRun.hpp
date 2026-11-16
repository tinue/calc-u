#pragma once
#include <cstdio>
#include <memory>
#include <string>

#include "../app/HarnessProduct.hpp"
#include "harness/HarnessHost.hpp"
#include "harness/Run.hpp"
#include "machine/Machine.hpp"

// ── CliPresetRun: the CLIs' presets on the library's loader ──────────────
//
// Adapter CliPresetRun (LIB-6 design §10.5, D-35; removed in M-51 with the
// CLIs): `pc1500_cli`, `pc1600_cli` and `pc1600_plotter_probe` load a
// preset as the product does, through Calc-U-1600's harness entry -- the
// library's app session in emulated time, the product's preset rules, LIB-18's
// build -- and wait until the machine is idle (the harness `wait`, LIB-6
// AC-34). The machine stays the session's; the tool reads and runs it for
// its own dumps afterwards. A `- trace:` or `- screenshot:` step writes into
// the working directory (the run's output folder), as the CLIs did. Run from
// products/calc-u-1600 (the ROMs in roms/).
namespace cli {

class CliPresetRun {
public:
    // nullptr with `error`: the harness did not start, or the preset was
    // refused or failed (its reason).
    static std::unique_ptr<CliPresetRun> load(const std::string& presetPath, std::string* error) {
        auto run = std::unique_ptr<CliPresetRun>(new CliPresetRun);
        run->product_ = shared::makeHarnessProduct();
        calcu::harness::RunOptions options;
        options.outputFolder = ".";
        run->server_ = calcu::harness::Server::start(*run->product_, options, error);
        if (!run->server_) return nullptr;
        for (const std::string& line : {"preset " + presetPath, std::string("wait")}) {
            const calcu::harness::Server::Result r = run->server_->line(line);
            if (r.text.rfind("refused: ", 0) == 0) {
                *error = r.text.substr(9);
                while (!error->empty() && error->back() == '\n') error->pop_back();
                return nullptr;
            }
        }
        return run;
    }
    ~CliPresetRun() {
        if (server_) server_->finish();
    }
    CliPresetRun(const CliPresetRun&) = delete;
    CliPresetRun& operator=(const CliPresetRun&) = delete;

    calcu::machine::Machine* machine() { return server_->host().session().context().machine(); }

private:
    CliPresetRun() = default;
    std::unique_ptr<calcu::harness::HarnessProduct> product_;
    std::unique_ptr<calcu::harness::Server> server_;
};

} // namespace cli
