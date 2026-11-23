#include "typing/TypingEngine.hpp"
#include "sessions/LibraryTopics.hpp"
#include "sessions/MachineSession.hpp"
#include "typing/TypingSupport.hpp"
#include <algorithm>

namespace calcu::typing {

using machine::KeyId;
using machine::Ticks;
using sessions::kNever;

std::u32string decodeUtf8(std::string_view s) {
    std::u32string out;
    for (std::size_t i = 0; i < s.size();) {
        const auto b = static_cast<unsigned char>(s[i]);
        std::size_t n = 0;
        char32_t cp = 0;
        if (b < 0x80) cp = b;
        else if ((b & 0xE0) == 0xC0) n = 1, cp = b & 0x1F;
        else if ((b & 0xF0) == 0xE0) n = 2, cp = b & 0x0F;
        else if ((b & 0xF8) == 0xF0) n = 3, cp = b & 0x07;
        else {
            ++i;   // a malformed byte
            continue;
        }
        bool ok = i + n < s.size();
        for (std::size_t k = 1; ok && k <= n; ++k) {
            const auto c = static_cast<unsigned char>(s[i + k]);
            ok = (c & 0xC0) == 0x80;
            cp = (cp << 6) | (c & 0x3F);
        }
        if (!ok) {
            ++i;
            continue;
        }
        i += n + 1;
        out.push_back(cp);
    }
    return out;
}

std::string encodeUtf8(char32_t c) {
    std::string out;
    if (c < 0x80) out += static_cast<char>(c);
    else if (c < 0x800) {
        out += static_cast<char>(0xC0 | (c >> 6));
        out += static_cast<char>(0x80 | (c & 0x3F));
    } else if (c < 0x10000) {
        out += static_cast<char>(0xE0 | (c >> 12));
        out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (c & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (c >> 18));
        out += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (c & 0x3F));
    }
    return out;
}

namespace {

std::string hostHolder(const std::string& physical) { return "host:" + physical; }
std::string screenHolder(std::uint32_t pointer) { return "screen:" + std::to_string(pointer); }

bool removeHolder(std::vector<std::string>& hs, const std::string& h) {
    const auto it = std::find(hs.begin(), hs.end(), h);
    if (it == hs.end()) return false;
    hs.erase(it);
    return true;
}

bool has(const std::vector<std::string>& hs, const std::string& h) { return std::find(hs.begin(), hs.end(), h) != hs.end(); }

} // namespace

// ── Machine ─────────────────────────────────────────────────────────────

void TypingEngine::bind(sessions::MachineSession& ms, std::function<void()> resync) {
    ms_ = &ms;
    resync_ = std::move(resync);
}

void TypingEngine::setMachine(machine::Machine* m, const TypingProfile* p, const KeyboardMap* k) {
    // The keys of the old machine went with it; nothing waits.
    queue_.clear();
    down_.clear();
    gapUntil_ = 0;
    pollAt_ = kNever;
    settling_ = false;
    const std::vector<Token> open = open_;
    machine_ = m;
    profile_ = m ? p : nullptr;
    map_ = m ? k : nullptr;
    if (ms_) gen_ = ms_->generation();
    // A new machine runs unless the session says otherwise now: the paused
    // state of the old one (told at the last host call) is not carried over,
    // which matters inside an operation in virtual time, where no host call
    // comes between the build and the keys (the DAP's clean start).
    paused_ = ms_ && m ? ms_->paused() : false;
    for (Token t : open) finish(t, TypeResult{false, kStartedOver});
    heldChanged();
}

bool TypingEngine::current() {
    if (ms_ && ms_->generation() != gen_) {
        if (resync_) resync_();
        if (ms_->generation() != gen_) setMachine(nullptr, nullptr, nullptr);
    }
    return machine_ && profile_;
}

TypingSupport* TypingEngine::support() const {
    return machine_ ? machine::first<TypingSupport>(*machine_) : nullptr;
}

Ticks TypingEngine::effHold(Ticks h) const { return std::max(h, profile_ ? profile_->minHold : Ticks{0}); }

// ── Results and topics ──────────────────────────────────────────────────

void TypingEngine::finish(Token t, TypeResult r) {
    if (t == 0) return;
    const auto it = std::find(open_.begin(), open_.end(), t);
    if (it == open_.end()) return;
    open_.erase(it);
    results_[t] = r;
    if (onFinished_) onFinished_(t, r);
}

std::optional<TypeResult> TypingEngine::takeResult(Token t) {
    const auto it = results_.find(t);
    if (it == results_.end()) return std::nullopt;
    TypeResult r = std::move(it->second);
    results_.erase(it);
    return r;
}

std::vector<KeyId> TypingEngine::liveHeld() const {
    std::vector<KeyId> out;
    for (const Down& d : down_)
        if (d.live && (!d.liveSingle || !d.holders.empty()) &&
            std::find(out.begin(), out.end(), d.key) == out.end())
            out.push_back(d.key);
    return out;
}

std::vector<KeyId> TypingEngine::down() const {
    std::vector<KeyId> out;
    for (const Down& d : down_) out.push_back(d.key);
    return out;
}

void TypingEngine::heldChanged() {
    std::vector<KeyId> now = liveHeld();
    if (now == lastLive_) return;
    lastLive_ = std::move(now);
    if (onHeld_) onHeld_();
}

bool TypingEngine::idle() const { return down_.empty() && queue_.empty() && !settling_; }

// ── Rules ───────────────────────────────────────────────────────────────

// No earlier than its hold (at least the minimum), and on a TI profile not
// before the ROM has read it, at most maxHold.
bool TypingEngine::releaseDue(const Down& d, Ticks now) const {
    if (!d.releaseAsked || paused_ || !profile_) return false;
    if (now < d.at + effHold(d.hold)) return false;
    if (profile_->holdUntilSampled && d.samplesAt) {
        const SampleRule& r = *profile_->holdUntilSampled;
        if (now >= d.at + r.maxHold) return true;
        const TypingSupport* s = support();
        const auto n = s ? s->samples(d.key) : std::nullopt;
        if (n && *n - *d.samplesAt < r.samplesNeeded) return false;
    }
    return true;
}

// What the next press waits for: the gap after a release; for
// a live key, an engine press down (a press of a sequence, of text, of a
// command) or a key in its minimum hold after the user let go; for any other
// press, any key the engine holds down (it never overlaps keys the user did
// not hold together).
bool TypingEngine::blocked(Ticks now, bool forLive) const {
    if (now < gapUntil_ && !(forLive && paused_)) return true;   // paused: the gap passes in host time too
    if (!forLive) return !down_.empty();
    return std::any_of(down_.begin(), down_.end(), [](const Down& d) { return !d.liveSingle || d.releaseAsked; });
}

void TypingEngine::keyDown(const KeyId& k, Ticks now) {
    machine_->keyDown(k);
    if (observer_) observer_->keyChanged(k, true, now);
}

void TypingEngine::release(std::size_t i, Ticks now) {
    const Down d = down_[i];
    down_.erase(down_.begin() + static_cast<std::ptrdiff_t>(i));
    machine_->keyUp(d.key);
    if (observer_) observer_->keyChanged(d.key, false, now);
    gapUntil_ = std::max(gapUntil_, now + profile_->gapAfter(d.key));
    if (profile_->waitsForReadinessAfterRelease) settling_ = !paused_;
    if (d.last) finish(d.token, TypeResult{});
    heldChanged();
}

void TypingEngine::requestRelease(Down& d) {
    d.releaseAsked = true;
    if (!paused_ || !machine_) return;
    const Ticks t = machine_->tickCount();
    const Ticks end = d.at + effHold(d.hold);
    d.wallDue = wall_ + pacing::wallTimeOf(end > t ? end - t : 0, machine_->descriptor().tickRate);
}

// The front of the queue starts when nothing blocks it, the machine
// runs (a live key excepted), and it is ready where the profile says. True
// when something changed (a character expanded, a press started).
bool TypingEngine::startFront(Ticks now) {
    if (queue_.empty()) return false;
    Item& f = queue_.front();
    if (blocked(now, f.liveSingle)) return false;
    if (!f.liveSingle && paused_) return false;   // waits for the machine to run
    if (f.isChar) {   // decided now, with the latches of now
        const Item base = f;
        queue_.pop_front();
        std::optional<std::vector<Press>> keys;
        if (profile_->charKeys) keys = profile_->charKeys(base.ch, support());
        if (!keys || keys->empty()) {
            if (base.last) finish(base.token, TypeResult{});
            return true;
        }
        for (std::size_t i = keys->size(); i-- > 0;) {
            Item it = base;
            it.isChar = false;
            it.press = (*keys)[i];
            it.firstOfStep = base.firstOfStep && i == 0;
            it.atOnce = base.atOnce && i == 0;
            it.boundAt = i == 0 ? base.boundAt : std::nullopt;
            it.last = base.last && i + 1 == keys->size();
            queue_.push_front(std::move(it));
        }
        return true;
    }
    if (!f.frontSince) f.frontSince = now;
    if (waitsForReadiness(f, now) && !machine_->isKeyReady()) return false;
    Item it = std::move(f);
    queue_.pop_front();
    Down d;
    d.key = it.press.key;
    d.at = now;
    d.hold = it.hold ? *it.hold : it.liveSingle ? profile_->minHold : profile_->typed.hold;
    d.holders = it.holders;
    d.liveSingle = it.liveSingle;
    d.live = it.origin == Origin::Live;
    d.token = it.token;
    d.last = it.last;
    if (profile_->holdUntilSampled && !it.hold)
        if (const TypingSupport* s = support()) d.samplesAt = s->samples(d.key);
    keyDown(d.key, now);
    down_.push_back(std::move(d));
    if (down_.back().holders.empty()) requestRelease(down_.back());
    heldChanged();
    return true;
}

// Whether the press, not blocked, still needs key readiness to start: a
// live key never; the first press of a step by its rule (and its
// bound); any other by queuedWaitsForReadiness.
bool TypingEngine::waitsForReadiness(const Item& f, Ticks now) const {
    if (f.liveSingle || f.isChar) return false;
    bool ready = profile_->queuedWaitsForReadiness;
    if (f.firstOfStep) {
        const StartRule r = f.atOnce ? StartRule::AtOnce : profile_->firstPressOfStep;
        if (r == StartRule::AtOnce) ready = false;
        else if (r == StartRule::KeyReadiness) ready = true;
    }
    if (ready && f.boundAt && now >= *f.boundAt) ready = false;   // the bound passed: all the same
    if (ready && f.frontSince && doesNotWait(f) && now >= *f.frontSince + profile_->doesNotWaitAfter) ready = false;
    return ready;
}

// A preset's keys are excepted: its items run in order, and there R/S
// continues a program once it has stopped.
bool TypingEngine::doesNotWait(const Item& f) const {
    return f.origin == Origin::Typed && !f.isChar && profile_->doesNotWait && profile_->doesNotWait(f.press.key);
}

// Something waits for the machine (readiness, a key read, readiness again):
// the engine looks again every readinessPoll ticks from the tick it last
// acted at, so where the host cut its slices does not matter.
bool TypingEngine::polling(Ticks now) const {
    if (paused_) return false;
    if (settling_) return true;
    for (const Down& d : down_)
        if (d.releaseAsked && profile_->holdUntilSampled && now >= d.at + effHold(d.hold)) return true;
    return !queue_.empty() && !blocked(now, queue_.front().liveSingle) && waitsForReadiness(queue_.front(), now);
}

void TypingEngine::run(Ticks now) {
    if (!machine_ || !profile_) return;
    runOnce(now);
    if (!polling(now)) pollAt_ = kNever;
    else if (pollAt_ <= now || pollAt_ == kNever) pollAt_ = now + std::max<Ticks>(1, profile_->readinessPoll);
}

void TypingEngine::runOnce(Ticks now) {
    // The pause as the session has it now, not as the last host call told
    // it: a caller that pauses the machine and types in the same call (a
    // debugger's set-up) must not see a press go down on the paused machine,
    // whose hold would then pass in host time, unread by the ROM.
    if (ms_ && machine_ && ms_->paused() != paused_) notePaused(ms_->paused());
    if (paused_) {
        for (std::size_t i = 0; i < down_.size();) {
            const Down& d = down_[i];
            if (d.releaseAsked && (virtual_ || (d.wallDue && wall_ >= *d.wallDue))) release(i, now);
            else ++i;
        }
        settling_ = false;
    } else {
        for (std::size_t i = 0; i < down_.size();) {
            if (releaseDue(down_[i], now)) release(i, now);
            else ++i;
        }
        if (settling_ && machine_->isKeyReady()) settling_ = false;
    }
    for (int guard = 0; guard < 10'000 && startFront(now); ++guard) {
    }
}

// What waits goes, but the presses that must finish; live keys
// that wait stay unless the focus went.
void TypingEngine::dropWaiting(const char* reason, bool keepLiveSingles) {
    std::vector<Token> dropped;
    std::deque<Item> kept;
    for (Item& it : queue_) {
        if (it.press.mustFinish && !it.isChar) kept.push_back(std::move(it));
        else if (keepLiveSingles && it.liveSingle) kept.push_back(std::move(it));
        else if (it.token) dropped.push_back(it.token);
    }
    queue_ = std::move(kept);
    // A token that lost presses is not done, now; what is left of it (a
    // press down, a press that must finish) runs on untracked (finish once).
    for (Token t : dropped) finish(t, TypeResult{false, reason});
}

// ── Live input ──────────────────────────────────────────────────────────

void TypingEngine::liveDown(const KeyId& key, const std::string& holder) {
    const Ticks t = now();
    dropWaiting(kStoppedByKey, true);
    // Another holder of a key the user still holds.
    for (Down& d : down_)
        if (d.key == key && d.liveSingle && !d.releaseAsked) {
            if (!has(d.holders, holder)) d.holders.push_back(holder);
            return;
        }
    for (Item& it : queue_)
        if (it.liveSingle && it.press.key == key && !it.holders.empty()) {
            if (!has(it.holders, holder)) it.holders.push_back(holder);
            return;
        }
    Item it;
    it.press = Press{key};
    it.origin = Origin::Live;
    it.liveSingle = true;
    it.holders = {holder};
    queue_.push_back(std::move(it));   // at once when nothing blocks, else right after
    run(t);
}

void TypingEngine::liveUp(const std::string& holder) {
    const Ticks t = now();
    for (Down& d : down_)
        if (removeHolder(d.holders, holder)) {   // down until its last holder lets go
            if (d.holders.empty()) requestRelease(d);
            heldChanged();
            run(t);
            return;
        }
    for (Item& it : queue_)
        if (it.liveSingle && removeHolder(it.holders, holder)) return;   // one press of the minimum
    // A key-up that holds nothing does nothing.
}

void TypingEngine::queueLiveSequence(const std::vector<Press>& presses) {
    for (const Press& p : presses) {
        Item it;
        it.press = p;
        it.origin = Origin::Live;
        queue_.push_back(std::move(it));
    }
}

void TypingEngine::hostKeyDown(const HostKeyEvent& e) {
    if (e.autoRepeat || !current() || !map_) return;   // the host's repeat is ignored
    const std::optional<Mapped> m = map_->map(e);
    if (!m || m->presses.empty()) return;   // no key: nothing typed, nothing waits
    if (m->held && m->presses.size() == 1) {
        liveDown(m->presses.front().key, hostHolder(e.physical));
        return;
    }
    dropWaiting(kStoppedByKey, true);
    queueLiveSequence(m->presses);
    run(now());
}

void TypingEngine::hostKeyUp(const std::string& physical) {
    if (!current()) return;
    liveUp(hostHolder(physical));
}

void TypingEngine::hostText(std::u32string_view text) {
    if (!current() || !map_ || text.empty()) return;
    dropWaiting(kStoppedByKey, true);
    for (char32_t c : text) {   // each a press of the minimum hold
        HostKeyEvent e;
        e.produced = c;
        const std::optional<Mapped> m = map_->map(e);
        if (!m || m->presses.empty()) continue;
        if (m->held && m->presses.size() == 1) {
            Item it;
            it.press = m->presses.front();
            it.origin = Origin::Live;
            it.liveSingle = true;
            queue_.push_back(std::move(it));
        } else {
            queueLiveSequence(m->presses);
        }
    }
    run(now());
}

void TypingEngine::screenKeyDown(const KeyId& key, std::uint32_t pointer) {
    if (!current()) return;
    liveDown(key, screenHolder(pointer));
}

void TypingEngine::screenKeyUp(const KeyId&, std::uint32_t pointer) {
    if (!current()) return;
    liveUp(screenHolder(pointer));
}

void TypingEngine::focusLost() {   // keys keep their minimum hold, so no press is too short for the ROM
    if (!current()) return;
    for (Down& d : down_)
        if (!d.holders.empty()) {
            d.holders.clear();
            requestRelease(d);
        }
    dropWaiting(kFocusWasLost, false);
    heldChanged();
    run(now());
}

// ── Typed text and commands ─────────────────────────────────────────────

Queued TypingEngine::typeKeys(std::vector<Press> presses, Origin origin, bool atOnce, std::optional<Ticks> hold) {
    if (!current()) return Queued{0, std::string(sessions::kNoMachine)};
    const Token t = nextToken();
    open_.push_back(t);
    if (presses.empty()) {
        finish(t, TypeResult{});
        return Queued{t, std::nullopt};
    }
    const Ticks n = now();
    for (std::size_t i = 0; i < presses.size(); ++i) {
        Item it;
        it.press = presses[i];
        it.origin = origin;
        it.token = t;
        it.firstOfStep = i == 0;
        it.atOnce = atOnce && i == 0;
        if (i == 0 && !atOnce && profile_->firstPressOfStep == StartRule::KeyReadiness && profile_->firstPressBound)
            it.boundAt = n + *profile_->firstPressBound;
        it.hold = hold;
        it.last = i + 1 == presses.size();
        queue_.push_back(std::move(it));
    }
    run(n);
    return Queued{t, std::nullopt};
}

Queued TypingEngine::typeText(std::string_view utf8, bool enter, bool refuseUnknown, Origin origin, bool atOnce) {
    if (!current()) return Queued{0, std::string(sessions::kNoMachine)};
    if (!profile_->charKeys) return Queued{0, std::string("This model types no text.")};
    const std::u32string text = decodeUtf8(utf8);
    if (refuseUnknown)
        for (char32_t c : text)
            if (!profile_->charKeys(c, nullptr)) {   // the same answer whatever is latched
                const std::string shown =
                    c < 0x20 || c == 0x7F ? "U+" + std::to_string(static_cast<std::uint32_t>(c)) : encodeUtf8(c);
                return Queued{0, "No key types the character \"" + shown + "\"."};
            }
    const Token t = nextToken();
    open_.push_back(t);
    const Ticks n = now();
    std::size_t count = text.size() + (enter && profile_->enter ? 1 : 0);
    if (count == 0) {
        finish(t, TypeResult{});
        return Queued{t, std::nullopt};
    }
    std::size_t i = 0;
    auto add = [&](Item it) {
        it.origin = origin;
        it.token = t;
        it.firstOfStep = i == 0;
        it.atOnce = atOnce && i == 0;
        if (i == 0 && !atOnce && profile_->firstPressOfStep == StartRule::KeyReadiness && profile_->firstPressBound)
            it.boundAt = n + *profile_->firstPressBound;
        it.last = i + 1 == count;
        ++i;
        queue_.push_back(std::move(it));
    };
    for (char32_t c : text) {
        Item it;
        it.isChar = true;
        it.ch = c;
        add(std::move(it));
    }
    if (enter && profile_->enter) {
        Item it;
        it.press = Press{*profile_->enter};
        add(std::move(it));
    }
    run(n);
    return Queued{t, std::nullopt};
}

void TypingEngine::paste(std::string_view utf8) {
    if (!current() || !profile_->charKeys) return;
    const std::string_view line = utf8.substr(0, utf8.find_first_of("\r\n"));   // the first line
    for (char32_t c : decodeUtf8(line)) {
        if (c < 0x20 || c == 0x7F) continue;
        if (!profile_->charKeys(c, nullptr)) continue;   // left out
        Item it;
        it.isChar = true;
        it.ch = c;
        it.origin = Origin::Typed;
        queue_.push_back(std::move(it));   // after what is typed
    }
    run(now());
}

// ── Reset, pause, the session ───────────────────────────────────────────

void TypingEngine::drop() {
    if (!current()) return;
    const Ticks t = now();
    // Up at once, without the minimum: the machine starts over.
    for (const Down& d : down_) {
        machine_->keyUp(d.key);
        if (observer_) observer_->keyChanged(d.key, false, t);
    }
    down_.clear();
    queue_.clear();
    gapUntil_ = 0;
    pollAt_ = kNever;
    settling_ = false;
    const std::vector<Token> open = open_;
    for (Token tok : open) finish(tok, TypeResult{false, kStartedOver});
    heldChanged();
}

void TypingEngine::setPaused(bool p, pacing::WallTime now) {
    wall_ = now;
    if (p == paused_) return;
    notePaused(p);
    if (current()) run(this->now());
}

void TypingEngine::notePaused(bool p) {
    paused_ = p;
    for (Down& d : down_) {
        d.wallDue.reset();
        if (p && d.releaseAsked) requestRelease(d);
    }
}

void TypingEngine::hostCall(pacing::WallTime now) {
    wall_ = now;
    if (paused_ && current()) run(this->now());
}

Ticks TypingEngine::nextDue(Ticks now) const {
    if (!machine_ || !profile_ || paused_) return kNever;
    Ticks due = pollAt_;
    for (const Down& d : down_) {
        if (!d.releaseAsked) continue;
        const Ticks end = d.at + effHold(d.hold);
        if (end > now) due = std::min(due, end);
        else if (profile_->holdUntilSampled)   // the first look at the end of the hold, then every poll
            due = std::min(due, pollAt_ == kNever ? now : std::max(now, d.at + profile_->holdUntilSampled->maxHold));
        else due = std::min(due, now);
    }
    if (!queue_.empty()) {
        const Item& f = queue_.front();
        if (now < gapUntil_) due = std::min(due, gapUntil_);
        else if (!blocked(now, f.liveSingle) && waitsForReadiness(f, now)) {
            if (pollAt_ == kNever) due = std::min(due, now);   // the first look once nothing blocks it
            if (f.boundAt) due = std::min(due, std::max(now, *f.boundAt));
            if (f.frontSince && doesNotWait(f)) due = std::min(due, std::max(now, *f.frontSince + profile_->doesNotWaitAfter));
        } else if (!blocked(now, f.liveSingle)) {
            due = std::min(due, now);
        }
    }
    return due;
}

void TypingEngine::act(machine::Machine&, Ticks now, pacing::WallTime) {
    if (!current()) return;
    run(now);
}

} // namespace calcu::typing
