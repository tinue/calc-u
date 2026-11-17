#include "qt6/AskedSend.hpp"
#include "media/SaveRules.hpp"
#include <cstring>

namespace calcu::qt6 {

std::optional<QString> askedQuestion(const std::string& reason) {
    const std::size_t n = std::strlen(media::kAsks);
    if (reason.compare(0, n, media::kAsks) != 0) return std::nullopt;
    return QString::fromStdString(reason.substr(n));
}

AskedSend sendAsking(const std::function<std::string(bool confirmed)>& send,
                     const std::function<bool(const QString& question)>& confirm) {
    AskedSend out;
    std::string why = send(false);
    if (const std::optional<QString> question = askedQuestion(why)) {
        if (!confirm(*question)) {
            out.declined = true;
            return out;
        }
        why = send(true);
    }
    out.done = why.empty();
    out.refusal = std::move(why);
    return out;
}

} // namespace calcu::qt6
