#pragma once
#include "DebugLog.hpp"
#include "DebugSnapshots.hpp"
#include "ModuleImage.hpp"
#include <optional>
#include <string>
#include <vector>

// The debug panel's data, for TIDebugPart's topics: the
// CALCULATOR tab's snapshot and program listing and the CPU tab's
// instruction history. Rebuilt at frame rate while running (only for tabs
// shown) and once per pause / step while paused. On the session's thread;
// reads the machine between two advances. The heatmap is the core's count
// (c59.rom-heatmap) and the log the library's `log` topic.

namespace cu59 {

/// Why a paused machine is shown: a pause or a start that fired, or a
/// `cpu` step (which leaves the CALCULATOR tab as it was).
enum class FrozenViewReason {
    Freeze,           ///< a pause landed, or an armed start fired
    StepInstruction,  ///< a step of the level cpu
};

class DebugModel {
public:
    /// What a call changed, for the panels to redraw.
    struct Update {
        bool live = false, listing = false, cpu = false;
        std::optional<std::string> diagnostic;  ///< developer log line
        Update& operator|=(const Update& o) {
            live |= o.live;
            listing |= o.listing;
            cpu |= o.cpu;
            if (o.diagnostic) diagnostic = o.diagnostic;
            return *this;
        }
    };

    /// A new machine: its module's image (empty: none), a fresh listing and history.
    void attach(const std::vector<uint8_t>& moduleImage);
    /// The module changed (no reset).
    void moduleChanged(const std::vector<uint8_t>& moduleImage);
    /// Why the module's program table couldn't be read, or empty.
    const std::string& moduleImageError() const { return m_moduleError; }

    /// The frame: the CALCULATOR snapshot while running and its panel
    /// shows; the running CPU history while its panel shows.
    Update tick(const calcu::machine::Machine& m, bool livePanel, bool cpuPanel, bool frozen);
    /// The machine stopped at a point to show (freeze, armed freeze, step).
    Update frozenView(const calcu::machine::Machine& m, FrozenViewReason reason);
    /// The CALCULATOR snapshot now, and when frozen the listing (only a frozen
    /// panel shows it).
    Update rebuildLive(const calcu::machine::Machine& m, bool frozen);


    const LiveDebugSnapshot& live() const { return m_live; }
    /// The whole program of the source at the last freeze (the frozen PROGRAM STEPS view).
    const std::vector<StepEntry>& listing() const { return m_listing; }
    /// The machine runs again: no listing. True: it had one.
    bool clearListing() {
        const bool had = !m_listing.empty();
        m_listing.clear();
        return had;
    }
    ProgramSource listingSource() const { return m_listingSource; }
    int listingCurrentIndex() const;
    /// Running: the newest 32 instructions; frozen: the trace ring plus the next.
    const std::vector<InspectorEntry>& cpuHistory() const { return m_cpuHistory; }

private:
    Update captureInspector(const calcu::machine::Machine& m);

    ModuleImage m_module;
    std::string m_moduleError;
    std::vector<uint8_t> m_romCache;
    LiveDebugSnapshot m_live;
    std::vector<StepEntry> m_listing;
    ProgramSource m_listingSource = ProgramSource::UserProgram;
    std::vector<InspectorEntry> m_cpuHistory;
    std::vector<CpuFrame> m_frames;  ///< the 60 Hz trace-ring read, reused
    std::optional<uint32_t> m_cpuHistorySeqno;  ///< newest frame in a running history
    int m_lastLibMismatch = -1;
};

} // namespace cu59
