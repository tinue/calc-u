#include "qt6/SessionHost.hpp"
#include "harness/attach/AttachPort.hpp"
#include "qt6/ClipboardService.hpp"
#include "sessions/LibraryTopics.hpp"
#include <QDesktopServices>
#include <QMetaObject>
#include <QTimer>
#include <QUrl>
#include <variant>

#ifdef CALCU_ATTACH
#include "harness/attach/AttachEndpoint.hpp"
#include "harness/attach/AttachedHarness.hpp"
#endif

namespace calcu::qt6 {

#ifdef CALCU_ATTACH
// The attach host (§3.9): what the endpoint's I/O thread posts is handed to
// the attached harness on the GUI thread through the event loop.
struct SessionHost::Attach final : harness::AttachInbox {
    SessionHost* host = nullptr;
    std::unique_ptr<harness::AttachEndpoint> endpoint;
    std::unique_ptr<harness::AttachedHarness> harness;
    void post(harness::AttachEvent e) override {
        QMetaObject::invokeMethod(
            host, [h = host, e = std::move(e)] {
                if (h->attach_ && h->attach_->harness) h->attach_->harness->handle(e);
            },
            Qt::QueuedConnection);
    }
    ~Attach() {
        harness.reset();    // a listener of the session: goes first
        endpoint.reset();   // closes the client and the socket, joins
    }
};
#else
struct SessionHost::Attach {};
#endif

SessionHost::SessionHost(sessions::AppSession& s, sessions::HostServices services, pacing::WallClock clock, QObject* parent)
    : QObject(parent), session_(s), clock_(std::move(clock)), timer_(new QTimer(this)),
      clipboard_(std::make_unique<ClipboardService>()) {
    timer_->setTimerType(Qt::PreciseTimer);
    timer_->setInterval(1000 / kCallsPerSecond);
    connect(timer_, &QTimer::timeout, this, &SessionHost::call);
    listener_.onMessage = [this](const sessions::Message& m) { take(m); };
    // The listener sends every state topic once at listen: held until now.
    inCall_ = true;
    listener_.listen(session_, std::move(services));
    inCall_ = false;
    QTimer::singleShot(0, this, [this] { flushHeld(); });
}

SessionHost::~SessionHost() {
    attach_.reset();
    listener_.stop();
}

void SessionHost::start() { timer_->start(); }
void SessionHost::stop() { timer_->stop(); }

void SessionHost::call() {
    inCall_ = true;
    session_.call(clock_());
    inCall_ = false;
#ifdef CALCU_ATTACH
    if (attach_ && attach_->harness) attach_->harness->afterHostCall();   // a due `wait` replies
#endif
    flushHeld();
    emit called();
}

sessions::CallId SessionHost::send(const sessions::CommandName& n, sessions::Value args, Reply reply) {
    flushHeld();
    return listener_.send(n, std::move(args), std::move(reply)).call;
}

sessions::CallId SessionHost::ask(const sessions::RequestName& n, sessions::Value args, Reply reply) {
    flushHeld();
    if (!reply) reply = [](const sessions::ReplyMessage&) {};
    return listener_.ask(n, std::move(args), std::move(reply));
}

void SessionHost::setServiceHandler(std::function<void(const sessions::ServiceAsk&)> h) { service_ = std::move(h); }

void SessionHost::answer(sessions::CallId ask, sessions::ServiceAnswer a) {
    flushHeld();
    session_.answer(ask, std::move(a));
}

void SessionHost::addMessageHandler(MessageHandler h) { handlers_.push_back(std::move(h)); }

sessions::Value SessionHost::value(const sessions::TopicName& t) const {
    const sessions::PayloadPtr* p = listener_.mirror().state(t);
    return p && *p ? (*p)->toValue() : sessions::Value();
}

void SessionHost::take(const sessions::Message& m) {
    if (inCall_) held_.push_back(m);
    else dispatch(m);
}

void SessionHost::flushHeld() {
    while (!held_.empty()) {
        std::vector<sessions::Message> q;
        q.swap(held_);
        for (const auto& m : q) dispatch(m);
    }
}

void SessionHost::dispatch(const sessions::Message& m) {
    if (const auto* a = std::get_if<sessions::ServiceAsk>(&m)) {
        if (service_) service_(*a);
        else defaultService(*a);
    }
    for (const auto& h : handlers_) h(m);
    if (const auto* s = std::get_if<sessions::StateMessage>(&m)) emit topicChanged(QString::fromStdString(s->topic.value));
    if (const auto* s = std::get_if<sessions::StreamMessage>(&m)) {
        streams_.insert(s->topic);
        emit topicChanged(QString::fromStdString(s->topic.value));
        // The handlers took what they keep; the mirror does not keep it.
        listener_.mirror().takeAppended(s->topic);
        listener_.mirror().takeReset(s->topic);
    }
}

void SessionHost::defaultService(const sessions::ServiceAsk& a) {
    sessions::ServiceAnswer answer;
    if (ClipboardService::handles(a.service)) {
        answer = clipboard_->answer(a);
    } else if (a.service == sessions::HostService::OpenUrl) {
        const sessions::Value* url = a.args.find("url");
        answer.given = url && url->string() && QDesktopServices::openUrl(QUrl(QString::fromStdString(*url->string())));
        if (!answer.given) answer.reason = "The address could not be opened.";
    } else {
        answer.reason = "This frontend does not offer it.";
    }
    session_.answer(a.ask, std::move(answer));
}

std::string SessionHost::openAttach(harness::HarnessProduct& product, std::uint16_t port) {
#ifdef CALCU_ATTACH
    attach_.reset();
    auto a = std::make_unique<Attach>();
    a->host = this;
    a->endpoint = std::make_unique<harness::AttachEndpoint>(*a, port);
    const auto opened = a->endpoint->open();
    if (!opened.ok) {   // FS 4, C59-5 EC-1: the app runs on
        session_.context().log("Attach is not available on port " + std::to_string(port) + ": " + opened.problem + ".");
        return opened.problem;
    }
    // Qt6 sends no suspend (LIB-3 AC-36): calls are never suspended.
    a->harness = std::make_unique<harness::AttachedHarness>(session_, product, *a->endpoint, [] { return false; });
    attach_ = std::move(a);
    return {};
#else
    (void)product;
    (void)port;
    return "Attach is not built into this frontend.";
#endif
}

std::uint16_t SessionHost::attachPort() const {
#ifdef CALCU_ATTACH
    return attach_ && attach_->endpoint ? attach_->endpoint->port() : 0;
#else
    return 0;
#endif
}

bool SessionHost::attachOpen() const {
#ifdef CALCU_ATTACH
    return attach_ && attach_->harness;
#else
    return false;
#endif
}

std::uint16_t SessionHost::attachPortFrom(const QStringList& arguments) {
    std::vector<std::string> args;
    for (int i = 0; i < arguments.size(); ++i) {
        const QString& a = arguments[i];
        if ((a == QStringLiteral("--attach-port") && i + 1 < arguments.size() && arguments[i + 1] == QStringLiteral("0")) ||
            a == QStringLiteral("--attach-port=0"))
            return 0;
        args.push_back(a.toStdString());
    }
    return harness::attachPort(args);
}

} // namespace calcu::qt6
