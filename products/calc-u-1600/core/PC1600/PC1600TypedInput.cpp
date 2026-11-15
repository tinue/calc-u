#include "PC1600/PC1600TypedInput.hpp"

#include "PC1600/PC1600Machine.hpp"

KeyLatches pc1600ReadLatches(PC1600Machine& machine) {
    KeyLatches l;
    l.shift = (machine.memory().peek(0xF64E) & 0x02) != 0;
    l.kbii = (machine.memory().peek(0xF3C6) & 0x80) != 0;
    return l;
}

bool pc1600AtBasicPrompt(const PC1600Machine& machine) {
    constexpr uint16_t kCmdLoopLo = 0x9280;
    constexpr uint16_t kCmdLoopHi = 0x9300;
    const uint16_t pc = machine.sc7852().pc();
    return machine.sc7852Owns() && pc >= kCmdLoopLo && pc <= kCmdLoopHi;
}
