#pragma once
#include "harness/Product.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// The program `calcu`: options, the product, the
// one-shot form or `serve`, and the exit status.
//
//   calcu [options] <command> [then <command> ...]
//   calcu [options] serve
//
// Exit status: 0 when every command was done; 1 at the first refused
// command, after its reply (the commands after it are not carried out);
// 2 when the run cannot start (usage, missing ROMs, no or unknown product), the message on standard
// error. `serve` gives 0 at `quit` or at the end of its input.

namespace calcu::harness::cli {

struct Io {
    std::function<std::optional<std::string>()> readLine;   // standard input, one line without its \n
    std::function<void(const std::string&)> out;             // standard output
    std::function<void(const std::string&)> err;             // standard error
};

int run(const std::vector<std::string>& args, const Io&, std::vector<std::unique_ptr<HarnessProduct>> products);

} // namespace calcu::harness::cli
