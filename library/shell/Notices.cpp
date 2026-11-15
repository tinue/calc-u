#include "shell/Notices.hpp"
#include "sessions/LibraryTopics.hpp"

namespace calcu::shell {

void NoticeQueue::push(std::string sentence) {
    last_ = sentence;
    queue_.push_back(Shown{std::move(sentence), false, {}});
}

void NoticeQueue::refusal(std::string reason, sessions::Command sent) {
    if (reason.rfind(kAsksPrefix, 0) == 0) {   // N4
        std::string question = reason.substr(std::char_traits<char>::length(kAsksPrefix));
        while (!question.empty() && question.front() == ' ') question.erase(question.begin());
        queue_.push_back(Shown{std::move(question), true, std::move(sent)});
        return;
    }
    // D-7; a stopped operation is the user's own doing (Reset, a model switch).
    if (reason.empty() || reason == last_ || reason == sessions::kStopped) return;
    push(std::move(reason));
}

const NoticeQueue::Shown* NoticeQueue::current() const { return queue_.empty() ? nullptr : &queue_.front(); }

void NoticeQueue::close() {
    if (!queue_.empty()) queue_.pop_front();
}

std::optional<sessions::Command> NoticeQueue::answer(bool yes) {
    if (queue_.empty()) return std::nullopt;
    Shown s = std::move(queue_.front());
    queue_.pop_front();
    if (!yes || !s.question) return std::nullopt;
    sessions::Value::Object args;
    if (const auto* m = s.command.args.members()) args = *m;
    bool set = false;
    for (auto& member : args)
        if (member.key == "confirmed") {
            member.value = sessions::Value(true);
            set = true;
        }
    if (!set) args.push_back({"confirmed", sessions::Value(true)});
    return sessions::Command{s.command.name, sessions::Value(std::move(args))};
}

} // namespace calcu::shell
