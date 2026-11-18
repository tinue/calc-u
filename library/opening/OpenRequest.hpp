#pragma once
#include "sessions/Messages.hpp"
#include "sessions/Payload.hpp"
#include "sessions/Value.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Opening a file: the one command a drop, Open With, a file named at the
// start, the file dialog of Load Preset… and the harness send, its request
// and its reply. The session finds out what the file is; no
// frontend decides it.
//
//   open             {files: [{name, path?, bytes?}], way, upload?, confirmed?}
//                    way: drop | open-with | start | dialog | harness
//   open-would-take  {name, size?} → {takes}  (a drag, before the drop)

namespace calcu::opening {

enum class Way { Drop, OpenWith, Start, Dialog, Harness };

std::string_view wayName(Way);
std::optional<Way> wayNamed(std::string_view);

struct OpenedFile {
    std::string name;                                  // the file's name, without folders
    std::optional<std::string> path;                   // where the host has it
    std::optional<std::vector<std::uint8_t>> bytes;    // nullopt: the host could not read it
};

struct OpenRequest {
    std::vector<OpenedFile> files;   // one; several only from a Web file dialog
    Way way = Way::Drop;
    bool upload = false;             // the host cannot write the file back (a Web frontend)
    bool confirmed = false;          // the user said yes to replacing a medium of that name (sent again after `asks:`)

    sessions::Value toValue() const;
    // nullopt: the arguments are not an open request (the reason in *why).
    static std::optional<OpenRequest> fromValue(const sessions::Value&, std::string* why);
};

inline const sessions::CommandName kOpenCommand{"open"};
inline const sessions::RequestName kWouldTakeRequest{"open-would-take"};

enum class OpenedAs { Preset, Medium, FileKind, Kept };
std::string_view openedAsName(OpenedAs);

// What the file was opened as. A refusal is the reply `done = false`
// with its sentence (and the same sentence as a notice).
struct OpenReply final : sessions::PayloadOf<OpenReply> {
    static constexpr sessions::PayloadKind kKind{"open"};
    OpenReply(OpenedAs a, std::string f, std::string d = {}) : as(a), file(std::move(f)), detail(std::move(d)) {}
    OpenedAs as;
    std::string file;     // the file's name
    std::string detail;   // Medium: the place word, or "imported"; FileKind: its name
    bool same(const OpenReply& o) const { return as == o.as && file == o.file && detail == o.detail; }
    sessions::Value toValue() const override;
};

struct WouldTake final : sessions::PayloadOf<WouldTake> {
    static constexpr sessions::PayloadKind kKind{"open-would-take"};
    explicit WouldTake(bool t) : takes(t) {}
    bool takes;
    bool same(const WouldTake& o) const { return takes == o.takes; }
    sessions::Value toValue() const override { return sessions::Value::object({{"takes", takes}}); }
};

} // namespace calcu::opening
