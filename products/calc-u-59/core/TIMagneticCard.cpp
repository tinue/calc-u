#include "TIMagneticCard.hpp"

namespace cu59 {

std::uint8_t pageByteOfBank(int bank) { return static_cast<std::uint8_t>(0x10 | ((bank - 1) * 3)); }
int bankOfPageByte(std::uint8_t pageByte) { return (pageByte & 0x0F) / 3 + 1; }

int firstStepOfBank(int bank) { return 240 * (bank - 1); }
bool stepInBank(int step, int bank) { return step >= firstStepOfBank(bank) && step < firstStepOfBank(bank) + 240; }
bool registerInBank(int reg, int bank) {
    const int raw = 119 - reg;   // data register n is raw register 119 − n
    return reg >= 0 && reg < 100 && raw >= 30 * (bank - 1) && raw < 30 * bank;
}

std::vector<std::uint8_t> buildCardBank(const TICardSideDef& card, int programRegs) {
    if (card.partitionSteps) programRegs = *card.partitionSteps / 8;
    const int bank = card.bank;
    const int firstReg = 30 * (bank - 1);
    // The bank's 30 raw RAM registers. A step is two nibbles of register
    // step / 8, units first; data register n is raw register 119 − n.
    std::vector<RegisterNibbles> regs(30, RegisterNibbles{});
    for (const auto& [step, keycode] : card.steps) {
        const int reg = step / 8 - firstReg;
        if (reg < 0 || reg >= 30) continue;
        regs[static_cast<std::size_t>(reg)][static_cast<std::size_t>((step & 7) * 2)] = keycode % 10;
        regs[static_cast<std::size_t>(reg)][static_cast<std::size_t>((step & 7) * 2 + 1)] = keycode / 10;
    }
    for (const auto& [n, value] : card.registers) {
        const int reg = 119 - n - firstReg;
        if (reg >= 0 && reg < 30) regs[static_cast<std::size_t>(reg)] = value;
    }
    const int partition = programRegs / 10;
    std::vector<std::uint8_t> d(kCardBankBytes, 0);
    d[0] = static_cast<std::uint8_t>(0x10 | partition);
    d[1] = static_cast<std::uint8_t>(0x10 | (partition >= 3 * bank ? 1 : 0));   // the bank entirely in program memory
    d[2] = pageByteOfBank(bank);
    d[3] = card.isProtected ? 0x10 : 0x00;
    unsigned sum = (d[0] & 0xFu) + (d[1] & 0xFu) + (d[2] & 0xFu);
    for (std::size_t r = 0; r < 30; ++r) {
        for (std::size_t k = 0; k < 8; ++k) {   // byte k: nibbles 14 − 2k and 15 − 2k
            const std::size_t i = 14 - 2 * k;
            d[4 + 8 * r + k] = static_cast<std::uint8_t>((regs[r][i] << 4) | regs[r][i + 1]);
        }
        for (std::uint8_t nibble : regs[r]) sum += nibble;
    }
    d[244] = d[245] = static_cast<std::uint8_t>(sum & 0xF);
    return d;
}

std::optional<int> sideBank(const TIMagneticCardData::Side& s) {
    if (const auto* def = std::get_if<TICardSideDef>(&s)) return def->bank;
    if (const auto* bytes = std::get_if<std::vector<std::uint8_t>>(&s); bytes && bytes->size() > 2)
        return bankOfPageByte((*bytes)[2]);
    return std::nullopt;
}

} // namespace cu59
