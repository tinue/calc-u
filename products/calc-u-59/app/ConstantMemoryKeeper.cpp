#include "ConstantMemoryKeeper.hpp"
#include "TIPresetKeys.hpp"
#include "../core/TICatalogue.hpp"
#include <chrono>

namespace cu59 {

std::shared_ptr<const calcu::machine::ProductData> ConstantMemoryKeeper::contentForBuild() {
    m_fileCard.reset();
    m_written.reset();
    auto content = std::make_shared<TIConstantMemory>();   // zeroed (EC-1)
    bool unreadable = false;
    const std::optional<std::string> text = m_files.readConstantMemory(&unreadable);
    if (text) {
        auto r = readConstantMemoryFile(*text);
        if (auto* c = std::get_if<ConstantMemoryContents>(&r)) {
            for (int reg = 0; reg < ConstantMemory::kRegisters; ++reg)
                for (int n = 0; n < 16; ++n)
                    content->ram[static_cast<std::size_t>(reg * 16 + n)] = c->ram[static_cast<std::size_t>(reg)][static_cast<std::size_t>(n)];
            m_fileCard = c->cueCard;
            m_written = *c;
        } else {
            const ConstantMemoryProblem& p = std::get<ConstantMemoryProblem>(r);
            if (p.kind == ConstantMemoryProblem::Kind::Damaged)   // EC-3
                m_notice("The constant memory file " + std::string(kConstantMemoryFileName) + " cannot be read (" + p.what +
                         "); the TI-58C starts with zeroed memory.");
            else if (p.kind == ConstantMemoryProblem::Kind::Newer)
                m_notice("The constant memory file " + std::string(kConstantMemoryFileName) +
                         " was written by a newer release; it is replaced at the first change.");
            else   // EC-2, D-6
                m_notice("The constant memory file " + std::string(kConstantMemoryFileName) +
                         " is outdated; it is replaced at the first change. Its format is described in the User Guide, "
                         "chapter Formats.");
        }
    } else if (unreadable) {
        m_notice("The constant memory file " + std::string(kConstantMemoryFileName) +
                 " cannot be read; the TI-58C starts with zeroed memory.");
    }
    m_handed = content;
    return content;
}

void ConstantMemoryKeeper::machineBuilt(calcu::machine::Machine* m, const std::optional<calcu::startup::BuiltWith>& built) {
    using calcu::startup::BuildOrigin;
    m_due.reset();
    m_fromFile = m && built && constantMemoryOf(*m) && m_handed && built->plan.content == m_handed &&
                 (built->origin == BuildOrigin::Start || built->origin == BuildOrigin::ModelSwitch);
    if (!m_fromFile) m_written.reset();
}

std::optional<ConstantMemoryContents> ConstantMemoryKeeper::current(calcu::machine::Machine* m,
                                                                    const std::optional<CueCardContent>& ownCard) const {
    ConstantMemory* k = m ? constantMemoryOf(*m) : nullptr;
    if (!k) return std::nullopt;
    ConstantMemoryContents c;
    c.ram = k->content();
    c.cueCard = ownCard;
    return c;
}

void ConstantMemoryKeeper::write(const ConstantMemoryContents& c) {
    m_due.reset();
    if (m_files.writeConstantMemory(writeConstantMemoryFile(c))) {
        m_written = c;
        return;
    }
    if (!m_failNoticed) {   // EC-4, D-8: once per app session; the next change tries again
        m_failNoticed = true;
        m_notice("The constant memory could not be saved to " + std::string(kConstantMemoryFileName) + ".");
    }
    m_written = c;   // the next change tries again
}

void ConstantMemoryKeeper::afterCall(calcu::machine::Machine* m, const std::optional<CueCardContent>& ownCard,
                                     calcu::pacing::WallTime now) {
    if (!m_fromFile) return;
    const auto c = current(m, ownCard);
    if (!c) return;
    if (m_written && *c == *m_written) {
        m_due.reset();
        return;
    }
    if (!m_due) m_due = now + std::chrono::milliseconds(500);
    if (now >= *m_due) write(*c);
}

void ConstantMemoryKeeper::writeNow(calcu::machine::Machine* m, const std::optional<CueCardContent>& ownCard) {
    if (!m_fromFile) return;
    const auto c = current(m, ownCard);
    if (!c || (m_written && *c == *m_written)) return;
    write(*c);
}

} // namespace cu59
