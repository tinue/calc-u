#include "paper/PaperPart.hpp"
#include "machine/Names.hpp"
#include "paper/Picture.hpp"
#include "paper/Replies.hpp"
#include "presets/Engine.hpp"
#include "presets/Vocabulary.hpp"
#include "sessions/Args.hpp"
#include "sessions/MachineSession.hpp"
#include "startup/MachineControl.hpp"
#include <algorithm>
#include <cctype>

namespace calcu::paper {

using namespace calcu::sessions;
using machine::PeripheralId;

namespace {

std::optional<std::string> ownerArg(const sessions::Value& args) {
    const std::string o = textArg(args, "owner");
    if (o.empty()) return std::nullopt;
    return o;
}

std::string joined(const std::vector<std::string>& lines) {
    std::string out;
    for (std::size_t i = 0; i < lines.size(); ++i) out += (i ? "\n" : "") + lines[i];
    return out;
}

} // namespace

PaperPart::PaperPart(const machine::Catalogue& cat, std::vector<PrinterPaper> printers,
                     const startup::MachineControl& control)
    : control_(control) {
    for (auto& p : printers) {
        const machine::PeripheralDescriptor* d = cat.peripheral(p.printer);
        Entry e;
        e.printer = p.printer;
        e.owner = d ? d->name : p.printer.value;
        e.paper = std::make_unique<Paper>(std::move(p.description));
        entries_.push_back(std::move(e));
    }
}

void PaperPart::attach(SessionSetup& s) {
    papersTopic_ = &s.stateTopic(kPapersTopic);
    auto list = std::make_shared<PaperList>();
    for (auto& e : entries_) {
        list->papers.push_back(PaperEntry{e.printer, e.owner, e.paper->description()});
        e.topic = &s.streamTopic(paperTopic(e.owner), History::SinceReset);
    }
    papersTopic_->set(list);
    copiesTopic_ = &s.streamTopic(kPaperCopiesTopic, History::None);

    // Copy: the picture and the text to the clipboard.
    s.command(kPaperCopyCommand, [this](const sessions::Value& a, SessionContext& c) -> CommandOutcome {
        sync(c);
        const Found f = find(c, ownerArg(a));
        if (!f.paper) return Refused{f.refusal};
        return copy(c, *entryOf(f.paper), false);
    });
    // Cut: copied, then cut, also when the copy did not succeed; the step
    // cut-paper: without a copy (`copy: false`), also an empty paper.
    s.command(kPaperCutCommand, [this](const sessions::Value& a, SessionContext& c) -> CommandOutcome {
        sync(c);
        const Found f = find(c, ownerArg(a));
        if (!f.paper) return Refused{f.refusal};
        Entry& e = *entryOf(f.paper);
        if (flagArg(a, "copy")) return copy(c, e, true);
        e.paper->cut();
        publish();
        return Done{};
    });
    // The replies of `paper text` and `paper marks`.
    s.request(kPaperTextRequest, [this](const sessions::Value& a, SessionContext& c) {
        sync(c);
        const Found f = find(c, ownerArg(a));
        if (!f.paper) return replyRefused(f.refusal);
        if (!f.paper->description().textLayer) return replyRefused(noTextLayerSentence(f.owner));
        return replyGiven(linesPayload(textReply(*f.paper)));
    });
    s.request(kPaperMarksRequest, [this](const sessions::Value& a, SessionContext& c) {
        sync(c);
        const Found f = find(c, ownerArg(a));
        if (!f.paper) return replyRefused(f.refusal);
        return replyGiven(linesPayload(marksReply(*f.paper)));
    });
    s.request(kPaperPictureRequest, [this](const sessions::Value& a, SessionContext& c) {
        sync(c);
        const Found f = find(c, ownerArg(a));
        if (!f.paper) return replyRefused(f.refusal);
        if (f.paper->empty()) return replyRefused(kPaperEmpty);
        const PictureSize size = pictureSize(*f.paper);
        return replyGiven(valuePayload("paper-picture", sessions::Value::object({{"bytes", picturePng(*f.paper)},
                                                                                {"dotsPerInch", size.dotsPerInch},
                                                                                {"width", size.width},
                                                                                {"height", size.height}})));
    });
}

void PaperPart::beforeCall(SessionContext& c, pacing::WallTime) { sync(c); }
void PaperPart::afterCall(SessionContext& c, pacing::WallTime) { sync(c); }
void PaperPart::machineChanged(SessionContext& c) { sync(c); }

void PaperPart::sync(SessionContext& c) {
    bindAll(c);
    publish();
}

PaperPart::Entry* PaperPart::entryOf(const PeripheralId& id) {
    for (auto& e : entries_)
        if (e.printer == id) return &e;
    return nullptr;
}

PaperPart::Entry* PaperPart::entryOf(const Paper* p) {
    for (auto& e : entries_)
        if (e.paper.get() == p) return &e;
    return nullptr;
}

void PaperPart::bindAll(SessionContext& c) {
    machine::Machine* m = c.machine();
    const std::uint64_t gen = c.machineSession().generation();
    if (!generation_ || *generation_ != gen) {
        // A new machine: the capabilities of the old one are gone.
        generation_ = gen;
        for (auto& e : entries_) e.paper->release();
        // A user's preset load starts with empty paper for the printers it names.
        if (m && control_.lastBuild() && control_.lastBuild()->origin == startup::BuildOrigin::Preset)
            for (const auto& r : control_.lastBuild()->plan.peripherals)
                if (Entry* e = entryOf(r.peripheral)) e->paper->cut();
    }
    std::vector<PaperOutput*> found(entries_.size(), nullptr);
    std::vector<PaperOutput*> present;
    if (m) {
        for (const auto& cap : m->capabilities(PaperOutput::kKind)) {
            auto* out = machine::capabilityAs<PaperOutput>(*m, cap.id);
            if (!out) continue;
            present.push_back(out);
            const auto* owner = std::get_if<machine::PeripheralOwner>(&cap.owner);
            Entry* e = owner ? entryOf(owner->peripheral) : nullptr;
            const std::size_t i = e ? static_cast<std::size_t>(e - entries_.data()) : 0;
            if (!e || found[i]) {   // not declared, or a second of its type: logged once, left unbound
                if (logged_.insert(cap.id.value).second)
                    c.appLog().warn("Paper output " + cap.id.value + (e ? " is a second printer of its type" : " is not a printer of the product") +
                                    "; it is not shown.");
                continue;
            }
            found[i] = out;
        }
    }
    for (std::size_t i = 0; i < entries_.size(); ++i) {
        Paper& p = *entries_[i].paper;
        PaperOutput* old = p.bound();
        const bool alive = old && std::find(present.begin(), present.end(), old) != present.end() && old->connected() == &p;
        if (old && !alive) p.release();
        if (found[i] && found[i] == p.bound()) continue;
        p.bind(found[i]);
    }
}

void PaperPart::publish() {
    for (auto& e : entries_) {
        auto d = e.paper->takeDelta();
        if (!d) continue;
        if (d->emptied) e.topic->reset();
        if (d->emptied && d->marks.empty() && d->lines.empty() && d->reach == Reach{}) continue;
        auto item = std::make_shared<PaperAppended>();
        item->continuesStroke = d->continuesStroke;
        item->marks = std::move(d->marks);
        item->lines = std::move(d->lines);
        item->reach = d->reach;
        e.topic->append(item);
    }
}

PaperPart::Entry* PaperPart::plugged(SessionContext& c) {
    machine::Machine* m = c.machine();
    if (!m) return nullptr;
    for (const auto& p : m->plugged().byPlace)
        if (Entry* e = entryOf(p.peripheral); e && e->paper->bound()) return e;
    return nullptr;
}

PaperPart::Found PaperPart::find(SessionContext& c, const std::optional<std::string>& owner) {
    Found f;
    if (!owner) {
        Entry* e = plugged(c);
        if (!e) {
            f.refusal = kNoPrinterPlugged;
            return f;
        }
        f.paper = e->paper.get();
        f.owner = e->owner;
        return f;
    }
    const std::string want = machine::lowerAscii(*owner);
    std::vector<std::string> owners;
    for (auto& e : entries_) {
        owners.push_back(e.owner);
        if (machine::lowerAscii(e.owner) == want) {
            f.paper = e.paper.get();
            f.owner = e.owner;
            return f;
        }
    }
    f.refusal = unknownPrinterSentence(*owner, owners);
    return f;
}

std::vector<std::pair<std::string, const Paper*>> PaperPart::papers() const {
    std::vector<std::pair<std::string, const Paper*>> out;
    for (const auto& e : entries_) out.emplace_back(e.owner, e.paper.get());
    return out;
}

void PaperPart::cutPlugged(SessionContext& c) {
    sync(c);
    if (Entry* e = plugged(c)) {
        e->paper->cut();
        publish();
    }
}

// The picture is made before the ask; a cut follows the ask, whatever the
// answer. Without a listener that offers the clipboard, a copy is
// refused and a cut is done.
CommandOutcome PaperPart::copy(SessionContext& c, Entry& e, bool cutAfter) {
    Paper& p = *e.paper;
    if (p.empty()) return Refused{kPaperEmpty};
    const PictureSize size = pictureSize(p);
    sessions::Value::Object args{{"bytes", sessions::Value(picturePng(p))}};
    if (p.description().textLayer) args.push_back({"text", joined(textReply(p))});
    args.push_back({"name", e.owner + ".png"});
    args.push_back({"dotsPerInch", size.dotsPerInch});
    const std::string owner = e.owner;
    StreamTopic* copies = copiesTopic_;
    // The answer comes later, without a SessionContext; the host's app log
    // outlives the session.
    applog::AppLog* log = &c.appLog();
    const auto asked = c.askHost(HostService::ClipboardImage, sessions::Value(std::move(args)),
                                 [copies, owner, log](const ServiceAnswer& a) {
                                     auto o = std::make_shared<CopyOutcome>();
                                     o->owner = owner;
                                     const sessions::Value* t = a.result.find("textOnly");
                                     o->outcome = !a.given ? CopyOutcome::Kind::Failed
                                                  : t && t->boolean() && *t->boolean() ? CopyOutcome::Kind::TextOnly
                                                                                      : CopyOutcome::Kind::Copied;
                                     o->reason = a.given ? std::string() : a.reason;
                                     if (o->outcome == CopyOutcome::Kind::Failed)
                                         log->warn("Copy of the " + owner + "'s paper failed: " +
                                                   (a.reason.empty() ? "the host gives no reason" : a.reason));
                                     else if (o->outcome == CopyOutcome::Kind::TextOnly)
                                         log->info("Copy of the " + owner +
                                                   "'s paper: the clipboard took the text without the picture.");
                                     copies->append(o);
                                 });
    if (cutAfter) {
        p.cut();
        publish();
        return Done{};
    }
    if (!asked) return Refused{kNoClipboard};
    return Done{};
}

// ── the step and the conformance check ──────────────────────────────────

namespace {
class CutPaperItem final : public presets::Item {
public:
    explicit CutPaperItem(PaperPart& p) : part_(p) {}
    Task<presets::ItemResult> carryOut(presets::RunContext& run) override {
        part_.cutPlugged(run.session());
        co_return presets::ItemResult{};
    }
    presets::After after() const override { return presets::After::TakenEffect; }
    bool needsRunningMachine() const override { return false; }
    std::string logText() const override { return "cut-paper"; }

private:
    PaperPart& part_;
};
} // namespace

void addPaperStep(presets::Vocabulary& v, PaperPart& part) {
    PaperPart* p = &part;
    v.add(presets::StepDef{"cut-paper", presets::StepDef::Value::None,
                           [p](const formats::YamlNode&, presets::ReadContext&) -> std::unique_ptr<presets::Item> {
                               return std::make_unique<CutPaperItem>(*p);
                           },
                           false, false});
}

std::optional<std::string> checkPaperOutput(machine::Machine& m, const PeripheralId& printer, const PaperDescription& d) {
    for (const auto& cap : m.capabilities(PaperOutput::kKind)) {
        const auto* owner = std::get_if<machine::PeripheralOwner>(&cap.owner);
        if (!owner || owner->peripheral != printer) continue;
        auto* out = machine::capabilityAs<PaperOutput>(m, cap.id);
        if (!out) return "The capability " + cap.id.value + " is not of the kind paper-output.";
        if (cap.id.value != machine::placeText(owner->place) + "/" + std::string(PaperOutput::kKind.value))
            return "The capability of " + printer.value + " is named " + cap.id.value + ".";
        if (!(out->description() == d)) return "The paper description of " + printer.value + " is not the one declared.";
        if (out->standing() != 0) return "The paper of " + printer.value + " does not stand at 0 after the plug.";
        return std::nullopt;
    }
    return printer.value + " offers no paper output.";
}

} // namespace calcu::paper
