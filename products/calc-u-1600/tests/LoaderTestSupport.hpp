#pragma once
// The loaders (core/Loaders.hpp) on a core machine, for the
// core's tests that build a PC1500Machine / PC1600Machine themselves: the
// loader memory access over the core (core/Machine/SharpLoaderMemory) and
// sharp::loaders on it, with the old result types.

#include <fstream>
#include <iterator>
#include <string>
#include <variant>
#include <vector>

#include "../core/Basic/BasicLoadResults.hpp"
#include "../core/Loaders.hpp"
#include "../core/Machine/SharpLoaderMemory.hpp"
#include "../core/PC1600/PC1600BasicLoader.hpp"

namespace loadertest {

inline std::vector<std::uint8_t> fileBytes(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

template <class Core> struct MemoryOf;
template <> struct MemoryOf<PC1500Machine> { using type = sharp::PC1500LoaderMemory; };
template <> struct MemoryOf<PC1600Machine> { using type = sharp::PC1600LoaderMemory; };

template <class Core> BasicLoadResult loadBasic(Core& m, const std::vector<std::uint8_t>& file) {
    typename MemoryOf<Core>::type mem(m);
    const sharp::loaders::BasicLoaded r = sharp::loaders::loadBasic(mem, file);
    BasicLoadResult out;
    out.ok = r.ok;
    out.baseAddr = r.start;
    out.endAddr = r.end;
    out.error = r.refusal;
    return out;
}

// The bare tokenized payload into the selected program area (PC-1600).
inline BasicLoadResult loadPayload(PC1600Machine& m, const std::vector<std::uint8_t>& payload) {
    sharp::PC1600LoaderMemory mem(m);
    return pc1600LoadBasicPayload(mem, payload);
}

template <class Core> BasicLoadResult loadBasicFile(Core& m, const std::string& path) {
    return loadBasic(m, fileBytes(path));
}

template <class Core>
std::variant<sharp::loaders::CodeLoaded, std::string> loadCode(Core& m, const std::vector<std::uint8_t>& file,
                                                               const sharp::loaders::CodeRequest& r = {}) {
    typename MemoryOf<Core>::type mem(m);
    return sharp::loaders::loadCode(mem, file, r);
}

template <class Core>
sharp::loaders::CodePlan planCode(Core& m, const std::vector<std::uint8_t>& file, const sharp::loaders::CodeRequest& r = {}) {
    typename MemoryOf<Core>::type mem(m);
    return sharp::loaders::planCode(mem, file, r);
}

} // namespace loadertest
