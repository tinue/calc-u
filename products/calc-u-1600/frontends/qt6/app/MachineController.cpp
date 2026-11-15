#include "MachineController.hpp"

#include <QDebug>
#include <QMessageBox>
#include <QTimer>
#include <QtGlobal>

#include <chrono>
#include <cstdlib>

#include "AppPaths.hpp"
#include "AppSettings.hpp"
#include "debug/DebugController.hpp"
#include "Machine/SharpBuilder.hpp"
#include "sessions/LibraryTopics.hpp"

namespace sessions = calcu::sessions;

namespace {
// The frozen clock: every read is this much host work, so an operation's
// call at full speed ends at its allowance (LIB-17 AC-5) on this clock too.
constexpr std::chrono::microseconds kFrozenReadCost{100};
constexpr std::chrono::nanoseconds kFrame{16'666'667};
} // namespace

MachineController::MachineController(QObject* parent) : QObject(parent) {
    shared::SharpSessionOptions options;
    options.clock = [this] { return now(); };
    m_app = shared::makeSharpSession(AppSettings::settings(), AppPaths::paths(), std::move(options));
    m_listener.onMessage = [this](const sessions::Message& m) { onMessage(m); };
    sessions::HostServices services;
    services.offered = {sessions::HostService::AudioSink,     sessions::HostService::ClipboardText,
                        sessions::HostService::ClipboardImage, sessions::HostService::FilePicker,
                        sessions::HostService::FolderPicker,   // File ▸ Mount Directory
                        sessions::HostService::KeepFile,       sessions::HostService::OpenUrl};
    m_listener.listen(*m_app.session, std::move(services));
    m_debug = std::make_unique<DebugController>(this);
    app().setDebugger(this);
    // The start: the startup model of the settings, then its default preset
    // (an operation; the window shows "Loading…" meanwhile). A file of the
    // ROM full set missing or bad (LIB-12 AC-5): no machine starts -- the
    // lookup's message, then exit; an abort would be a silent crash for an
    // app not launched from a terminal, and this is reached mid-construction
    // of MainWindow, which is not set up to unwind from here.
    send(shared::commands::kStart, {}, [this](const sessions::ReplyMessage& r) {
        if (r.done) return;
        if (const auto* missing = m_listener.state<shared::RomsMissingPayload>(shared::topics::kRomsMissing)) {
            QMessageBox::critical(nullptr, QObject::tr("Calc-U-1600 cannot start"),
                                  QString::fromStdString(calcu::roms::describe(missing->missing)));
            std::exit(1);
        }
    });
    m_debug->refreshServer();
}

MachineController::~MachineController() {
    // The debugger goes first: its target refers to the live machine.
    app().setDebugger(nullptr);
    m_debug.reset();
    m_listener.stop();
    m_app = {};
}

calcu::pacing::WallTime MachineController::now() {
    if (m_frozen) {
        m_frozenNow += kFrozenReadCost;
        return m_frozenNow;
    }
    const auto real = std::chrono::duration_cast<calcu::pacing::WallTime>(std::chrono::steady_clock::now().time_since_epoch());
    m_frozenNow = std::max(m_frozenNow, real);   // never back, also after a frozen stretch
    return m_frozenNow;
}

void MachineController::call() {
    const calcu::pacing::WallTime t = now();
    if (m_calls++ > 0) m_longestGapMs = std::max(m_longestGapMs, std::chrono::duration<double, std::milli>(t - m_lastCall).count());
    m_lastCall = t;
    m_app.session->call(t);
    m_callMs += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch() - t).count();
}

void MachineController::send(const sessions::CommandName& name, sessions::Value args, Reply done) {
    const auto sent = m_listener.send(name, std::move(args));
    if (sent.reply) {   // a command that started no operation: replied at once
        if (done) done(*sent.reply);
        return;
    }
    // An operation: its reply when it ends, shown after the host call.
    m_listener.onReply(sent.call, [this, done = std::move(done)](const sessions::ReplyMessage& r) {
        if (!done) return;
        sessions::ReplyMessage reply = r;
        // A failed operation's reason was a notice already (LIB-3 AC-18).
        if (!reply.done && reply.reason == m_lastNotice) reply.reason.clear();
        QTimer::singleShot(0, this, [done, reply] { done(reply); });
    });
    if (m_frozen) finishOperation();
}

void MachineController::setFrozen(bool frozen) {
    if (frozen == m_frozen) return;
    now();   // the frozen clock starts at the real one
    m_frozen = frozen;
    if (frozen) finishOperation();
}

void MachineController::finishOperation() {
    while (m_app.session->busy()) {
        m_frozenNow += kFrame;
        call();
    }
}

void MachineController::runFor(double seconds) {
    const int frames = static_cast<int>(seconds * 60 + 0.5);
    for (int i = 0; i < frames; ++i) {
        m_frozenNow += kFrame;
        call();
    }
}

bool MachineController::runUntilTyped(double capSeconds) {
    const int frames = static_cast<int>(capSeconds * 60);
    for (int i = 0; i < frames && !app().typing().idle(); ++i) {
        m_frozenNow += kFrame;
        call();
    }
    return app().typing().idle();
}

void MachineController::attachAudio(shared::AudioSink& sink) {
    m_sound = std::make_unique<shared::SoundFeed>(sink, [](const std::string& text) {
        qWarning().noquote() << QString::fromStdString(text);
    });
    m_sound->warmUp(sharp::kSoundRate);
}

void MachineController::onMessage(const sessions::Message& m) {
    if (const auto* s = std::get_if<sessions::StateMessage>(&m)) {
        onState(s->topic);
    } else if (const auto* st = std::get_if<sessions::StreamMessage>(&m)) {
        for (const auto& p : st->appended) {
            if (const auto* n = sessions::payloadAs<sessions::Notice>(p)) {
                m_lastNotice = n->sentence;
                // Queued: a notice from inside a host call is shown once the
                // call is over, not while it runs.
                QTimer::singleShot(0, this, [this, text = QString::fromStdString(n->sentence)] { emit noticeShown(text); });
            } else if (const auto* l = sessions::payloadAs<sessions::LogLine>(p)) {
                qDebug().noquote() << "[log]" << QString::fromStdString(l->text);
            }
        }
        (void)m_listener.mirror().takeAppended(st->topic);   // kept here only
    } else if (std::holds_alternative<sessions::ReplyMessage>(m)) {
        // every reply has its handler (send())
    } else if (m_sound) {
        m_sound->deliver(m);   // SoundStart, SoundSamples, SoundStop (LIB-3 AC-20, AC-37)
    }
}

void MachineController::onState(const sessions::TopicName& topic) {
    if (topic == sessions::kDisplayTopic) {
        if (const auto* d = m_listener.display()) m_frame = shared::displayFrameOf(*d);
        emit displayChanged();
    } else if (topic == sessions::kBusyTopic) {
        const auto* b = m_listener.state<sessions::BusyState>(sessions::kBusyTopic);
        if (!b) return;
        const bool full = b->speed == calcu::pacing::Speed::Full;
        const bool was = m_busy, wasFull = m_busyFull;
        m_busy = b->operationRuns;
        m_busyFull = full;
        if (was != m_busy) QTimer::singleShot(0, this, [this, busy = m_busy] { emit appBusyChanged(busy); });
        else if (m_busy && wasFull != full) emit operationSpeedChanged();
    } else if (topic == shared::topics::kMachine) {
        const auto* p = m_listener.state<shared::MachinePayload>(topic);
        if (!p) return;
        const shared::MachinePayload before = m_machine;
        m_machine = *p;
        if (before.model != p->model || before.modelSwitches != p->modelSwitches) emit modelChanged(p->model);
        for (std::size_t slot = 1; slot < p->moduleChanges.size(); ++slot)
            if (slot < before.moduleChanges.size() && before.moduleChanges[slot] != p->moduleChanges[slot])
                emit bayModuleChanged(static_cast<int>(slot));
    } else if (topic == shared::topics::kTapes) {
        const auto* p = m_listener.state<shared::TapesPayload>(topic);
        if (p && p->listChanges != m_tapeListChanges) {
            m_tapeListChanges = p->listChanges;
            emit tapeListChanged();
        }
    } else if (topic == shared::topics::kSerialLinks) {
        const auto* p = m_listener.state<shared::SerialLinksPayload>(topic);
        if (p && p->moves != m_serialMoves) {
            m_serialMoves = p->moves;
            emit serialLinksMoved();
        }
    } else if (topic == shared::topics::kControlState) {
        if (m_busy) QTimer::singleShot(0, this, [this] { emit presetWasArmed(); });
    }
}

shared::ControlState MachineController::controlState() const {
    const auto* p = m_listener.state<shared::ControlStatePayload>(shared::topics::kControlState);
    return p ? p->state : session().controlState();
}

TapeDeck::Status MachineController::tapeStatus() const {
    const auto* p = m_listener.state<shared::TapesPayload>(shared::topics::kTapes);
    return p ? p->status : TapeDeck::Status{};
}

GrayImage MachineController::currentScreenImage() const {
    const auto* d = m_listener.display();
    return d ? session().screenImage(*d) : GrayImage{};
}

bool MachineController::lcdText(LcdText* out) const {
    const auto* d = m_listener.display();
    return d && session().lcdText(*d, out);
}

void MachineController::machineAboutToChange() {
    if (m_debug) m_debug->machineAboutToChange();
}

void MachineController::machineReplaced() {
    if (m_debug) m_debug->machineReplaced();
}

bool MachineController::runDebuggerSlice(std::uint64_t ticks) {
    if (!m_debug || !m_debug->attached()) return false;
    m_debug->runSlice(ticks);
    return true;
}

void MachineController::refreshDebugServer() { m_debug->refreshServer(); }

bool MachineController::tapeEject(QString* error) {
    return withQtError(error, [this](std::string* err) { return app().tapeEject(err); });
}
