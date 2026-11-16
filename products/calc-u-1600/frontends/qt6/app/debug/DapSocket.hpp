#pragma once
#include <QObject>
#include <QPointer>

#include <cstdint>
#include <optional>
#include <string>

#include "app/dap/DapTransport.hpp"

class QTcpServer;
class QTcpSocket;

namespace shared::dap {
class DapPart;
}

// The Qt6 frontend's socket for the DAP server (C1600-3 design §3.2, §4):
// a QTcpServer on 127.0.0.1 only, one client; a second one gets the
// sentence it is given and is closed (AC-3). What arrives goes to the
// part on the GUI thread, which is the session's. It decides nothing.
class DapSocket final : public QObject, public shared::dap::DapTransport {
    Q_OBJECT
public:
    DapSocket(shared::dap::DapPart& part, QObject* parent = nullptr);
    ~DapSocket() override;

    std::optional<std::string> listen(std::uint16_t port, std::string busy) override;
    void close() override;
    void send(std::string bytes) override;
    void dropClient() override;
    void quitProgram() override;

private:
    void onNewConnection();
    void onReadyRead();

    shared::dap::DapPart& m_part;
    QTcpServer* m_server = nullptr;
    QPointer<QTcpSocket> m_client;
    std::string m_busy;
};
