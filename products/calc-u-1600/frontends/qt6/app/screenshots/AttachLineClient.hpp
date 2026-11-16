#pragma once
#include <QByteArray>
#include <QObject>
#include <QString>
#include <QStringList>
#include <cstdint>
#include <deque>
#include <functional>

class QTcpSocket;

// A line client of harness attach (LIB-14 AC-44, D.6) on 127.0.0.1, in the
// app's own process: the serve protocol of LIB-11 §3.9 -- one command per
// line, one reply per command (`done`, `done <n>` and n lines,
// `refused: <reason>`); the attach's message lines are skipped. Asynchronous
// on the GUI thread's event loop, so the app's host calls go on while a
// reply is awaited; one command at a time, in order.
class AttachLineClient : public QObject {
    Q_OBJECT
public:
    struct Reply {
        bool done = false;
        QString first;        // the reply's first line
        QStringList lines;    // the result lines after `done <n>`
        QString reason() const { return first.startsWith(QLatin1String("refused: ")) ? first.mid(9) : first; }
    };
    using Done = std::function<void(const Reply&)>;

    explicit AttachLineClient(QObject* parent = nullptr);
    bool connectTo(std::uint16_t port, int timeoutMs = 3000);
    void send(const QString& line, Done done);
    bool busy() const { return !m_pending.empty(); }

private:
    void readLines();
    QTcpSocket* m_socket;
    QByteArray m_buffer;
    struct Pending {
        QString line;
        Done done;
    };
    std::deque<Pending> m_pending;
    Reply m_reply;
    int m_more = -1;   // result lines still to read; -1: waiting for a reply's first line
};
