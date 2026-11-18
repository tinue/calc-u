#pragma once
#include <functional>
#include <optional>
#include <string>
#include <string_view>

// A product's option of `serve`: Calc-U-1600's
// `calcu --product c1600 serve --dap <port>`. It takes its value at the
// start; then it has things done on the serve loop's thread (what its own
// I/O thread got) through the loop's inbox, with standard input's lines in
// the order they came. While the inbox is empty and an add-on wants time,
// the loop lets emulated time pass, unpaced, in pieces of 1/60 s, and looks
// at the inbox between pieces: there is no wall clock.

namespace calcu::sessions {
class AppSession;
}

namespace calcu::harness {

struct ServeLoop {
    // Any thread: `task` runs on the loop's thread, in order with the lines.
    std::function<void(std::function<void()> task)> post;
    // The loop's thread: the run ends after the task that called it (exit 0).
    std::function<void()> quit;
};

class ServeAddOn {
public:
    virtual ~ServeAddOn() = default;
    virtual std::string_view option() const = 0;   // "--dap"
    virtual std::string_view help() const = 0;     // a line of `calcu --help`
    // The reason it cannot start (the run ends with exit status 2), or none.
    virtual std::optional<std::string> start(std::string_view value, sessions::AppSession&, ServeLoop) = 0;
    // The loop's thread: the machine should run now.
    virtual bool wantsTime() const = 0;
    // The loop's thread, at the end of the run.
    virtual void stop() = 0;
};

} // namespace calcu::harness
