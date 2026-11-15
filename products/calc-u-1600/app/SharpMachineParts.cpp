#include "app/SharpMachineParts.hpp"

#include <algorithm>

#include "Machine/LegacyCalls.hpp"
#include "Machine/SharpCatalogue.hpp"
#include "Machine/SharpProductKinds.hpp"
#include "PC1500/PC1500TypedInput.hpp"
#include "PC1600/PC1600TypedInput.hpp"

namespace shared {

using calcu::machine::KeyId;
using calcu::machine::Machine;
using calcu::machine::Ticks;

namespace {

// The key ID of a key name of the typing helpers: they give a letter as its
// capital (PC1500TypedInput.hpp, PC1600TypedInput.hpp); the machine takes
// exactly its key IDs (C1600-1 AC-9, EC-1).
KeyId keyIdOf(const std::string& name) {
    if (name.size() == 1 && name[0] >= 'A' && name[0] <= 'Z') return KeyId{std::string(1, static_cast<char>(name[0] - 'A' + 'a'))};
    return KeyId{name};
}

// releaseKey()'s minimum hold for a frontend's key: a press of one frame is
// scanned on both models; two leave a margin without making a fast typist's
// keys overlap.
constexpr int kMinKeyHoldFrames = 2;

// tapShiftedKey(): 30 ms shift hold, 100 ms gap, 30 ms base-key hold.
constexpr double kTapHoldSeconds = 0.030;
constexpr double kTapGapSeconds = 0.100;

} // namespace

SharpTypingHook::SharpTypingHook(calcu::sessions::MachineSession& ms, std::function<bool()> busy)
    : m_ms(ms), m_busy(std::move(busy)) {}

bool SharpTypingHook::isPC1600() const {
    const Machine* m = machine();
    return m && m->descriptor().model.value == sharp::kPC1600;
}

double SharpTypingHook::clockHz() const {
    const Machine* m = machine();
    return m ? static_cast<double>(m->descriptor().tickRate) : 1.0;
}

Ticks SharpTypingHook::now() const {
    const Machine* m = machine();
    return m ? m->tickCount() : 0;
}

Ticks SharpTypingHook::frameTicks() const { return std::max<Ticks>(1, static_cast<Ticks>(clockHz() / 60.0)); }

Ticks SharpTypingHook::minKeyHoldTicks() const { return static_cast<Ticks>(kMinKeyHoldFrames * clockHz() / 60.0); }

// ---- SliceHook ----

Ticks SharpTypingHook::nextDue(Ticks) const {
    Ticks due = calcu::sessions::kNever;
    if (m_paste.active()) due = m_nextFrame;
    if (!m_timedKeys.empty()) due = std::min(due, m_timedKeys.front().due);
    return due;
}

void SharpTypingHook::act(Machine&, Ticks t, calcu::pacing::WallTime) {
    if (m_paste.active() && t >= m_nextFrame) {
        // The frame boundaries stay on the grid the paste started on.
        while (m_nextFrame <= t) m_nextFrame += frameTicks();
        pasteOnFrame();
    }
    fireDueKeys(t);
}

void SharpTypingHook::machineGone() {
    m_paste.cancel({});   // the machine it was typing into is going away
    m_liveTyping = false;
    m_timedKeys.clear();
    m_keyPressedAt.clear();
}

// ---- Keys ----

void SharpTypingHook::machinePressKey(const std::string& name) {
    if (Machine* m = machine()) m->keyDown(keyIdOf(name));   // a key the model lacks changes nothing (C1600-1 EC-1)
}

void SharpTypingHook::machineReleaseKey(const std::string& name) {
    if (Machine* m = machine()) m->keyUp(keyIdOf(name));
}

void SharpTypingHook::pressKey(const std::string& name) {
    if (userKeysDropped()) return;
    // A release still to come (a fast typist): this press waits for it and a
    // gap, so two keys never overlap and a double tap stays two taps.
    if (!m_timedKeys.empty()) {
        const Ticks at = m_timedKeys.back().due + minKeyHoldTicks();
        m_keyPressedAt[name] = at;
        scheduleKeyAt(at, true, name);
        return;
    }
    m_keyPressedAt[name] = now();
    machinePressKey(name);
}

void SharpTypingHook::releaseKey(const std::string& name) {
    const auto it = m_keyPressedAt.find(name);
    if (it != m_keyPressedAt.end()) {
        const Ticks holdUntil = it->second + minKeyHoldTicks();
        m_keyPressedAt.erase(it);
        if (now() < holdUntil) {
            scheduleKeyAt(holdUntil, false, name);
            return;
        }
    }
    machineReleaseKey(name);
}

void SharpTypingHook::setOnKeyPressed(bool pressed) {
    if (!pressed) machineReleaseKey("on");
    else if (!userKeysDropped()) machinePressKey("on");
}

void SharpTypingHook::scheduleKey(double afterSeconds, bool press, const std::string& name) {
    scheduleKeyAt(now() + static_cast<Ticks>(afterSeconds * clockHz()), press, name);
}

void SharpTypingHook::scheduleKeyAt(Ticks due, bool press, const std::string& name) {
    auto it = std::upper_bound(m_timedKeys.begin(), m_timedKeys.end(), due,
                               [](Ticks d, const TimedKey& k) { return d < k.due; });
    m_timedKeys.insert(it, TimedKey{due, press, name});
}

void SharpTypingHook::fireDueKeys(Ticks t) {
    while (!m_timedKeys.empty() && m_timedKeys.front().due <= t) {
        const TimedKey key = m_timedKeys.front();
        m_timedKeys.pop_front();
        if (key.press) machinePressKey(key.name);
        else machineReleaseKey(key.name);
    }
}

void SharpTypingHook::tapShiftedKey(const std::string& baseName) {
    if (userKeysDropped()) return;
    machinePressKey("shift");
    scheduleKey(kTapHoldSeconds, false, "shift");
    scheduleKey(kTapHoldSeconds + kTapGapSeconds, true, baseName);
    scheduleKey(kTapHoldSeconds + kTapGapSeconds + kTapHoldSeconds, false, baseName);
}

void SharpTypingHook::tapKey(const std::string& name) {
    Machine* m = machine();
    if (!m || userKeysDropped()) return;
    if (auto* queue = sharp::firstOf<sharp::KeyQueue>(*m)) {   // the PC-1500's
        queue->enqueue(keyIdOf(name).value);
    } else {
        machinePressKey(name);
        scheduleKey(kTapHoldSeconds, false, name);
    }
}

void SharpTypingHook::enqueueKey(const std::string& name) {
    // PC-1600 buffering is out of scope: ignored there.
    if (Machine* m = machine())
        if (auto* queue = sharp::firstOf<sharp::KeyQueue>(*m)) queue->enqueue(keyIdOf(name).value);
}

void SharpTypingHook::enqueueShiftedKey(const std::string& baseName) {
    enqueueKey("shift");
    enqueueKey(baseName);
}

bool SharpTypingHook::hostKeyDown(const ResolvedKey& key, bool physical) {
    if (userKeysDropped()) return false;
    // A real keystroke aborts a paste still typing.
    if (pasteActive()) interruptPaste();

    if (isPC1600()) {
        // An accented character is a KBII key sequence, and a key typed while
        // one still runs waits behind it: both through the frame-paced queue.
        if (key.needsKbii || liveTypingActive()) {
            typeLiveStep(PasteStep{key.baseKey, key.needsShift, key.needsKbii});
        } else if (key.needsShift) {
            tapShiftedKey(key.baseKey);
        } else if (physical) {
            pressKey(key.baseKey);
            return true;
        } else {
            tapKey(key.baseKey);
        }
        return false;
    }

    // PC-1500: cursor keys bypass the queue (the ROM's own auto-repeat);
    // everything else is queued so fast typing can't outrun the key scan.
    if (key.needsShift) {
        enqueueShiftedKey(key.baseKey);
    } else if (physical && key.isPc1500RepeatKey()) {
        pressKey(key.baseKey);
        return true;
    } else {
        enqueueKey(key.baseKey);
    }
    return false;
}

void SharpTypingHook::hostShiftTap() {
    if (userKeysDropped()) return;
    if (pasteActive()) interruptPaste();
    if (liveTypingActive()) typeLiveStep(PasteStep{"shift"});
    else tapKey("shift");
}

void SharpTypingHook::pasteText(const std::string& text) {
    if (userKeysDropped()) return;
    enqueueTyped(text, /*pressEnter=*/false);
}

void SharpTypingHook::typeCommand(const std::string& line) { enqueueTyped(line, /*pressEnter=*/true); }

void SharpTypingHook::startFrames() {
    if (!m_paste.active()) m_nextFrame = now() + frameTicks();
}

void SharpTypingHook::enqueueTyped(const std::string& text, bool pressEnter) {
    if (!machine()) return;
    startFrames();
    const bool pc1600 = isPC1600();
    std::vector<PasteStep> steps = pc1600 ? buildPasteSteps(text, pc1600ResolveTypedChar, pc1600ResolveKbiiChar)
                                          : buildPasteSteps(text, pc1500ResolveTypedChar);
    if (pressEnter) steps.push_back(PasteStep{"enter", false});
    m_paste.setPacing(pc1600 ? pc1600PastePacing() : pc1500PastePacing());
    m_paste.append(steps);
    m_liveTyping = false;
}

void SharpTypingHook::typeLiveStep(const PasteStep& step) {
    if (!isPC1600()) return;
    startFrames();
    m_paste.setPacing(pc1600PastePacing());
    m_paste.append({step});
    m_liveTyping = true;
}

void SharpTypingHook::cancelPaste() {
    m_paste.cancel([this](const std::string& key) { machineReleaseKey(key); });
    m_liveTyping = false;
}

void SharpTypingHook::interruptPaste() {
    m_paste.cancel([this](const std::string& key) { machineReleaseKey(key); }, /*finishKbii=*/true);
    m_liveTyping = m_paste.active();   // what's left is a KBII sequence's closing tap
}

void SharpTypingHook::pasteOnFrame() {
    m_paste.onFrame([this](const std::string& key) { machinePressKey(key); },
                    [this](const std::string& key) { machineReleaseKey(key); },
                    [this] { return machine() ? sharp::legacy::readLatches(*machine()) : KeyLatches{}; });
}

// ---- SharpDebuggerGate ----

Ticks SharpDebuggerGate::advance(Machine& m, Ticks atLeast, bool& stoppedEarly) {
    stoppedEarly = false;
    if (!m_debugger) return m.advance(atLeast);
    const Ticks before = m.tickCount();
    if (!m_debugger->runDebuggerSlice(atLeast)) return m.advance(atLeast);
    const Ticks ran = m.tickCount() - before;
    stoppedEarly = ran < atLeast;   // paused or stopped at a breakpoint: the call ends
    return ran;
}

} // namespace shared
