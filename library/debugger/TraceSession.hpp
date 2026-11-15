#pragma once
#include "debugger/RunState.hpp"
#include "machine/Machine.hpp"
#include "sessions/SessionPart.hpp"
#include "sessions/Topics.hpp"
#include "settings/Paths.hpp"
#include <cstdint>
#include <fstream>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

// The trace session (LIB-10 design §3.7; spec AC-22 to AC-26, OQ-1, OQ-3):
// one trace at a time, started by the step `trace:`, the command or a
// button, limited and ended by the library; what the file holds is the
// product's (TraceRecorder, ADR 0037).

namespace calcu::debugger {

// The library's: a file in the trace folder, or memory (the Web, T1).
class TraceFile {
public:
    static std::unique_ptr<TraceFile> open(const std::string& path, std::string* error);   // replaces a file of that name
    static std::unique_ptr<TraceFile> inMemory();
    void write(std::span<const std::uint8_t>);
    std::uint64_t bytes() const { return bytes_; }
    bool memory() const { return memory_; }
    std::vector<std::uint8_t> takeMemory() { return std::move(kept_); }
    void close();

private:
    TraceFile() = default;
    bool memory_ = false;
    std::ofstream out_;
    std::vector<std::uint8_t> kept_;
    std::uint64_t bytes_ = 0;
};

// The product's, one per machine (ADR 0037).
class TraceRecorder {
public:
    virtual ~TraceRecorder() = default;
    virtual void begin(TraceFile&) = 0;   // header, session start; turns on the core's recording
    // Whole records of what ran since the last pump, at most `budget` bytes;
    // false: there was more than the budget (the limit is reached).
    virtual bool pump(TraceFile&, std::uint64_t budget) = 0;
    virtual std::uint64_t endBytes() const = 0;   // the size of what end() writes
    virtual void end(TraceFile&) = 0;             // session end; turns the core's recording off
    virtual std::string buttonFileName() const = 0;   // AC-23
    // The machine was replaced: the recorder may not touch its core again.
    virtual void machineGone() {}
};

struct TraceSetup {
    // The product's recorder for a machine; unset or nullptr: no traces.
    std::function<std::unique_ptr<TraceRecorder>(machine::Machine&)> recorder;
    // The trace folder as Paths::ensure(Folder::Traces) gives it; nullopt or
    // unset: this host has none (the Web, LIB-9 AC-17).
    std::function<std::optional<settings::Paths::Ensured>()> folder;
    // [trace] max-file-size-mb (LIB-9 AC-11); zero or less, or unset: 50.
    std::function<long()> limitMb;
    // D-10: without a folder, a trace is kept in memory and handed over with
    // KeepFile when a listener offers it; false: such a start fails.
    bool handOver = false;
};

// The trace folder of a product's Paths, for TraceSetup::folder.
std::function<std::optional<settings::Paths::Ensured>()> traceFolderOf(const settings::Paths&);

class TraceSession {
public:
    enum class Ending { Off, OperationEnded, NewMachine, Limit };

    explicit TraceSession(TraceSetup s) : setup_(std::move(s)) {}
    void setTopic(sessions::StateTopic* t);
    void setFolder(std::function<std::optional<settings::Paths::Ensured>()> f) { setup_.folder = std::move(f); }
    void setLimit(std::function<long()> f) { setup_.limitMb = std::move(f); }
    // T1: a name with `/` or `\` is refused; a running trace ends first.
    // `name` empty with `button`: the product's name (AC-23). The reason
    // when it does not start.
    std::optional<std::string> start(sessions::SessionContext&, std::string name, bool button, bool ofOperation);
    void end(sessions::SessionContext&, Ending);
    void pump(sessions::SessionContext&);   // T2: after each piece of the gate
    bool running() const { return file_ != nullptr; }
    bool ofOperation() const { return ofOperation_; }
    const std::string& name() const { return name_; }
    std::uint64_t limitBytes() const;

private:
    void publish();
    TraceSetup setup_;
    sessions::StateTopic* topic_ = nullptr;
    std::unique_ptr<TraceRecorder> recorder_;
    std::unique_ptr<TraceFile> file_;
    std::string name_;
    bool ofOperation_ = false;
};

// T1's check of a name: the reason it is refused, or none.
std::optional<std::string> traceNameRefusal(const std::string& name);

} // namespace calcu::debugger
