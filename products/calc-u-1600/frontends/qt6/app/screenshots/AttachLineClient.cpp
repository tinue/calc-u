#include "AttachLineClient.hpp"

#include <QHostAddress>
#include <QTcpSocket>

AttachLineClient::AttachLineClient(QObject* parent) : QObject(parent), m_socket(new QTcpSocket(this)) {
    connect(m_socket, &QTcpSocket::readyRead, this, &AttachLineClient::readLines);
}

bool AttachLineClient::connectTo(std::uint16_t port, int timeoutMs) {
    m_socket->connectToHost(QHostAddress::LocalHost, port);
    return m_socket->waitForConnected(timeoutMs);
}

void AttachLineClient::send(const QString& line, Done done) {
    m_pending.push_back(Pending{line, std::move(done)});
    m_socket->write(line.toUtf8() + '\n');
    m_socket->flush();
}

void AttachLineClient::readLines() {
    m_buffer += m_socket->readAll();
    for (;;) {
        const int nl = m_buffer.indexOf('\n');
        if (nl < 0) return;
        const QString line = QString::fromUtf8(m_buffer.left(nl));
        m_buffer.remove(0, nl + 1);
        if (m_more < 0) {
            if (line.startsWith(QLatin1String("message "))) continue;   // the listener's messages
            m_reply = Reply{};
            m_reply.first = line;
            m_reply.done = line == QLatin1String("done") || line.startsWith(QLatin1String("done "));
            m_more = line.startsWith(QLatin1String("done ")) ? line.mid(5).toInt() : 0;
        } else {
            m_reply.lines << line;
            --m_more;
        }
        if (m_more == 0) {
            m_more = -1;
            if (m_pending.empty()) continue;
            Pending p = std::move(m_pending.front());
            m_pending.pop_front();
            if (p.done) p.done(m_reply);
        }
    }
}
