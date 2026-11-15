#include "presets/Vocabulary.hpp"

namespace calcu::presets {

bool Vocabulary::known(std::string_view n) const {
    if (n == kKeysSection) return true;
    return header(n) || section(n) || step(n);
}

void Vocabulary::add(HeaderKeyDef d) {
    if (known(d.key)) duplicates_.push_back(d.key);
    else headers_.push_back(std::move(d));
}

void Vocabulary::add(SectionDef d) {
    if (known(d.key)) duplicates_.push_back(d.key);
    else sections_.push_back(std::move(d));
}

void Vocabulary::add(StepDef d) {
    if (known(d.verb) || d.verb == kSkipWait || d.verb == kLetItFinish) duplicates_.push_back(d.verb);
    else steps_.push_back(std::move(d));
}

const HeaderKeyDef* Vocabulary::header(std::string_view n) const {
    for (const auto& d : headers_)
        if (d.key == n) return &d;
    return nullptr;
}

const SectionDef* Vocabulary::section(std::string_view n) const {
    for (const auto& d : sections_)
        if (d.key == n) return &d;
    return nullptr;
}

const StepDef* Vocabulary::step(std::string_view n) const {
    for (const auto& d : steps_)
        if (d.verb == n) return &d;
    return nullptr;
}

std::vector<std::string> Vocabulary::rawVerbs() const {
    std::vector<std::string> out;
    for (const auto& d : steps_)
        if (d.value == StepDef::Value::Raw) out.push_back(d.verb);
    return out;
}

} // namespace calcu::presets
