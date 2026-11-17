#include "TITypingSupport.hpp"
#include "TIKeys.hpp"
#include "TIMachine.hpp"

std::optional<std::uint64_t> TITypingSupport::samples(const calcu::machine::KeyId& id) const {
    const TIKey* k = tiKeyById(id.value);
    if (!k) return std::nullopt;
    return m_.m_hw.keySamples(kTIKeyLineBits[k->col], k->row);
}


// ── The idle/key-detect range ────────────────────────────────────────────────
std::array<std::uint16_t, 4> idleAndKeyDetectRange(const calcu::machine::ModelId& model) {
    const MachineVariant v = model.value == "TI-58" ? MachineVariant::TI58
                             : model.value == "TI-58C" ? MachineVariant::TI58C
                                                       : MachineVariant::TI59;
    const PcRange r = romRanges(v).idleAndKeyDetect;
    return {r.lo, r.hi, r.extraLo, r.extraHi};
}
