// main() of `calcu` and `calcu_test`: the
// products of the build come from the generated table products().
#include "Cli.hpp"
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

std::vector<std::unique_ptr<calcu::harness::HarnessProduct>> products();

int main(int argc, char** argv) {
#ifdef _WIN32
    // Text mode would write \n as \r\n; the reader drops \r.
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    std::ios::sync_with_stdio(false);
    std::vector<std::string> args(argv + 1, argv + argc);
    calcu::harness::cli::Io io;
    io.readLine = []() -> std::optional<std::string> {
        std::string line;
        if (!std::getline(std::cin, line)) return std::nullopt;
        return line;
    };
    io.out = [](const std::string& s) {
        std::cout << s;
        std::cout.flush();   // one reply at a time over a pipe (serve)
    };
    io.err = [](const std::string& s) { std::cerr << s; };
    const int status = calcu::harness::cli::run(args, io, products());
    std::cout.flush();
    return status;
}
