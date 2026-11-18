#pragma once
#include "pacing/WallClock.hpp"
#include "sessions/AppSession.hpp"
#include "sessions/HostServices.hpp"
#include "sessions/Listener.hpp"
#include "sessions/Messages.hpp"
#include "sessions/TopicMirror.hpp"
#include <QObject>
#include <QStringList>
#include <cstdint>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

class QTimer;

namespace calcu::parts {
class LibraryParts;
}
namespace calcu::harness {
class HarnessProduct;
}

// The host of an app session in a Qt6 frontend: on the GUI thread, a 60 Hz
// timer with one host call per tick, a listener over the session's topics,
// commands and requests with their reply handlers, the host-service asks and, in builds with CALCU_ATTACH,
// harness attach (the library's AttachEndpoint and AttachedHarness).
//
// Messages that arrive during a host call are handed on after it returns,
// so a handler may send again (a sink does not call back into the session
// from deliver()).

namespace calcu::qt6 {

class ClipboardService;

class SessionHost : public QObject {
    Q_OBJECT
public:
    using Reply = std::function<void(const sessions::ReplyMessage&)>;
    using MessageHandler = std::function<void(const sessions::Message&)>;
    static constexpr int kCallsPerSecond = 60;

    SessionHost(sessions::AppSession&, sessions::HostServices, pacing::WallClock clock = pacing::steadyWallTime,
                QObject* parent = nullptr);
    ~SessionHost() override;

    void start();   // the 60 Hz QTimer (Qt::PreciseTimer): one call() per tick
    void stop();
    void call();    // one host call; then the attach harness's afterHostCall()

    sessions::CallId send(const sessions::CommandName&, sessions::Value args = {}, Reply = {});
    sessions::CallId ask(const sessions::RequestName&, sessions::Value args = {}, Reply = {});

    // Every message the listener takes (state, stream, asks, replies nobody
    // waits for), in order, after the host call it came in.
    void addMessageHandler(MessageHandler);

    const sessions::TopicMirror& topics() const { return listener_.mirror(); }
    sessions::Value value(const sessions::TopicName&) const;   // toValue() of the last state, or Null
    sessions::AppSession& session() { return session_; }
    pacing::WallTime now() const { return clock_(); }

    // Builds with CALCU_ATTACH only. Returns the problem; empty when
    // open. A port that cannot be bound gives a line in the session's log,
    // "Attach is not available on port N: <problem>.", and the app runs on.
    // `parts`: the library's parts of the session, which the attached
    // commands use.
    std::string openAttach(harness::HarnessProduct&, std::uint16_t port, parts::LibraryParts& parts);
    std::uint16_t attachPort() const;   // the bound port (port 0: the one the system picked)
    bool attachOpen() const;
    // `--attach-port N` (0: the system picks, for an in-process client),
    // else 59059.
    static std::uint16_t attachPortFrom(const QStringList& arguments);

signals:
    void topicChanged(const QString& name);
    void called();

private:
    void take(const sessions::Message&);
    void dispatch(const sessions::Message&);
    void flushHeld();
    void defaultService(const sessions::ServiceAsk&);

    sessions::AppSession& session_;
    pacing::WallClock clock_;
    sessions::Listener listener_;
    QTimer* timer_;
    std::unique_ptr<ClipboardService> clipboard_;
    std::vector<MessageHandler> handlers_;
    std::set<sessions::TopicName> streams_;
    bool inCall_ = false;
    std::vector<sessions::Message> held_;
    struct Attach;
    std::unique_ptr<Attach> attach_;
};

} // namespace calcu::qt6
