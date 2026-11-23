#include "MachineController.hpp"

#include <QDebug>
#include <QTimer>

#include <algorithm>
#include <chrono>

#include "AppPaths.hpp"
#include "AppSettings.hpp"
#include "Machine/SharpBuilder.hpp"
#include "app/SharpFileKinds.hpp"
#include "app/dap/DapPart.hpp"
#include "debug/DapSocket.hpp"
#include "debugger/RunState.hpp"
#include "sessions/LibraryTopics.hpp"
#include "typing/TypingTopics.hpp"

namespace sessions = calcu::sessions;

MachineController::MachineController(QObject* parent) : QObject(parent) {
    // The settings dialog reads and writes the product's file
    // through the session; a change the product applies at once.
    cu1600::SharpSessionOptions options;
    options.settings = &AppPaths::folders().settings();
    options.settingsChanged = [this](std::string_view section, std::string_view) {
        if (section == "serial") app().refreshSerialLinkDirectory();
    };
    sessions::HostServices services;
    services.offered = {sessions::HostService::AudioSink,     sessions::HostService::ClipboardText,
                        sessions::HostService::ClipboardImage, sessions::HostService::FilePicker,
                        sessions::HostService::FolderPicker,   sessions::HostService::KeepFile,
                        sessions::HostService::OpenUrl};
    m_session = std::make_unique<calcu::qt6::ProductHost<cu1600::SharpAppPart>>(
        cu1600::makeSharpSession(AppSettings::settings(), AppPaths::paths(), std::move(options)), std::move(services));
    host().addMessageHandler([this](const sessions::Message& m) { onMessage(m); });
    connect(&host(), &calcu::qt6::SessionHost::called, this, [this] {
        const auto t = host().now();
        if (m_calls++ > 0)
            m_longestGapMs = std::max(m_longestGapMs, std::chrono::duration<double, std::milli>(t - m_lastCall).count());
        m_lastCall = t;
    });
    // The DAP server's socket; the part listens when the settings
    // or --dap say so.
    if (cu1600::dap::DapPart* dap = app().dapPart()) m_dapSocket = std::make_unique<DapSocket>(*dap);
}

MachineController::~MachineController() {
    // The DAP's socket goes first: no message reaches the part after it.
    if (m_dapSocket && app().dapPart()) app().dapPart()->setTransport(nullptr);
    m_dapSocket.reset();
    // The product quits: what a medium still has to save is written (the
    // parts' `closing`). No message goes out after it.
    m_session.reset();
}

void MachineController::start() {
    m_session->start();
    if (m_dapSocket) app().dapPart()->setTransport(m_dapSocket.get());
}

void MachineController::send(const sessions::CommandName& name, sessions::Value args, Reply done) {
    host().send(name, std::move(args), std::move(done));
}

sessions::ReplyMessage MachineController::ask(const sessions::RequestName& name, sessions::Value args) {
    return m_session->ask(name, std::move(args));
}

bool MachineController::busy() const {
    const auto* b = state<sessions::BusyState>(sessions::kBusyTopic);
    return b && b->operationRuns;
}

void MachineController::pressKey(const std::string& name) {
    send(calcu::typing::kScreenKeyDown, sessions::Value::object({{"key", name}, {"pointer", 0}}));
}
void MachineController::releaseKey(const std::string& name) {
    send(calcu::typing::kScreenKeyUp, sessions::Value::object({{"key", name}, {"pointer", 0}}));
}
void MachineController::hostKeyDown(const calcu::typing::HostKeyEvent& e) {
    send(calcu::typing::kHostKeyDown, calcu::typing::toValue(e));
}
void MachineController::hostKeyUp(const std::string& physical) {
    send(calcu::typing::kHostKeyUp, sessions::Value::object({{"physical", physical}}));
}
void MachineController::hostText(const std::string& utf8) {
    send(calcu::typing::kHostText, sessions::Value::object({{"text", utf8}}));
}
void MachineController::focusLost() { send(calcu::typing::kFocusLost); }
void MachineController::pasteText(const std::string& text) {
    send(calcu::typing::kPaste, sessions::Value::object({{"text", text}}));
}
void MachineController::typeCommand(const std::string& line) {
    app().typingPart().engine().typeText(line, /*enter=*/true, /*refuseUnknown=*/false, calcu::typing::Origin::Typed);
}
void MachineController::cancelPaste() { app().typingPart().engine().focusLost(); }

void MachineController::attachAudio(cu1600::AudioSink& sink) {
    m_sound = std::make_unique<cu1600::SoundFeed>(sink, [](const std::string& text) {
        qWarning().noquote() << QString::fromStdString(text);
    });
    m_sound->warmUp(sharp::kSoundRate);
}

void MachineController::onMessage(const sessions::Message& m) {
    if (const auto* s = std::get_if<sessions::StateMessage>(&m)) {
        if (s->topic == sessions::kBusyTopic) {
            const bool now = busy();
            if (now != m_busy) {
                m_busy = now;
                emit appBusyChanged(now);
            }
        } else if (s->topic == calcu::debugger::kRunStateTopic) {
            emit runStateChanged();
        } else if (s->topic == calcu::debugger::kTraceTopic) {
            emit traceStateChanged();
        } else if (s->topic == cu1600::topics::kSerialLinks) {
            const auto* p = state<cu1600::SerialLinksPayload>(s->topic);
            if (p && p->moves != m_serialMoves) {
                m_serialMoves = p->moves;
                emit serialLinksMoved();
            }
        }
        return;
    }
    if (const auto* st = std::get_if<sessions::StreamMessage>(&m)) {
        for (const auto& p : st->appended)
            if (const auto* o = sessions::payloadAs<cu1600::ProgramOpenedPayload>(p))
                // Queued: the flow's dialogs open after the message is handed on.
                QTimer::singleShot(0, this, [this, name = QString::fromStdString(o->name),
                                             bytes = QByteArray(reinterpret_cast<const char*>(o->bytes.data()),
                                                                static_cast<qsizetype>(o->bytes.size())),
                                             kind = QString::fromStdString(o->kind)] { emit programOpened(name, bytes, kind); });
        return;
    }
    if (m_sound && !std::holds_alternative<sessions::ReplyMessage>(m) && !std::holds_alternative<sessions::ServiceAsk>(m))
        m_sound->deliver(m);   // SoundStart, SoundSamples, SoundStop
}

const calcu::machine::DisplayOutput* MachineController::display() const {
    const auto* d = state<sessions::DisplayState>(sessions::kDisplayTopic);
    return d ? &d->output : nullptr;
}

GrayImage MachineController::currentScreenImage() const {
    const auto* d = display();
    return d ? app().screenImage(*d) : GrayImage{};
}

bool MachineController::lcdText(LcdText* out) const {
    const auto* d = display();
    return d && app().lcdText(*d, out);
}

bool MachineController::machinePaused() const {
    const auto* s = state<calcu::debugger::RunState>(calcu::debugger::kRunStateTopic);
    return s && s->paused;
}

cu1600::LampsPayload MachineController::lamps() const {
    const auto* p = state<cu1600::LampsPayload>(cu1600::topics::kLamps);
    return p ? *p : cu1600::LampsPayload{};
}

std::optional<std::string> MachineController::setDapRunPort(long port) {
    std::optional<std::string> refused;
    send(cu1600::dap::kDapPortCommand, sessions::Value::object({{"port", static_cast<std::int64_t>(port)}}),
         [&refused](const sessions::ReplyMessage& r) {
             if (!r.done) refused = r.reason;
         });
    return refused;   // the command is carried out at once
}

QString MachineController::dapStatus() const {
    const auto* s = state<cu1600::dap::DapStatus>(cu1600::dap::kDapTopic);
    return s ? QString::fromStdString(s->status) : tr("Off");
}

bool MachineController::dapClient() const {
    const auto* s = state<cu1600::dap::DapStatus>(cu1600::dap::kDapTopic);
    return s && s->client;
}

void MachineController::disconnectDap() { send(cu1600::dap::kDapDisconnectCommand); }
