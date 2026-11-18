#pragma once
#include "sessions/Messages.hpp"
#include <deque>
#include <optional>
#include <string>

// Notices and questions: shown one after the other in the order they came, none lost.
// A refusal of a command the shell sent is shown like a notice unless it
// repeats the last notice, says "Stopped." (an operation the user stopped)
// or is a question (`asks:`, from media and opening), which is shown with Yes and No; Yes sends the same command again
// with `confirmed: true`. Mirrored in shellModel.js (calcuShellNotices).

namespace calcu::shell {

inline constexpr const char* kAsksPrefix = "asks:";

class NoticeQueue {
public:
    struct Shown {
        std::string text;
        bool question = false;
        sessions::Command command;   // a question: what Yes sends again
    };

    void push(std::string sentence);   // a notice of the `notices` topic
    // The reason of a refused reply to `sent`, a command the shell sent.
    void refusal(std::string reason, sessions::Command sent = {});
    const Shown* current() const;      // shown now; nullptr when none
    // Closes the one shown (OK, or No for a question); the next follows.
    void close();
    // A question answered: closes it; Yes gives the command to send again.
    std::optional<sessions::Command> answer(bool yes);
    std::size_t waiting() const { return queue_.size(); }

private:
    std::deque<Shown> queue_;
    std::string last_;   // the last notice
};

} // namespace calcu::shell
