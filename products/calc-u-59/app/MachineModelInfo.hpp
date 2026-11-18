#pragma once
#include "machine/Ids.hpp"
#include <cstdint>
#include <optional>

// Per-model facts the frontends need above the machine: the app's model
// enum and its ModelId and the RAM layout of the presets. Shared by the
// Swift app (via Bridge/) and the Qt6 app; see reference/AppLayer.md.
// The tick rate, key readiness and the ROM address ranges come from the
// machine (calcu::machine::Machine); the ROM files from the
// descriptor's ROM entries through the ROM lookup (app/ProductRoms).

namespace cu59 {

/// Raw values match MachineModel (Swift) and the TI59MachineWrapper model code.
enum class Model : int { TI59 = 0, TI58 = 1, TI58C = 2 };

/// The model's ID in the catalogue (tiCatalogue()).
inline calcu::machine::ModelId modelId(Model m) {
    switch (m) {
    case Model::TI58:  return {"TI-58"};
    case Model::TI58C: return {"TI-58C"};
    case Model::TI59:  break;
    }
    return {"TI-59"};
}

inline std::optional<Model> modelFrom(const calcu::machine::ModelId& id) {
    if (id.value == "TI-59") return Model::TI59;
    if (id.value == "TI-58") return Model::TI58;
    if (id.value == "TI-58C") return Model::TI58C;
    return std::nullopt;
}

struct ModelInfo {
    const char* displayName;

    bool hasConstantMemory;  ///< TI-58C: RAM survives power-off (ti58c.mem).
    bool hasLargeMemory;     ///< TI-59: 120 registers.
    bool hasCardReader;      ///< TI-59 only.

    /// TI-58C only: extra constant-memory registers E000…E003 (H00…H03 in state
    /// files) starting at raw RAM index 60 — see reference/CoreArchitecture.md
    /// § "TI-58C Extra (Constant Memory) Registers".
    int extraRegisterCount;

    /// Highest program step the RAM can hold (all registers partitioned to program).
    int maxStep() const { return hasLargeMemory ? 959 : 479; }
};

inline constexpr int kExtraRegisterBase = 60;

inline const ModelInfo& info(Model m) {
    static const ModelInfo kTI59{"TI-59", false, true, true, 0};
    static const ModelInfo kTI58{"TI-58", false, false, false, 0};
    static const ModelInfo kTI58C{"TI-58C", true, false, false, 4};
    switch (m) {
    case Model::TI59:  return kTI59;
    case Model::TI58:  return kTI58;
    case Model::TI58C: return kTI58C;
    }
    return kTI59;
}

} // namespace cu59
