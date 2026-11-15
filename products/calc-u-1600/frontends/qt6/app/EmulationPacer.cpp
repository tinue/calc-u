#include "EmulationPacer.hpp"

#include <QTimer>
#include <utility>

#include "MachineController.hpp"
#include "sessions/LibraryTopics.hpp"

namespace {
// ~60 Hz.
constexpr int kFrameIntervalMs = 16;
} // namespace

EmulationPacer::EmulationPacer(MachineController* controller, std::function<void()> refreshViews, QObject* parent)
    : QObject(parent), m_controller(controller), m_refreshViews(std::move(refreshViews)) {
    m_timer = new QTimer(this);
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QTimer::timeout, this, &EmulationPacer::onTick);
    resume();
}

void EmulationPacer::suspend() { m_timer->stop(); }

void EmulationPacer::resume() {
    if (!m_frozen) m_timer->start(kFrameIntervalMs);
}

void EmulationPacer::setTurbo(bool active) {
    m_controller->send(calcu::sessions::kSpeedCommand, calcu::sessions::Value::object({{"full", active}}));
}

void EmulationPacer::setFrozen(bool frozen) {
    m_frozen = frozen;
    m_controller->setFrozen(frozen);
    if (frozen) {
        m_timer->stop();
        m_refreshViews();
        return;
    }
    resume();
}

void EmulationPacer::onTick() {
    m_controller->call();
    m_refreshViews();
}

void EmulationPacer::runFor(double seconds) {
    m_controller->runFor(seconds);
    m_refreshViews();
}

bool EmulationPacer::runUntilPasteDone(double capSeconds) {
    const bool done = m_controller->runUntilTyped(capSeconds);
    m_refreshViews();
    return done;
}
