#include "app/dap/DapPart.hpp"

#include <memory>

#include "sessions/Args.hpp"
#include "settings/SettingsFile.hpp"

namespace shared::dap {

using calcu::sessions::CommandOutcome;
using calcu::sessions::Done;
using calcu::sessions::Refused;
using calcu::sessions::SessionContext;

DapPart::~DapPart() = default;

void DapPart::attach(calcu::sessions::SessionSetup& setup) {
    ctx_ = &setup.context();
    topic_ = &setup.stateTopic(kDapTopic);
    publish();

    // Settings ▸ Debugger ▸ Disconnect (AC-10, AC-18): the client is told
    // `terminated`, then dropped; the machine runs on.
    setup.command(kDapDisconnectCommand, [this](const Value&, SessionContext&) -> CommandOutcome {
        if (!client_ || !session_) return Refused{kNoClient};
        session_->toldToGo();
        if (transport_) transport_->dropClient();
        client_ = false;
        held_.clear();
        reader_.clear();
        endSession();
        publish();
        return Done{};
    });

    // --dap <port> for this run (AC-2, EC-1, D-6): a port out of range is a
    // notice too, and the server is off for the run.
    setup.command(kDapPortCommand, [this](const Value& args, SessionContext& c) -> CommandOutcome {
        const std::optional<std::string> refused = setRunPort(static_cast<long>(calcu::sessions::integerArg(args, "port")));
        if (refused) {
            c.notice(*refused);
            return Refused{*refused};
        }
        return Done{};
    });
}

void DapPart::setTransport(DapTransport* t) {
    if (transport_ && transport_ != t) transport_->close();
    transport_ = t;
    lastWanted_.reset();
    refreshListening();
}

std::optional<std::string> DapPart::setRunPort(long port) {
    if (port < 1024 || port > 65535) {
        runPort_.reset();
        runPortBad_ = true;
        refreshListening();
        return std::string(kPortRange);
    }
    runPort_ = port;
    runPortBad_ = false;
    refreshListening();
    return std::nullopt;
}

void DapPart::refreshListening() {
    if (!transport_) return;
    bool wanted = false;
    int port = 0;
    if (runPortBad_) {
        wanted = false;
    } else if (runPort_) {
        wanted = true;
        port = static_cast<int>(*runPort_);
    } else if (deps_.settings) {
        wanted = deps_.settings->boolean("debug", "dap-enabled");
        port = static_cast<int>(deps_.settings->integer("debug", "dap-port"));
    }
    const std::pair<bool, int> key{wanted, wanted ? port : 0};
    if (lastWanted_ && *lastWanted_ == key) return;
    lastWanted_ = key;
    if (client_) {   // a rebind drops the client: its session ends
        client_ = false;
        held_.clear();
        reader_.clear();
        endSession();
    }
    transport_->close();
    listening_ = false;
    listenError_.clear();
    port_ = port;
    if (wanted) {
        if (port < 1 || port > 65535) {
            listenError_ = "not a port";
        } else {
            Value::Object busy{{"seq", Value(0)},
                               {"type", Value("event")},
                               {"event", Value("output")},
                               {"body", Value::object({{"category", Value("important")}, {"output", Value(kSecondClient)}})}};
            const std::string busyBytes = frame(writeJson(Value(std::move(busy))));
            const std::optional<std::string> problem = transport_->listen(static_cast<std::uint16_t>(port), busyBytes);
            listening_ = !problem;
            if (problem) listenError_ = *problem;
        }
    }
    publish();
}

void DapPart::publish() {
    const bool wanted = lastWanted_ && lastWanted_->first;
    DapStatus s;
    s.port = port_;
    s.client = client_;
    if (!wanted) s.status = "Off";
    else if (!listening_) s.status = "Port " + std::to_string(port_) + " not available: " + listenError_;
    else if (client_) s.status = "Client connected on 127.0.0.1:" + std::to_string(port_);
    else s.status = "Listening on 127.0.0.1:" + std::to_string(port_);
    status_ = s;
    if (topic_) topic_->set(std::make_shared<DapStatus>(std::move(s)));
}

void DapPart::take(DapInput in) {
    if (!ctx_) return;
    switch (in.kind) {
    case DapInput::Kind::Connected: {
        if (session_ && session_->operationRuns()) {   // EC-5: the old request is finished first
            session_->detach();
            retired_ = std::move(session_);
        } else if (session_) {
            finishEnd();
        }
        client_ = true;
        reader_.clear();
        held_.clear();
        DapSession::Host host;
        host.send = [this](const std::string& body) {
            if (transport_) transport_->send(frame(body));
        };
        host.dropClient = [this] {
            if (transport_) transport_->dropClient();
        };
        host.quitProgram = [this] {
            if (transport_) transport_->quitProgram();
        };
        session_ = std::make_unique<DapSession>(deps_, *ctx_, std::move(host));
        endPending_ = false;
        publish();
        break;
    }
    case DapInput::Kind::Bytes:
        if (!client_) break;
        reader_.feed(in.bytes);
        while (std::optional<std::string> body = reader_.next())
            if (std::optional<Value> message = parseJson(*body)) held_.push_back(std::move(*message));
        drain();
        break;
    case DapInput::Kind::Disconnected:
        if (!client_) break;
        client_ = false;
        held_.clear();   // the old client's; a new one's must not see them
        reader_.clear();
        endSession();
        publish();
        break;
    }
}

// AC-8: a message that arrives while an operation runs waits; the held ones
// are answered in the order of arrival when it is over, until one starts an
// operation again.
void DapPart::drain() {
    if (draining_ || !ctx_) return;
    draining_ = true;
    while (!held_.empty() && session_ && !endPending_ && !ctx_->busy()) {
        const Value message = std::move(held_.front());
        held_.pop_front();
        session_->handle(message);
        if (!session_) break;
        session_->poll();
        if (session_->closing()) {   // `disconnect`, `calcu1600/quit`: answered, the client goes
            client_ = false;
            held_.clear();
            reader_.clear();
            endSession();
            publish();
            break;
        }
    }
    draining_ = false;
}

void DapPart::endSession() {
    if (!session_) return;
    if (session_->operationRuns()) {   // EC-5: its request is finished first
        endPending_ = true;
        return;
    }
    finishEnd();
}

void DapPart::finishEnd() {
    endPending_ = false;
    if (!session_) return;
    session_->end();
    session_.reset();
}

bool DapPart::wantsTime() const { return (session_ && session_->wantsTime()) || !held_.empty(); }

void DapPart::afterCall(SessionContext&, calcu::pacing::WallTime) {
    refreshListening();   // the settings may have changed (AC-2)
    if (session_) session_->poll();
    if (!held_.empty()) drain();
}

void DapPart::machineChanged(SessionContext&) {
    if (session_) session_->machineChanged();
}

void DapPart::operationEnded(SessionContext&) {
    if (retired_) {
        retired_->operationEnded();
        if (!retired_->operationRuns()) {
            retired_->end();
            retired_.reset();
        }
        if (!session_) return;
    }
    if (!session_) return;
    session_->operationEnded();
    if (endPending_ && !session_->operationRuns()) {
        finishEnd();
        publish();
        return;
    }
    drain();
}

void DapPart::closing(SessionContext&) {
    if (session_ && !session_->operationRuns()) finishEnd();
}

} // namespace shared::dap
