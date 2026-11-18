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

// The trace session: one trace at a time, started by the step `trace:`, the command or a
// button, limited and ended by the library; what the records are is the
// product's (TraceRecorder).

namespace calcu::debugger {

// The library's: a file in the trace folder, or memory (the Web).
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

// The product's record source, one per machine: what the next
// bytes of the file are. TraceSession writes them and keeps the size limit:
// it alone decides how much goes into the file.
class TraceRecorder {
public:
    virtual ~TraceRecorder() = default;
    // Turns the core's recording on; appends the header and the session start.
    virtual void begin(std::vector<std::uint8_t>& out) = 0;
    // Appends whole records of what ran since the last call, at most
    // `budget` bytes; false: more were waiting than the budget took.
    virtual bool take(std::vector<std::uint8_t>& out, std::uint64_t budget) = 0;
    virtual std::uint64_t endBytes() const = 0;   // the size of the session's end record
    // Appends the records still waiting and then the session's end record
    // (the last endBytes() bytes); turns the core's recording off.
    virtual void end(std::vector<std::uint8_t>& out) = 0;
    virtual std::string buttonFileName() const = 0;   // the name of a trace the button starts
    // The machine was replaced: the recorder may not touch its core again.
    virtual void machineGone() {}
};

struct TraceSetup {
    // The product's recorder for a machine; unset or nullptr: no traces.
    std::function<std::unique_ptr<TraceRecorder>(machine::Machine&)> recorder;
    // The trace folder as Paths::ensure(Folder::Traces) gives it, asked at
    // each trace start (the problem a phrase without its final period);
    // nullopt or unset: this host has none (the Web).
    std::function<std::optional<settings::Paths::Ensured>()> folder;
    // [trace] max-file-size-mb; zero or less, or unset: 50.
    std::function<long()> limitMb;
    // Without a folder, a trace is kept in memory and handed over with
    // KeepFile when a listener offers it; false: such a start fails.
    bool handOver = false;
};

// The trace folder of a product's Paths, for TraceSetup::folder: the places
// and settings as they are at each call, the folder made and written once
// (Paths::ensure, which also asks the Swift frontend's iCloud).
std::function<std::optional<settings::Paths::Ensured>()> traceFolderOf(const settings::Paths&);

class TraceSession {
public:
    enum class Ending { Off, OperationEnded, NewMachine, Limit };

    explicit TraceSession(TraceSetup s) : setup_(std::move(s)) {}
    void setTopic(sessions::StateTopic* t);
    // A name with `/` or `\` is refused; a running trace ends first.
    // `name` empty with `button`: the product's name. The reason
    // when it does not start.
    std::optional<std::string> start(sessions::SessionContext&, std::string name, bool button, bool ofOperation);
    void end(sessions::SessionContext&, Ending);
    void pump(sessions::SessionContext&);   // after each piece of the gate
    bool running() const { return file_ != nullptr; }
    bool ofOperation() const { return ofOperation_; }
    const std::string& name() const { return name_; }
    std::uint64_t limitBytes() const;

private:
    void publish();
    void write();   // buffer_ into the file
    TraceSetup setup_;
    sessions::StateTopic* topic_ = nullptr;
    std::unique_ptr<TraceRecorder> recorder_;
    std::unique_ptr<TraceFile> file_;
    std::vector<std::uint8_t> buffer_;   // the recorder's bytes on their way to the file
    std::string name_;
    bool ofOperation_ = false;
};

// The check of a trace's name: the reason it is refused, or none.
std::optional<std::string> traceNameRefusal(const std::string& name);

} // namespace calcu::debugger
