#include "EmulatorController.hpp"
#include "AppPaths.hpp"
#include "AppSettings.hpp"
#include "KeyIds.hpp"
#include "ModuleCatalog.hpp"
#include "QtResourceBundle.hpp"
#include <QMessageBox>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

namespace {

QString qs(const std::string& s) { return QString::fromStdString(s); }

std::optional<std::string> readResource(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return std::nullopt;
    return f.readAll().toStdString();
}

std::optional<std::vector<uint8_t>> readBytes(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return std::nullopt;
    QByteArray b = f.readAll();
    return std::vector<uint8_t>(b.begin(), b.end());
}

bool writeText(const QString& path, const std::string& text) {
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(text.data(), static_cast<qint64>(text.size()));
    return f.commit();
}

} // namespace

EmulatorController::EmulatorController(QObject* parent) : QObject(parent) {
    // The ROM lookup first (LIB-12): without the full set, romsMissing() and no controller.
    m_c = cu59::SessionController::create(static_cast<Platform&>(*this), QtResourceBundle(),
                                          readResource(QStringLiteral(":/roms/cuecards.txt")).value_or(""),
                                          cu59::kStartModuleID);
    if (!m_c) return;
    m_frameTimer.setTimerType(Qt::PreciseTimer);
    m_frameTimer.setInterval(1000 / 60);
    connect(&m_frameTimer, &QTimer::timeout, this, [this] { m_c->tick(std::chrono::steady_clock::now()); });
    m_frameTimer.start();
}

EmulatorController::~EmulatorController() { m_c.reset(); }

QVector<cu59::ModuleMetadata> EmulatorController::allModules() const {
    auto all = m_c->allModules();
    return QVector<cu59::ModuleMetadata>(all.begin(), all.end());
}

void EmulatorController::pressKey(int row, int col) { m_c->pressKey(cu59::matrixCode(row, col)); }
void EmulatorController::releaseKey(int row, int col) { m_c->releaseKey(cu59::matrixCode(row, col)); }

void EmulatorController::fitCard(const QString& path) {
    m_c->fitCard(readBytes(path), path.toStdString(), QFileInfo(path).fileName().toStdString());
}

void EmulatorController::fitBlankCard(const QString& savePath) {
    m_c->fitBlankCard(savePath.toStdString(), QFileInfo(savePath).fileName().toStdString());
}

void EmulatorController::loadStateFile(const QString& path) { m_c->loadStateFile(readBytes(path)); }

cu59::SessionController::DropAction EmulatorController::dropAction(const QString& path) const {
    return m_c->dropAction(QFileInfo(path).fileName().toStdString());
}

void EmulatorController::openDroppedFile(const QString& path) {
    m_c->openDroppedFile(readBytes(path), path.toStdString(), QFileInfo(path).fileName().toStdString());
}

void EmulatorController::loadAsmFile(const QString& path) {
    m_c->loadAsm(readResource(path), QFileInfo(path).fileName().toStdString());
}

// ── SessionController::Platform ──────────────────────────────────────────────

void EmulatorController::runOnUiThread(std::function<void()> job) {
    QMetaObject::invokeMethod(this, std::move(job), Qt::QueuedConnection);
}

std::optional<std::string> EmulatorController::readConstantMemory() { return readResource(AppPaths::constantMemoryFile()); }

void EmulatorController::writeConstantMemory(const std::string& text) { writeText(AppPaths::constantMemoryFile(), text); }

bool EmulatorController::writeCard(const std::string& path, const std::string& text) { return writeText(qs(path), text); }

std::string EmulatorController::cardFilePath(const std::string& fileName) {
    return (AppPaths::cardDir() + QLatin1Char('/') + qs(fileName)).toStdString();
}

EmulatorController::CardRead EmulatorController::readCard(const std::string& path) {
    if (!QFileInfo::exists(qs(path))) return {};
    auto raw = readBytes(qs(path));
    const bool unreadable = !raw.has_value();
    return {std::move(raw), unreadable};
}

cu59::SessionController::TraceLocation EmulatorController::traceLocation() {
    return {AppPaths::toFsPath(AppPaths::traceDir()), static_cast<uint64_t>(AppSettings::settings().traceMaxFileSizeMB()) * 1'000'000};
}

void EmulatorController::saveLastUsedModel(cu59::Model model) { AppSettings::settings().setLastUsedModel(model); }

// Adapter A-6 (LIB-9 design §5, until M-33): the module is not remembered
// (AC-22, behaviour change 59-9).
void EmulatorController::saveModuleID(const std::string&) {}

void EmulatorController::error(const std::string& title, const std::string& message) {
    emit errorOccurred(qs(title), qs(message));
}

// LIB-12 AC-5, AC-10 (design D-8, until LIB-14 and C59-8): a dialog with the
// lookup's text; main() then ends the app with exit code 1.
void EmulatorController::romsMissing(const calcu::roms::RomsMissing& missing) {
    QMessageBox::critical(nullptr, QStringLiteral("Calc-U-59: ROM files missing"),
                          qs("Calc-U-59 cannot start.\n\n" + calcu::roms::describe(missing)));
}

void EmulatorController::diagnostic(const std::string& message) { qDebug("%s", message.c_str()); }

void EmulatorController::changed(uint32_t c) {
    using S = cu59::SessionController;
    if (c & S::PaperCut) m_printerLines.clear();
    if (c & S::PrinterChanged) {
        // Mirror the text lines in Qt types: append what's new.
        const auto& lines = m_c->printerLines();
        for (size_t i = static_cast<size_t>(m_printerLines.size()); i < lines.size(); ++i) m_printerLines << qs(lines[i]);
    }
    if (c & S::ModelChanged) emit modelChanged();
    if (c & S::DisplayChanged) emit displayChanged();
    if (c & S::CueCardChanged) emit cueCardChanged();
    if (c & (S::PrinterChanged | S::PaperCut)) emit printerChanged();
    if (c & (S::CardChanged | S::ModelChanged)) emit cardStateChanged();
    if (c & S::RunStateChanged) emit runStateChanged();
    if (c & (S::LiveChanged | S::ListingChanged)) emit liveSnapshotChanged();
    if (c & S::CpuChanged) emit cpuChanged();
    if (c & S::HeatmapChanged) emit heatmapChanged();
    if (c & S::LogChanged) emit logChanged();
    if (c & S::AsmChanged) emit asmChanged();
    if (c & S::TraceChanged) emit traceChanged();
}
