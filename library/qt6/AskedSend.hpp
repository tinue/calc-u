#pragma once
#include <QString>
#include <functional>
#include <optional>
#include <string>

// The `asks:` round trip of a Qt6 host: a command that starts or saves a
// named medium may answer with the refusal "asks: <question>" (a medium of
// that name exists). The host asks the user the question with its own dialog
// and, on yes, sends the same command again with `confirmed: true`. Used by
// the Name & Save dialog and the products' "a new medium of a name" controls.

namespace calcu::qt6 {

// The question of a reason that starts with `media::kAsks`; nullopt for
// any other reason.
std::optional<QString> askedQuestion(const std::string& reason);

struct AskedSend {
    bool done = false;       // done, at once or once confirmed
    bool declined = false;   // the user said no to the question
    std::string refusal;     // neither: why not (a reason that is no question)
};

// send(false); a question goes to confirm(question) and, on yes, send(true).
// `send` returns the reply's reason, empty when it was done.
AskedSend sendAsking(const std::function<std::string(bool confirmed)>& send,
                     const std::function<bool(const QString& question)>& confirm);

} // namespace calcu::qt6
