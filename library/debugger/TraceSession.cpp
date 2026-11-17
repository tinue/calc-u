#include "debugger/TraceSession.hpp"
#include "files/Files.hpp"
#include "sessions/LibraryTopics.hpp"
#include <filesystem>

namespace calcu::debugger {

namespace {
} // namespace

std::unique_ptr<TraceFile> TraceFile::open(const std::string& path, std::string* error) {
    std::unique_ptr<TraceFile> f(new TraceFile());
    f->out_.open(files::toPath(path), std::ios::binary | std::ios::trunc);
    if (!f->out_) {
        if (error) *error = "The trace file " + path + " cannot be made.";
        return nullptr;
    }
    return f;
}

std::unique_ptr<TraceFile> TraceFile::inMemory() {
    std::unique_ptr<TraceFile> f(new TraceFile());
    f->memory_ = true;
    return f;
}

void TraceFile::write(std::span<const std::uint8_t> b) {
    if (memory_) kept_.insert(kept_.end(), b.begin(), b.end());
    else out_.write(reinterpret_cast<const char*>(b.data()), static_cast<std::streamsize>(b.size()));
    bytes_ += b.size();
}

void TraceFile::close() {
    if (!memory_ && out_.is_open()) out_.close();
}

std::function<std::optional<settings::Paths::Ensured>()> traceFolderOf(const settings::Paths& paths,
                                                                        std::function<bool()> available) {
    return [&paths, available]() -> std::optional<settings::Paths::Ensured> {
        const std::optional<std::string> folder = paths.folder(settings::Folder::Traces);
        if (!folder) return std::nullopt;
        if (available && !available()) return settings::Paths::Ensured{false, *folder, "the trace location is not available"};
        return paths.ensure(settings::Folder::Traces);
    };
}

std::optional<std::string> traceNameRefusal(const std::string& name) {
    if (name.empty() || name.find('/') != std::string::npos || name.find('\\') != std::string::npos)
        return std::string("trace: takes a file name without a folder, or off.");
    return std::nullopt;
}

void TraceSession::setTopic(sessions::StateTopic* t) {
    topic_ = t;
    publish();
}

void TraceSession::publish() {
    if (!topic_) return;
    auto s = std::make_shared<TraceState>();
    s->running = running();
    s->fileName = running() ? name_ : std::string();
    topic_->set(std::move(s));
}

std::uint64_t TraceSession::limitBytes() const {
    long mb = setup_.limitMb ? setup_.limitMb() : 0;
    if (mb <= 0) mb = 50;
    return static_cast<std::uint64_t>(mb) * 1024 * 1024;
}

std::optional<std::string> TraceSession::start(sessions::SessionContext& c, std::string name, bool button,
                                               bool ofOperation) {
    machine::Machine* m = c.machine();
    if (!m) return std::string(sessions::kNoMachine);
    std::unique_ptr<TraceRecorder> recorder = setup_.recorder ? setup_.recorder(*m) : nullptr;
    if (!recorder) return std::string("The ") + m->model().value + " records no traces.";
    if (button && name.empty()) name = recorder->buttonFileName();
    if (auto why = traceNameRefusal(name)) return why;
    if (running()) end(c, Ending::Off);   // one trace at a time (OQ-3)

    std::unique_ptr<TraceFile> file;
    const std::optional<settings::Paths::Ensured> folder =
        setup_.folder ? setup_.folder() : std::optional<settings::Paths::Ensured>();
    if (!folder) {
        // D-10: kept in memory where a listener takes it (the Web, T4).
        if (!setup_.handOver || !c.hostOffers(sessions::HostService::KeepFile))
            return std::string("There is no folder for traces.");   // EC-8
        file = TraceFile::inMemory();
    } else {
        if (!folder->ok) {   // EC-8, AC-53: one sentence, its final period once (a system message may end in one)
            std::string why = "The trace folder cannot be written: " + folder->problem;
            if (why.back() != '.') why += '.';
            return why;
        }
        std::string error;
        file = TraceFile::open(files::fromPath(files::toPath(folder->path) / files::toPath(name)), &error);
        if (!file) return error;   // AC-22: the step fails
    }
    recorder_ = std::move(recorder);
    file_ = std::move(file);
    name_ = std::move(name);
    ofOperation_ = ofOperation;
    recorder_->begin(buffer_);
    write();
    publish();
    return std::nullopt;
}

void TraceSession::pump(sessions::SessionContext& c) {
    if (!running()) return;
    const std::uint64_t limit = limitBytes();
    const std::uint64_t used = file_->bytes() + recorder_->endBytes();
    const std::uint64_t budget = used >= limit ? 0 : limit - used;
    const bool all = recorder_->take(buffer_, budget);
    write();
    if (!all) end(c, Ending::Limit);   // T2
}

void TraceSession::write() {
    if (!buffer_.empty()) file_->write(buffer_);
    buffer_.clear();
}

void TraceSession::end(sessions::SessionContext& c, Ending why) {
    if (!running()) return;
    if (why == Ending::NewMachine) recorder_->machineGone();
    else if (why != Ending::Limit) pump(c);   // what ran since the last piece
    if (!running()) return;                   // the pump reached the limit and ended it
    recorder_->end(buffer_);
    if (why == Ending::Limit) {
        // AC-24, AC-43: of what waits, only the session's end goes in,
        // within the bytes the budget kept for it.
        const std::uint64_t n = recorder_->endBytes();
        if (buffer_.size() >= n) buffer_.erase(buffer_.begin(), buffer_.end() - static_cast<std::ptrdiff_t>(n));
        else buffer_.clear();
    }
    write();
    file_->close();
    const std::string name = name_;
    switch (why) {
    case Ending::Limit:   // T2, AC-24, D-11
        c.notice("The trace " + name + " reached the size limit of " + std::to_string(limitBytes() / (1024 * 1024)) +
                 " MB and ended.");
        break;
    case Ending::NewMachine: c.log("The trace " + name + " ended: a new machine was built."); break;
    case Ending::Off:
    case Ending::OperationEnded: break;
    }
    if (file_->memory()) {   // T4: handed to the user to keep (AC-26)
        std::vector<std::uint8_t> bytes = file_->takeMemory();
        c.askHost(sessions::HostService::KeepFile,
                  sessions::Value::object({{"name", name},
                                           {"bytes", sessions::Value(std::move(bytes))},
                                           {"mediaType", "application/octet-stream"},
                                           {"kind", "trace"}}),   // what a page lists as a trace (C59-4 AC-12)
                  nullptr);
    }
    file_.reset();
    recorder_.reset();
    name_.clear();
    ofOperation_ = false;
    publish();
}

} // namespace calcu::debugger
