#pragma once
#include "PaperFixtures.hpp"
#include "TestMachines.hpp"
#include "TestProduct.hpp"
#include "paper/PaperOutput.hpp"
#include "paper/PaperPart.hpp"
#include "peripherals/Controls.hpp"
#include <string>
#include <vector>

// What the paper tests share: a printer the tests drive call by call, and
// helpers over the harness's test product (TestProduct, which has the
// PaperPart with TD-20 and TP-4 on the paper test catalogue, the startup
// machine control, the presets and the peripherals part): its lines, its machine, the replies split into lines.

namespace calcu::paper::test {

// A printer whose calls the test makes: emit* in its own coordinates.
class FakePrinter final : public PaperOutputBase {
public:
    explicit FakePrinter(PaperDescription d) : d_(std::move(d)) {}
    const PaperDescription& description() const override { return d_; }
    Coord standing() const override { return standing_; }
    void dot(Coord x, Coord y, std::uint8_t d = 1) { emitDot(Position{x, y}, d); }
    void down(Rgb c, Coord x, Coord y) { emitPenDown(c, Position{x, y}); }
    void to(Coord x, Coord y) { emitPenTo(Position{x, y}); }
    void up() { emitPenUp(); }
    void text(std::string t, Coord y) { emitTextLine(std::move(t), Position{0, y}); }
    void feedTo(Coord y) {
        emitFedTo(y);
        standing_ = y;
    }

private:
    PaperDescription d_;
    Coord standing_ = 0;
};

inline Coord u(double units) { return static_cast<Coord>(units * kPerUnit + (units < 0 ? -0.5 : 0.5)); }

// The lines of a plain reply after its `done <n>` line ("" lines too).
inline std::vector<std::string> replyLines(const std::string& reply) {
    std::vector<std::string> out;
    std::size_t at = reply.find('\n');
    if (at == std::string::npos || reply.rfind("done", 0) != 0) return out;
    for (std::size_t from = at + 1; from < reply.size();) {
        const std::size_t nl = reply.find('\n', from);
        out.push_back(reply.substr(from, nl == std::string::npos ? std::string::npos : nl - from));
        if (nl == std::string::npos) break;
        from = nl + 1;
    }
    return out;
}

// A run of the harness's test product, test-seg started bare.
struct PaperRun : harness::test::TestRun {
    using TestRun::TestRun;
    paper::PaperPart& part() { return *product.paperPart(); }
    // The paper, bound and sent as before a paper command.
    const Paper& paperOf(const std::string& owner) {
        part().sync(server->host().session().context());
        for (const auto& [o, p] : part().papers())
            if (o == owner) return *p;
        return *part().papers().front().second;
    }
    // The TD-20 online, through its controls capability.
    void online(const char* place = "printer") {
        const machine::PeripheralOwner owner{machine::test::kDotter, machine::PlacePath{{machine::PlaceName{place}}}};
        if (auto* c = peripherals::controlsCapability(machine(), owner)) c->set(machine::ControlId{"online"}, true);
    }
    // A key on the machine, held 10 ms, and time for the printer to finish.
    std::string press(const std::string& key) { return line("key " + key) + line("wait 100ms"); }
    std::vector<std::string> marks(const std::string& owner = "") {
        return replyLines(line("paper marks" + (owner.empty() ? std::string() : " " + owner)));
    }
    std::vector<std::string> text(const std::string& owner = "") {
        return replyLines(line("paper text" + (owner.empty() ? std::string() : " " + owner)));
    }
};

} // namespace calcu::paper::test
