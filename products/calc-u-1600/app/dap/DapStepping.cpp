#include "app/dap/DapStepping.hpp"

#include "Debug/SharpDebugging.hpp"

namespace shared::dap {

bool locate(const DapTargets& targets, const debug::SourceMap& map, int thread, debug::SourceLocation* out) {
    if (!targets.valid()) return false;
    return map.lookup(thread, targets.pc(thread), targets.bankMatch(), out, targets.codePeek());
}

calcu::debugger::RunControl::Result step(calcu::debugger::RunControl& control, const DapTargets& targets,
                                         const debug::SourceMap& map, int thread, StepKind kind, bool line) {
    using LibKind = calcu::debugger::StepKind;
    if (!targets.valid()) return {false, "The machine offers no debug target."};
    const calcu::machine::DebugTargetId target = targets.targetId(thread);
    if (kind == StepKind::Out) return control.step(calcu::debugger::kCpu, 1, target, LibKind::Out);
    const LibKind lib = kind == StepKind::Over ? LibKind::Over : LibKind::In;
    debug::SourceLocation from;
    if (kind == StepKind::Instruction || !line || !locate(targets, map, thread, &from))
        return control.step(calcu::debugger::kCpu, 1, target, lib);
    // D-16: on while the thread is on the same source line; a step that
    // leaves the source shows the disassembly there.
    const debug::SourceMap* m = &map;
    calcu::debugger::StepTest sameLine = [m, thread, from](const calcu::machine::DebugTarget& t) {
        const auto& st = static_cast<const sharp::SharpDebugTarget&>(t);
        debug::SourceLocation now;
        const debug::BankMatch match = [&st](int, const debug::BankKey& k, std::uint16_t a) { return st.bankMatches(k, a); };
        const debug::CodePeek peek = [&st](int, std::uint16_t a, std::uint8_t* v) {
            return st.peekByte(debug::kSpaceMain, a, v);
        };
        if (!m->lookup(thread, st.pc(), match, &now, peek)) return false;
        return now.file == from.file && now.line == from.line;
    };
    return control.step(calcu::debugger::kCpu, kLineStepLimit, target, lib, 0, std::move(sameLine));
}

} // namespace shared::dap
