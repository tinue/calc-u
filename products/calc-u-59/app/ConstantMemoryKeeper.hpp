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

// The file's side of the TI-58C's constant memory: what a TI-58C built
// without a preset gets, whether the running machine was built from the
// file, the write at the latest half a second of the session's wall time
// after a change, the writes at once and the notices. It reads the machine only through the kind
// `c59.constant-memory`; TIAppPart tells it the builds, the calls and the
// moments.

namespace cu59 {

class ConstantMemoryKeeper {
public:
    // `warn`: the app log's line of a failure, with the path and the cause the
    // notice leaves out. `savedAt`: the time of a write for the file's
    // `saved:` key (ISO-8601 UTC).
    ConstantMemoryKeeper(TIFileAccess& files, std::function<void(std::string)> notice,
                         std::function<void(std::string)> warn, std::function<std::string()> savedAt = {})
        : m_files(files), m_notice(std::move(notice)), m_warn(std::move(warn)), m_savedAt(std::move(savedAt)) {}

    /// What a TI-58C built without a preset gets (the
    /// file's registers, or zeroed memory); reads the file, once per build,
    /// and keeps its cue card for fileCueCard().
    std::shared_ptr<const calcu::machine::ProductData> contentForBuild();
    /// The cue card of the file the last contentForBuild() read.
    const std::optional<CueCardContent>& fileCueCard() const { return m_fileCard; }

    /// A new machine: from the file when the library's machine control built
    /// a TI-58C at the start or a model switch with the content this keeper
    /// handed.
    void machineBuilt(calcu::machine::Machine*, const std::optional<calcu::startup::BuiltWith>&);
    bool fromFile() const { return m_fromFile; }

    /// After each host call, with the session's wall time.
    void afterCall(calcu::machine::Machine*, const std::optional<CueCardContent>& ownCard, calcu::pacing::WallTime now);
    /// At once, if the machine is from the file and something changed.
    void writeNow(calcu::machine::Machine*, const std::optional<CueCardContent>& ownCard);

private:
    std::optional<ConstantMemoryContents> current(calcu::machine::Machine*, const std::optional<CueCardContent>&) const;
    void write(const ConstantMemoryContents&);
    std::string path() const;
    void warn(const std::string&) const;

    TIFileAccess& m_files;
    std::function<void(std::string)> m_notice;
    std::function<void(std::string)> m_warn;
    std::function<std::string()> m_savedAt;
    std::optional<CueCardContent> m_fileCard;
    std::shared_ptr<const calcu::machine::ProductData> m_handed;   // the content of the last contentForBuild()
    bool m_fromFile = false;
    std::optional<ConstantMemoryContents> m_written;   // what the file holds as far as this keeper knows
    std::optional<calcu::pacing::WallTime> m_due;
    bool m_failNoticed = false;   // once per app session
};

} // namespace cu59
