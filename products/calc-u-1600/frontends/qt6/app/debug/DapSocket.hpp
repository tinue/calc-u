#pragma once
#include <QObject>
#include <QPointer>

#include <cstdint>
#include <optional>
#include <string>

#include "app/dap/DapTransport.hpp"

class QTcpServer;
class QTcpSocket;

namespace cu1600::dap {
class DapPart;
}

// The Qt6 frontend's socket for the DAP server:
// a QTcpServer on 127.0.0.1 only, one client; a second one gets the
// sentence it is given and is closed. What arrives goes to the
// part on the GUI thread, which is the session's. It decides nothing.
class DapSocket final : public QObject, public cu1600::dap::DapTransport {
    Q_OBJECT
public:
    DapSocket(cu1600::dap::DapPart& part, QObject* parent = nullptr);
    ~DapSocket() override;

    std::optional<std::string> listen(std::uint16_t port, std::string busy) override;
    void close() override;
    void send(std::string bytes) override;
    void dropClient() override;
    void quitProgram() override;

private:
    void onNewConnection();
    void onReadyRead();

    cu1600::dap::DapPart& m_part;
    QTcpServer* m_server = nullptr;
    QPointer<QTcpSocket> m_client;
    std::string m_busy;
};
