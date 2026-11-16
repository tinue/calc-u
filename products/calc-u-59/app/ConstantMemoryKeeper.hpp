#pragma once
#include "ConstantMemoryFile.hpp"
#include "TIFileAccess.hpp"
#include "machine/Machine.hpp"
#include "machine/ProductData.hpp"
#include "pacing/WallClock.hpp"
#include "startup/MachinePlan.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <string>

// The file's side of the TI-58C's constant memory (C59-3 design §3.3, D-2):
// what a TI-58C built without a preset gets (AC-7, EC-1 to EC-3, EC-6),
// whether the running machine was built from the file (AC-8, AC-10, AC-12,
// EC-7, D-3), the write at the latest half a second of the session's wall
// time after a change (AC-5), the writes at once (AC-6) and the notices
// (EC-2 to EC-4, D-6, D-8). It reads the machine only through the kind
// `c59.constant-memory`; TIAppPart tells it the builds, the calls and the
// moments.

namespace cu59 {

class ConstantMemoryKeeper {
public:
    ConstantMemoryKeeper(TIFileAccess& files, std::function<void(std::string)> notice)
        : m_files(files), m_notice(std::move(notice)) {}

    /// AC-7, EC-1–EC-3, EC-6: what a TI-58C built without a preset gets (the
    /// file's registers, or zeroed memory); reads the file, once per build,
    /// and keeps its cue card for fileCueCard().
    std::shared_ptr<const calcu::machine::ProductData> contentForBuild();
    /// The cue card of the file the last contentForBuild() read (AC-7).
    const std::optional<CueCardContent>& fileCueCard() const { return m_fileCard; }

    /// A new machine: from the file when LIB-18 built a TI-58C at the start
    /// or a model switch with the content this keeper handed (D-3).
    void machineBuilt(calcu::machine::Machine*, const std::optional<calcu::startup::BuiltWith>&);
    bool fromFile() const { return m_fromFile; }

    /// AC-5: after each host call, with the session's wall time.
    void afterCall(calcu::machine::Machine*, const std::optional<CueCardContent>& ownCard, calcu::pacing::WallTime now);
    /// AC-6: at once, if the machine is from the file and something changed.
    void writeNow(calcu::machine::Machine*, const std::optional<CueCardContent>& ownCard);

private:
    std::optional<ConstantMemoryContents> current(calcu::machine::Machine*, const std::optional<CueCardContent>&) const;
    void write(const ConstantMemoryContents&);

    TIFileAccess& m_files;
    std::function<void(std::string)> m_notice;
    std::optional<CueCardContent> m_fileCard;
    std::shared_ptr<const calcu::machine::ProductData> m_handed;   // the content of the last contentForBuild()
    bool m_fromFile = false;
    std::optional<ConstantMemoryContents> m_written;   // what the file holds as far as this keeper knows
    std::optional<calcu::pacing::WallTime> m_due;
    bool m_failNoticed = false;   // D-8: once per app session
};

} // namespace cu59
