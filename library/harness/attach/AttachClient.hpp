#pragma once
#include <functional>
#include <optional>
#include <string>
#include <vector>

// `calcu attach [--port N] [--json] [--messages]` (C59-5 AC-10, AC-11; design
// §3.2, D-1, D-2, D-4): connects to 127.0.0.1:N (default 59059), sends
// `json on` first with --json, relays standard input line by line and prints
// the replies; the message lines only with --messages. It ends at `quit` or
// at the end of the input, which sends `quit`. The options that choose a
// product, a settings file, a save folder, an output folder or a ROM folder
// do not apply to an attached harness: a reason and exit status 2.
// Exit status 0; 2 when it cannot connect or for a usage error.

namespace calcu::harness {

int runAttach(const std::vector<std::string>& args,
              const std::function<std::optional<std::string>()>& readLine,
              const std::function<void(const std::string&)>& out,
              const std::function<void(const std::string&)>& err);

} // namespace calcu::harness
