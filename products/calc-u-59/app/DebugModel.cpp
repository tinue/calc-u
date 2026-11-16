#include "DebugModel.hpp"
#include <algorithm>

namespace cu59 {

void DebugModel::attach(const std::vector<uint8_t>& moduleImage) {
    moduleChanged(moduleImage);
    m_live = {};
    m_listing.clear();
    m_listingSource = ProgramSource::UserProgram;
    m_cpuHistory.clear();
    m_cpuHistorySeqno.reset();
    m_lastLibMismatch = -1;
}

void DebugModel::moduleChanged(const std::vector<uint8_t>& moduleImage) {
    m_moduleError.clear();
    m_module = ModuleImage::parse(moduleImage, &m_moduleError);
    m_romCache.clear();
}

DebugModel::Update DebugModel::tick(const calcu::machine::Machine& m, bool livePanel, bool cpuPanel, bool frozen) {
    Update u;
    if (livePanel && !frozen) u |= rebuildLive(m, false);
    if (cpuPanel && !frozen) {
        readTraceRing(m, m_frames);
        // Running: rebuild the list only when new instructions ran.
        uint32_t newest = m_frames.empty() ? 0 : m_frames.back().seqno;
        if (!frozen && newest != m_cpuHistorySeqno) {
            m_cpuHistorySeqno = newest;
            m_cpuHistory = recentInstructions(m_frames);
            u.cpu = true;
        }
    }
    return u;
}

DebugModel::Update DebugModel::frozenView(const calcu::machine::Machine& m, FrozenViewReason reason) {
    Update u;
    if (reason != FrozenViewReason::StepInstruction) u |= rebuildLive(m, true);
    u |= captureInspector(m);
    return u;
}

DebugModel::Update DebugModel::rebuildLive(const calcu::machine::Machine& m, bool frozen) {
    Update u;
    LiveDebugBuild b = buildLiveSnapshot(m, m_module, m_romCache, calcViewState(m));
    if (b.libProgramMismatch && b.libProgramMismatch->second != m_lastLibMismatch) {
        m_lastLibMismatch = b.libProgramMismatch->second;
        u.diagnostic = "Library exec PC resolves to Pgm " + std::to_string(b.libProgramMismatch->first) +
                       " but SCOM[9] says Pgm " + std::to_string(b.libProgramMismatch->second);
    }
    // solidStateReturn shows a raw CROM address: keep the listing, whose
    // marker stays on the RTN until the module's next fetch.
    if (frozen && b.snap.programSource != ProgramSource::SolidStateReturn) {
        auto listing = programListing(b.sourceKeycodes, b.argSteps, b.snap.currentStep);
        if (listing != m_listing || b.snap.programSource != m_listingSource) {
            m_listing = std::move(listing);
            m_listingSource = b.snap.programSource;
            u.listing = true;
        }
    }
    if (b.snap != m_live) {
        m_live = std::move(b.snap);
        u.live = true;
    }
    return u;
}

DebugModel::Update DebugModel::captureInspector(const calcu::machine::Machine& m) {
    m_cpuHistory = inspectorHistory(m);
    m_cpuHistorySeqno.reset();  // the next running frame rebuilds the list
    Update u;
    u.cpu = true;
    return u;
}

int DebugModel::listingCurrentIndex() const {
    auto it = std::find_if(m_listing.begin(), m_listing.end(), [](const StepEntry& e) { return e.isCurrent; });
    return it == m_listing.end() ? -1 : static_cast<int>(it - m_listing.begin());
}

} // namespace cu59
