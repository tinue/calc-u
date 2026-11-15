#include "PC1500/PC1500TypedInput.hpp"

#include "PC1500/PC1500Machine.hpp"

bool pc1500AtBasicPrompt(const PC1500Machine& machine) {
    constexpr uint16_t kPromptLoopLo = 0xE200;
    constexpr uint16_t kPromptLoopHi = 0xE4FF;
    const uint16_t pc = machine.debugPC();
    return pc >= kPromptLoopLo && pc <= kPromptLoopHi;
}
