#include "app/SharpTraceRecorder.hpp"

#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "Machine/SharpProductKinds.hpp"

namespace cu1600 {

namespace {

constexpr std::uint64_t kAll = std::numeric_limits<std::uint64_t>::max();

// The core's records as they are; the library's TraceSession keeps the limit.
class SharpTraceRecorder final : public calcu::debugger::TraceRecorder {
public:
    SharpTraceRecorder(sharp::TraceRecords& records, std::string fileName)
        : records_(&records), fileName_(std::move(fileName)) {}

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
    std::string buttonFileName() const override { return fileName_; }
    void machineGone() override { records_ = nullptr; }

private:
    sharp::TraceRecords* records_;
    std::string fileName_;
};

// The button's file is named after the model: "PC-1500A" gives CALCU1500A_TRACE.bin.
std::string buttonFileNameOf(const calcu::machine::ModelId& model) {
    const std::string& m = model.value;
    return "CALCU" + (m.rfind("PC-", 0) == 0 ? m.substr(3) : m) + "_TRACE.bin";
}

} // namespace

std::unique_ptr<calcu::debugger::TraceRecorder> makeSharpTraceRecorder(calcu::machine::Machine& m) {
    sharp::TraceRecords* records = calcu::machine::first<sharp::TraceRecords>(m);
    if (!records) return nullptr;
    return std::make_unique<SharpTraceRecorder>(*records, buttonFileNameOf(m.model()));
}

} // namespace cu1600
