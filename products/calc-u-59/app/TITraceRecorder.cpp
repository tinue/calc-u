#include "TITraceRecorder.hpp"
#include "MachineModelInfo.hpp"
#include "TraceWriter.hpp"
#include "../core/TraceRecords.hpp"
#include <chrono>
#include <limits>
#include <vector>

namespace cu59 {

namespace {

class TITraceRecorder final : public calcu::debugger::TraceRecorder {
public:
    TITraceRecorder(TraceRecords& records, Model model, std::function<std::uint64_t()> clock)
        : m_records(&records), m_model(model), m_clock(std::move(clock)) {}

    void begin(std::vector<std::uint8_t>& out) override {
        const std::uint64_t now = m_clock ? m_clock()
                                          : static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(
                                                std::chrono::system_clock::now().time_since_epoch())
                                                                           .count());
        TraceWriter::sessionStart(out, m_model, now);
        if (m_records) m_records->begin();
    }

    bool take(std::vector<std::uint8_t>& out, std::uint64_t budget) override {
        std::uint64_t used = 0;
        for (;;) {
            if (m_lost) {   // a gap goes before the frames that follow it
                if (used + TraceWriter::kGapBytes > budget) return false;
                TraceWriter::gap(out, m_lost);
                used += TraceWriter::kGapBytes;
                m_lost = 0;
            }
            if (m_next == m_frames.size()) {
                m_frames.clear();
                m_next = 0;
                if (!m_records) return true;
                std::uint32_t lost = 0;
                const std::uint32_t n = m_records->take(m_frames, 1024, lost);
                m_lost = lost;
                if (n == 0 && lost == 0) return true;
                continue;
            }
            if (used + TraceWriter::kEventBytes > budget) return false;
            TraceWriter::event(out, m_frames[m_next++]);
            used += TraceWriter::kEventBytes;
            ++m_events;
        }
    }

    std::uint64_t endBytes() const override { return TraceWriter::kEndBytes; }

    void end(std::vector<std::uint8_t>& out) override {
        take(out, std::numeric_limits<std::uint64_t>::max());
        TraceWriter::sessionEnd(out, m_events);
        if (m_records) m_records->end();
    }

    std::string buttonFileName() const override { return TraceWriter::baseFileName(m_model); }
    void machineGone() override { m_records = nullptr; }

private:
    TraceRecords* m_records;
    Model m_model;
    std::function<std::uint64_t()> m_clock;
    std::vector<CpuFrame> m_frames;
    std::size_t m_next = 0;
    std::uint32_t m_lost = 0;
    std::uint32_t m_events = 0;
};

} // namespace

std::unique_ptr<calcu::debugger::TraceRecorder> makeTITraceRecorder(calcu::machine::Machine& m,
                                                                    std::function<std::uint64_t()> clock) {
    auto* records = calcu::machine::capabilityAs<TraceRecords>(m, calcu::machine::CapabilityId{"TMC0501/c59.trace-records"});
    if (!records) return nullptr;
    return std::make_unique<TITraceRecorder>(*records, modelFrom(m.model()).value_or(Model::TI59), std::move(clock));
}

} // namespace cu59
