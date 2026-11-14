#pragma once
#include "machine/Ids.hpp"
#include <cstdint>
#include <optional>

// Per-model facts the frontends need above the machine: the app's model
// enum and its ModelId, the RAM layout of the state files and the ROM file
// names. Shared by the Swift app (via Bridge/) and the Qt6 app; see
// reference/SharedArchitecture.md. The tick rate, key readiness and the ROM
// address ranges come from the machine (calcu::machine::Machine, C59-1); the
// ROM file names here are LIB-1's adapter until M-24 (the lookup reads the
// descriptor's romsFor()).

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

    const char* romFiles[3];    ///< Concatenated in order into the 6144-word image.
    const char* constFiles[2];  ///< CONST-K files, 32 rows each.

    /// Highest program step the RAM can hold (all registers partitioned to program).
    int maxStep() const { return hasLargeMemory ? 959 : 479; }
};

inline constexpr int kExtraRegisterBase = 60;

inline const ModelInfo& info(Model m) {
    static const ModelInfo kTI59 {
        "TI-59", false, true, true, 0,
        {"TMC0582.txt", "TMC0583.txt", "TMC0571B.txt"},
        {"TMC0582-CONST-K.txt", "TMC0583-CONST-K.txt"},
    };
    static const ModelInfo kTI58 {
        "TI-58", false, false, false, 0,
        {"TMC0582.txt", "TMC0583.txt", "TMC0571B.txt"},
        {"TMC0582-CONST-K.txt", "TMC0583-CONST-K.txt"},
    };
    static const ModelInfo kTI58C {
        "TI-58C", true, false, false, 4,
        {"CD2400.txt", "CD2401.txt", "TMC0573.txt"},
        {"CD2400-CONST-K.txt", "CD2401-CONST-K.txt"},
    };
    switch (m) {
    case Model::TI59:  return kTI59;
    case Model::TI58:  return kTI58;
    case Model::TI58C: return kTI58C;
    }
    return kTI59;
}

} // namespace cu59
