#pragma once
#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "app/dap/DapSession.hpp"
#include "app/dap/DapTransport.hpp"
#include "app/dap/DapWire.hpp"
#include "sessions/Payload.hpp"
#include "sessions/SessionPart.hpp"

// The DAP server: Calc-U-1600's app layer, in the session of every
// native host (SharpAppPart; the harness's parts), never in the Web's. It
// needs from its host only a socket on this computer (DapTransport), the
// settings and the app session; one client at a time, one DapSession per
// client. Single-threaded, on the session's thread.
//
// Names (SharpTopics.hpp has them too):
//   c1600.dap             state topic: {status, port, client}
//   c1600.dap-disconnect  command: ends the DAP session (Settings ▸ Debugger)
//   c1600.dap-port        command: {port} for this run (--dap), no setting changed

namespace cu1600::dap {

inline const calcu::sessions::TopicName kDapTopic{"c1600.dap"};
inline const calcu::sessions::CommandName kDapDisconnectCommand{"c1600.dap-disconnect"};
inline const calcu::sessions::CommandName kDapPortCommand{"c1600.dap-port"};

inline constexpr const char* kNoClient = "No debugger is attached.";
inline constexpr const char* kPortRange = "The DAP port must be from 1024 to 65535.";
inline constexpr const char* kSecondClient =
    "Calc-U-1600 already has a debugger attached. Stop that debug session (Shift+F5), or use Settings > Debugger > "
    "Disconnect in the app.\n";

// The topic's payload.
struct DapStatus final : calcu::sessions::PayloadOf<DapStatus> {
    static constexpr calcu::sessions::PayloadKind kKind{"c1600.dap"};
    std::string status;   // "Off", "Listening on 127.0.0.1:<port>", "Client connected on …", "Port <port> not available: <why>"
    int port = 0;
    bool client = false;
    bool same(const DapStatus& o) const { return status == o.status && port == o.port && client == o.client; }
    calcu::sessions::Value toValue() const override {
        return calcu::sessions::Value::object({{"status", status}, {"port", port}, {"client", client}});
    }
};

class DapPart final : public calcu::sessions::SessionPart {
public:
    explicit DapPart(DapDeps deps) : deps_(std::move(deps)) {}
    ~DapPart();
    DapDeps& deps() { return deps_; }

    // The host's socket; nullptr: none (the server is off). The part listens
    // at once when it should.
    void setTransport(DapTransport*);
    // What the host's socket got, in order, on the session's thread.
    void take(DapInput);
    // `--dap <port>` for this run; the refusal: the server is then off
    // for the run.
    std::optional<std::string> setRunPort(long port);
    // The machine should run now (the harness's serve loop).
    bool wantsTime() const;
    const DapStatus& status() const { return status_; }
    bool hasClient() const { return client_; }

    // ---- SessionPart ----
    void attach(calcu::sessions::SessionSetup&) override;
    void afterCall(calcu::sessions::SessionContext&, calcu::pacing::WallTime) override;
    void machineChanged(calcu::sessions::SessionContext&) override;
    void operationEnded(calcu::sessions::SessionContext&) override;
    void closing(calcu::sessions::SessionContext&) override;

private:
    void refreshListening();
    void publish();
    void drain();
    // The client is gone or is sent away: the session ends once its request
    // is finished.
    void endSession();
    void finishEnd();

    DapDeps deps_;
    calcu::sessions::SessionContext* ctx_ = nullptr;
    calcu::sessions::StateTopic* topic_ = nullptr;
    DapTransport* transport_ = nullptr;

    std::optional<long> runPort_;
    bool runPortBad_ = false;
    std::optional<std::pair<bool, int>> lastWanted_;   // (wanted, port) of the last listen
    bool listening_ = false;
    std::string listenError_;
    int port_ = 0;
    bool client_ = false;
    DapStatus status_;

    DapReader reader_;
    std::deque<Value> held_;   // the messages that wait for an operation to end
    bool draining_ = false;
    std::unique_ptr<DapSession> session_;
    std::unique_ptr<DapSession> retired_;   // a gone client's, while its request is finished
    bool endPending_ = false;
};

} // namespace cu1600::dap
