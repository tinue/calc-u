// LIB-18 design L8: the harness commands of AC-15 (OQ-5), on the harness's
// test product.
#include "TestHarness.hpp"
#include "TestProduct.hpp"
#include "harness/Run.hpp"
#include <string>
#include <vector>

using namespace calcu::harness;
using namespace calcu::harness::test;
namespace mt = calcu::machine::test;

namespace {
bool startsWith(const std::string& s, const std::string& p) { return s.rfind(p, 0) == 0; }
} // namespace

TEST(ac15_each_command_at_top_level_with_its_operations_reply) {
    TestRun run;   // test-seg
    CHECK(run.server != nullptr);
    if (!run.server) return;
    CHECK_EQ(run.line("model test-dot"), std::string("done\n"));
    CHECK(run.machine().model() == mt::kDot);
    CHECK(run.machine().isKeyReady());   // the reply is that of the operation's end
    CHECK_EQ(run.line("variant V2"), std::string("done\n"));
    CHECK(run.machine().variant() == std::optional<calcu::machine::VariantId>(calcu::machine::VariantId{"V2"}));
    CHECK_EQ(run.line("attach a test-printer"), std::string("done\n"));
    CHECK(run.line("capabilities").find("place a test-printer\n") != std::string::npos);
    CHECK_EQ(run.machine().operationsTold(calcu::machine::PlacePath{{calcu::machine::PlaceName{"a"}}}), std::uint64_t{1});
    for (const char* c : {"reset", "cold-boot", "power-cycle"}) {
        CHECK_EQ(run.line(c), std::string("done\n"));
        CHECK(run.machine().isKeyReady());
    }
    CHECK_EQ(run.line("detach a"), std::string("done\n"));
    CHECK(run.line("capabilities").find("place a empty\n") != std::string::npos);
    CHECK_EQ(run.line("model nothing"), std::string("refused: There is no model \"nothing\".\n"));
    CHECK(startsWith(run.line("model"), "refused: usage: model <name>"));
    CHECK(startsWith(run.line("detach"), "refused: usage: detach <place>"));
    CHECK(startsWith(run.line("attach a"), "refused: usage: attach <place> <peripheral>"));
}

TEST(ac15_attach_variant_as_in_a_preset) {
    TestRun run({}, mt::kDot);
    CHECK(run.server != nullptr);
    if (!run.server) return;
    CHECK_EQ(run.line("attach card-1 test-module:8k"), std::string("done\n"));
    CHECK(run.line("capabilities").find("place card-1 test-module:8k\n") != std::string::npos);
    CHECK_EQ(run.line("attach card-1 test-module:2k"), std::string("refused: The Test module has no variant 2k.\n"));
    CHECK_EQ(run.line("attach card-1 test-module"), std::string("done\n"));   // the default variant: a replace
    CHECK(run.line("capabilities").find("place card-1 test-module:4k\n") != std::string::npos);
}

TEST(ac15_attach_place_path) {
    TestRun run({}, mt::kDot);
    CHECK(run.server != nullptr);
    if (!run.server) return;
    CHECK_EQ(run.line("attach a test-hub"), std::string("done\n"));
    CHECK_EQ(run.line("attach a/jack test-printer"), std::string("done\n"));
    CHECK(run.line("capabilities").find("place a/jack test-printer\n") != std::string::npos);
    CHECK_EQ(run.line("detach a/jack"), std::string("done\n"));
    CHECK(run.line("capabilities").find("place a test-hub\n") != std::string::npos);
}

TEST(ac15_refusal_exits_nonzero) {
    TestRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    std::string out;
    const int status = run.server->oneShot({"model test-dot", "attach slot-1 test-module", "display-text"},
                                           [&](const std::string& t) { out += t; });
    CHECK_EQ(status, 1);
    CHECK_EQ(out, std::string("done\nrefused: The test-dot has no place slot-1.\n"));
}
