#include "TIPresetCommands.hpp"
#include "Bcd.hpp"
#include "Keycodes.hpp"
#include "TIPresetKeys.hpp"
#include "sessions/Args.hpp"
#include "sessions/LibraryTopics.hpp"
#include "sessions/Payload.hpp"

namespace cu59 {

using calcu::sessions::CommandOutcome;
using calcu::sessions::Done;
using calcu::sessions::Refused;
using calcu::sessions::SessionContext;
using calcu::sessions::Value;

namespace {

CommandOutcome lines(const std::vector<std::string>& l) {
    Value::Array a;
    for (const auto& s : l) a.emplace_back(s);
    return Done{calcu::sessions::valuePayload("c59.lines", Value(std::move(a)))};
}

std::string three(int step) {
    std::string s = std::to_string(step);
    while (s.size() < 3) s = "0" + s;
    return s;
}

std::string two(int v) { return v < 10 ? "0" + std::to_string(v) : std::to_string(v); }

std::optional<int> stepArg(const Value& args, int lastStep, std::string* why) {
    const Value* v = args.find("step");
    if (!v || v->isNull()) return 0;
    std::string t = v->string() ? *v->string() : (v->integer() ? std::to_string(*v->integer()) : std::string());
    bool digits = !t.empty();
    for (char c : t) digits = digits && c >= '0' && c <= '9';
    if (!digits || t.size() > 4) {
        *why = "The step is a number of three digits (000 to " + three(lastStep) + ").";
        return std::nullopt;
    }
    const int s = std::stoi(t);
    if (s > lastStep) {
        *why = "The step " + t + " is beyond the last step, " + three(lastStep) + ".";
        return std::nullopt;
    }
    return s;
}

} // namespace

void ProgramMemoryPart::attach(calcu::sessions::SessionSetup& setup) {
    const char* busy = calcu::sessions::kAnotherOperationRuns;
    setup.command(commands::kPartition, [busy](const Value& args, SessionContext& s) -> CommandOutcome {
        calcu::machine::Machine* m = s.machine();
        ProgramMemory* pm = m ? programMemoryOf(*m) : nullptr;
        if (!pm) return Refused{calcu::sessions::kNoMachine};
        const Value* v = args.find("value");
        if (!v || v->isNull()) return lines({three(pm->partitionSteps() - 1)});
        if (s.busy()) return Refused{busy};
        const std::string text = v->string() ? *v->string() : std::string();
        auto r = partitionSteps(text, pm->lastStep());
        if (auto* why = std::get_if<std::string>(&r)) return Refused{*why};
        pm->setPartition(std::get<int>(r));
        return Done{};
    });
    setup.command(commands::kProgram, [busy](const Value& args, SessionContext& s) -> CommandOutcome {
        calcu::machine::Machine* m = s.machine();
        ProgramMemory* pm = m ? programMemoryOf(*m) : nullptr;
        if (!pm) return Refused{calcu::sessions::kNoMachine};
        std::string why;
        const auto step = stepArg(args, pm->lastStep(), &why);
        if (!step) return Refused{why};
        if (const Value* count = args.find("count"); count && !count->isNull()) {
            const std::int64_t n = count->integer() ? *count->integer() : -1;
            if (n < 1) return Refused{"count= is a number of steps, at least 1."};
            std::vector<std::string> out;
            for (int i = *step; i <= pm->lastStep() && i < *step + n; ++i)
                out.push_back(three(i) + " " + two(pm->readStep(i)));
            return lines(out);
        }
        if (s.busy()) return Refused{busy};
        const std::string codes = calcu::sessions::textArg(args, "keycodes");
        if (codes.empty()) return Refused{"usage: c59 program [<step>] <keycode>… | c59 program <step> count=<n>"};
        // The compact notation's rules from the step.
        for (char c : codes)
            if (c == '\n' || c == '#') return Refused{"The keycodes are two-digit numbers on one line."};
        auto r = readKeycodes(codes, pm->lastStep() - *step);
        if (auto* e = std::get_if<KeycodeError>(&r)) return Refused{"program: " + e->message};
        for (const ProgramStep& p : std::get<std::vector<ProgramStep>>(r)) pm->writeStep(*step + p.step, p.keycode);
        return Done{};
    });
    setup.command(commands::kDataRegisters, [busy](const Value& args, SessionContext& s) -> CommandOutcome {
        calcu::machine::Machine* m = s.machine();
        ProgramMemory* pm = m ? programMemoryOf(*m) : nullptr;
        if (!pm) return Refused{calcu::sessions::kNoMachine};
        const calcu::machine::ModelId model = m->descriptor().model;
        if (const Value* get = args.find("get"); get && get->elements()) {
            std::vector<std::string> out;
            for (const Value& k : *get->elements()) {
                const std::string key = k.string() ? *k.string() : std::string();
                auto dk = dataKey(key, model);
                if (auto* why = std::get_if<std::string>(&dk)) return Refused{*why};
                const DataKey d = std::get<DataKey>(dk);
                out.push_back(key + ": " + registerText(d.extra ? pm->readExtra(d.index) : pm->readRegister(d.index)));
            }
            return lines(out);
        }
        const Value* set = args.find("set");
        if (!set || !set->elements() || set->elements()->empty())
            return Refused{"usage: c59 data-registers <NN>=<value>… | c59 data-registers <NN>…"};
        if (s.busy()) return Refused{busy};
        struct W {
            DataKey key;
            RegisterNibbles value;
        };
        std::vector<W> writes;
        for (const Value& pair : *set->elements()) {
            const auto* e = pair.elements();
            if (!e || e->size() != 2 || !(*e)[0].string() || !(*e)[1].string()) return Refused{"A register is written NN=value."};
            auto dk = dataKey(*(*e)[0].string(), model);
            if (auto* why = std::get_if<std::string>(&dk)) return Refused{*why};
            const auto value = parseDecimal(*(*e)[1].string());
            if (!value)
                return Refused{"data: " + *(*e)[0].string() + ": the value is no number (a decimal number, an exponent allowed: -1.5e-3)."};
            writes.push_back({std::get<DataKey>(dk), *value});
        }
        for (const W& w : writes) {
            if (w.key.extra) pm->writeExtra(w.key.index, w.value);
            else pm->writeRegister(w.key.index, w.value);
        }
        return Done{};
    });
}

} // namespace cu59
