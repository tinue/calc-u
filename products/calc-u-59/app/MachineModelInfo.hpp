#pragma once
#include "machine/Ids.hpp"
#include <cstddef>
#include <cstdint>
#include <iterator>

// Per-model facts the frontends need above the machine, keyed by the
// model's ID in the catalogue (tiCatalogue()), and the RAM layout of the
// presets. The tick rate, key readiness and the ROM address ranges come
// from the machine (calcu::machine::Machine); the ROM files from the
// descriptor's ROM entries through the ROM lookup (app/ProductRoms).

namespace cu59 {

using calcu::machine::ModelId;

/// The three models' IDs.
inline constexpr const char* kTI59 = "TI-59";
inline constexpr const char* kTI58 = "TI-58";
inline constexpr const char* kTI58C = "TI-58C";

struct ModelInfo {
    const char* id;           ///< ModelId::value
    const char* displayName;

    bool hasConstantMemory;  ///< TI-58C: RAM survives power-off (ti58c.mem).
    bool hasLargeMemory;     ///< TI-59: 120 registers.
    bool hasCardReader;      ///< TI-59 only.

    /// TI-58C only: extra constant-memory registers E000…E003 (H00…H03 in state
    /// files) starting at raw RAM index 60 — see docs/developer/Core-Architecture.md
    /// § "TI-58C Extra (Constant Memory) Registers".
    int extraRegisterCount;

    /// The model's code in a trace file (v2) and the trace's file name.
    std::uint8_t traceCode;
    const char* traceFileName;

    /// Highest program step the RAM can hold (all registers partitioned to program).
    int maxStep() const { return hasLargeMemory ? 959 : 479; }
};

inline constexpr int kExtraRegisterBase = 60;

/// The models, in the order of the catalogue and the Model menu.
inline constexpr ModelInfo kModelFacts[] = {
    {kTI59, "TI-59", false, true, true, 0, 0, "CALCU59_TRACE.bin"},
    {kTI58, "TI-58", false, false, false, 0, 1, "CALCU58_TRACE.bin"},
    {kTI58C, "TI-58C", true, false, false, 4, 2, "CALCU58C_TRACE.bin"},
};

/// The model's facts; nullptr for an ID outside the catalogue.
inline const ModelInfo* findModel(const ModelId& id) {
    for (const ModelInfo& m : kModelFacts)
        if (id.value == m.id) return &m;
    return nullptr;
}

/// The model's facts; the TI-59's for an ID outside the catalogue (a
/// machine of this product always has one of the three).
inline const ModelInfo& info(const ModelId& id) {
    const ModelInfo* m = findModel(id);
    return m ? *m : kModelFacts[0];
}

/// The model's place in kModelFacts (tables per model).
inline std::size_t modelIndex(const ModelId& id) { return static_cast<std::size_t>(&info(id) - std::begin(kModelFacts)); }

} // namespace cu59
