#include "DapSocket.hpp"

#include <QCoreApplication>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

#include "app/dap/DapPart.hpp"

using cu1600::dap::DapInput;

DapSocket::DapSocket(cu1600::dap::DapPart& part, QObject* parent) : QObject(parent), m_part(part) {
    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &DapSocket::onNewConnection);
}

DapSocket::~DapSocket() {
    if (m_client) m_client->abort();
    m_server->close();
}

std::optional<std::string> DapSocket::listen(std::uint16_t port, std::string busy) {
    close();
    m_busy = std::move(busy);
    if (m_server->listen(QHostAddress::LocalHost, port)) return std::nullopt;
    return m_server->errorString().toStdString();
}

void DapSocket::close() {
    dropClient();
    m_server->close();
}

void DapSocket::onNewConnection() {
    while (QTcpSocket* socket = m_server->nextPendingConnection()) {
        if (m_client) {   // one client at a time: the newcomer is told why
            socket->write(m_busy.data(), static_cast<qint64>(m_busy.size()));
            socket->disconnectFromHost();
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            continue;
        }
        m_client = socket;
        connect(socket, &QTcpSocket::readyRead, this, &DapSocket::onReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, [this, socket] {
            socket->deleteLater();
            if (m_client != socket) return;
            m_client = nullptr;
            m_part.take(DapInput{DapInput::Kind::Disconnected, {}});
        });
        m_part.take(DapInput{DapInput::Kind::Connected, {}});
    }
}

void DapSocket::onReadyRead() {
    if (!m_client) return;
    const QByteArray bytes = m_client->readAll();
    m_part.take(DapInput{DapInput::Kind::Bytes, std::string(bytes.constData(), static_cast<std::size_t>(bytes.size()))});
}

void DapSocket::send(std::string bytes) {
    if (m_client) m_client->write(bytes.data(), static_cast<qint64>(bytes.size()));
}

void DapSocket::dropClient() {
    if (!m_client) return;
    QTcpSocket* socket = m_client;
    m_client = nullptr;   // the part knows already: no Disconnected for it
    socket->flush();
    socket->disconnectFromHost();
}

void DapSocket::quitProgram() {
    // A normal quit, so macOS does not take it for a crash.
    QTimer::singleShot(0, qApp, &QCoreApplication::quit);
}
