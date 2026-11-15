#pragma once
#include "harness/DisplayText.hpp"
#include "harness/Reply.hpp"
#include "harness/WavRecorder.hpp"
#include "machine/Display.hpp"
#include "sessions/AppSession.hpp"
#include "sessions/LibraryTopics.hpp"
#include "sessions/TopicMirror.hpp"
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// The harness's side of the app session (LIB-11 design §3.4; spec AC-5 to
// AC-7, AC-11; harness.md I-1): the listener that keeps the topics, the
// recording sound sink, and commands whose operations run in one go.
// - No wall clock (AC-6, harness.md I-2): the session is hosted in
//   Hosting::VirtualTime; the host never makes the host call and emulated
//   time passes only through advance() (from `wait`) and through operations
//   run in one go (LIB-3 AC-19). Messages are delivered at once, on the
//   caller's thread. No thread, no lock (ADR 0009).
// - Nothing but replies (AC-5): notices and log lines are kept for `state`.

namespace calcu::harness {

class HarnessProduct;
class RunSetup;

struct TopicRead {
    std::string topic;
    bool stream = false;
    sessions::PayloadPtr state;                  // a state topic: its last state (may be null: never sent)
    bool reset = false;                          // a stream topic: it started over since the last read
    std::vector<sessions::PayloadPtr> items;     // a stream topic: what was appended since the last read
};

// A payload value in this area's Value: doubles with six decimals, bytes
// as "0x…" (design D-10).
Value fromSession(const sessions::Value&);

class HarnessHost final : public sessions::MessageSink {
public:
    HarnessHost(sessions::AppSession&, HarnessProduct&, const RunSetup&);
    ~HarnessHost();
    HarnessHost(const HarnessHost&) = delete;
    HarnessHost& operator=(const HarnessHost&) = delete;

    // session.send(me, cmd): an operation it starts runs to its end inside
    // send() (VirtualTime), so the reply is that of its end (AC-7).
    sessions::ReplyMessage command(sessions::Command);
    sessions::ReplyMessage ask(sessions::Request);
    machine::Ticks advance(machine::Ticks);       // AC-6, AC-9: session.advanceUnpaced
    std::optional<TopicRead> readTopic(std::string_view name);   // AC-11, EC-5
    std::vector<sessions::TopicEntry> topics();
    const machine::DisplayOutput* display() const;   // the mirror's `display` state
    const machine::MachineDescriptor* descriptor();
    DisplayText displayText();                       // through the product's rule (AC-10)
    // AC-9 `audio`: from now on all sound output into a WAV file at `path`;
    // a second call completes the first file and starts the new one (D-8).
    Reply record(const std::string& path);
    void finish();   // completes the WAV file (EC-3) and stops listening

    sessions::AppSession& session() { return session_; }
    HarnessProduct& product() { return product_; }
    const RunSetup& setup() const { return setup_; }

    void deliver(const sessions::Message&) override;

private:
    struct SoundSink final : sessions::MessageSink {
        WavRecorder wav;
        void deliver(const sessions::Message&) override;
    };
    sessions::ReplyMessage takeReply(sessions::CallId);

    sessions::AppSession& session_;
    HarnessProduct& product_;
    const RunSetup& setup_;
    sessions::TopicMirror mirror_;
    sessions::ListenerId me_ = 0;
    std::vector<sessions::ReplyMessage> replies_;
    std::unique_ptr<SoundSink> sound_;
    std::optional<sessions::ListenerId> soundListener_;
    bool finished_ = false;
};

} // namespace calcu::harness
