#include "app/SharpTraceRecorder.hpp"

#include <cstdint>
#include <limits>
#include <span>
#include <vector>

#include "Machine/SharpProductKinds.hpp"

namespace shared {

namespace {

class SharpTraceRecorder final : public calcu::debugger::TraceRecorder {
public:
    explicit SharpTraceRecorder(sharp::TraceRecords& records) : records_(&records) {}

    void begin(calcu::debugger::TraceFile& file) override {
        if (!records_ || !records_->begin()) return;
        move(file, std::numeric_limits<std::uint64_t>::max());
    }
    bool pump(calcu::debugger::TraceFile& file, std::uint64_t budget) override {
        if (!records_) return true;
        const bool all = move(file, budget);
        if (!all) full_ = true;
        return all;
    }
    // SESSION_END: a record header (type, length) and the event count.
    std::uint64_t endBytes() const override { return 3 + 4; }
    void end(calcu::debugger::TraceFile& file) override {
        if (!records_) return;
        records_->endRecord();
        if (full_) {
            // The limit ended it (AC-24, AC-43): of what waits, only the
            // session's end goes in, within the bytes kept for it.
            buffer_.clear();
            records_->take(buffer_, std::numeric_limits<std::uint64_t>::max());
            if (buffer_.size() >= endBytes())
                file.write(std::span<const std::uint8_t>(buffer_).last(static_cast<std::size_t>(endBytes())));
        } else {
            move(file, std::numeric_limits<std::uint64_t>::max());
        }
        records_->end();
    }
    std::string buttonFileName() const override { return "TRACE.bin"; }   // AC-23
    void machineGone() override { records_ = nullptr; }

private:
    bool move(calcu::debugger::TraceFile& file, std::uint64_t budget) {
        buffer_.clear();
        const bool all = records_->take(buffer_, budget);
        if (!buffer_.empty()) file.write(buffer_);
        return all;
    }

    sharp::TraceRecords* records_;
    std::vector<std::uint8_t> buffer_;
    bool full_ = false;   // a pump had more than its budget
};

} // namespace

std::unique_ptr<calcu::debugger::TraceRecorder> makeSharpTraceRecorder(calcu::machine::Machine& m) {
    sharp::TraceRecords* records = sharp::firstOf<sharp::TraceRecords>(m);
    if (!records) return nullptr;
    return std::make_unique<SharpTraceRecorder>(*records);
}

} // namespace shared
