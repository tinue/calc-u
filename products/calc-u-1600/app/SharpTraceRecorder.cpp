#include "app/SharpTraceRecorder.hpp"

#include <cstdint>
#include <limits>
#include <vector>

#include "Machine/SharpProductKinds.hpp"

namespace cu1600 {

namespace {

constexpr std::uint64_t kAll = std::numeric_limits<std::uint64_t>::max();

// The core's records as they are; the library's TraceSession keeps the limit.
class SharpTraceRecorder final : public calcu::debugger::TraceRecorder {
public:
    explicit SharpTraceRecorder(sharp::TraceRecords& records) : records_(&records) {}

    void begin(std::vector<std::uint8_t>& out) override {
        if (records_ && records_->begin()) records_->take(out, kAll);
    }
    bool take(std::vector<std::uint8_t>& out, std::uint64_t budget) override {
        return !records_ || records_->take(out, budget);
    }
    // SESSION_END: a record header (type, length) and the event count.
    std::uint64_t endBytes() const override { return 3 + 4; }
    void end(std::vector<std::uint8_t>& out) override {
        if (!records_) return;
        records_->endRecord();
        records_->take(out, kAll);
        records_->end();
    }
    std::string buttonFileName() const override { return "TRACE.bin"; }   // AC-23
    void machineGone() override { records_ = nullptr; }

private:
    sharp::TraceRecords* records_;
};

} // namespace

std::unique_ptr<calcu::debugger::TraceRecorder> makeSharpTraceRecorder(calcu::machine::Machine& m) {
    sharp::TraceRecords* records = calcu::machine::first<sharp::TraceRecords>(m);
    if (!records) return nullptr;
    return std::make_unique<SharpTraceRecorder>(*records);
}

} // namespace cu1600
