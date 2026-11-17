#include "TIMachineState.hpp"
#include "TIMachine.hpp"
#include "TIStateRecords.hpp"

template <class Io>
void TIMachine::stateIo(Io& io) {
    io.begin("TIM ");
    io(m_ticksBeforeReset)(m_retired);
    bool restorePending = m_restore.has_value();
    std::vector<std::uint8_t> savedRam = m_restore ? m_restore->ram : std::vector<std::uint8_t>{};
    int savedPartition = m_restore ? m_restore->partitionProgramRegs : 0;
    io(restorePending)(savedRam)(savedPartition);
    if constexpr (Io::kRestoring) {
        if (restorePending) m_restore = SavedMemory{std::move(savedRam), savedPartition};
        else m_restore.reset();
    }
    // What is plugged: the library's header says the same, checked here too.
    bool printer = m_printer != nullptr, module = m_module != nullptr, reader = m_cardReader != nullptr;
    const bool wasPrinter = printer, wasModule = module, wasReader = reader;
    io(printer)(module)(reader);
    if constexpr (Io::kRestoring) {
        if (printer != wasPrinter || module != wasModule || reader != wasReader)
            io.fail("the state was saved with other peripherals plugged in");
    }
    io.end();
    m_hw.stateIo(io);
    if (m_printer) m_printer->stateIo(io);
    if (m_module) m_module->stateIo(io);
    if (m_cardReader) m_cardReader->stateIo(io);
}

std::vector<std::uint8_t> TIMachine::saveState() {
    cu59::StateWriter w;
    stateIo(w);
    return w.take();
}

std::optional<std::string> TIMachine::restoreState(const std::vector<std::uint8_t>& bytes) {
    const std::vector<std::uint8_t> before = saveState();
    cu59::StateReader r(bytes);
    stateIo(r);
    if (r.ok() && !r.atEnd()) r.fail("the state holds more than this machine has");
    if (r.ok()) {
        applyTraceFlags();
        return std::nullopt;
    }
    const std::string why = r.error();
    cu59::StateReader back(before);
    stateIo(back);
    return "The state does not fit this machine: " + why + ".";
}

std::vector<std::uint8_t> TIMachineState::save() const { return m_.saveState(); }

std::optional<std::string> TIMachineState::restore(const std::vector<std::uint8_t>& bytes) { return m_.restoreState(bytes); }
