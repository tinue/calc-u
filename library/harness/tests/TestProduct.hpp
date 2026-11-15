#pragma once
#include "TestMachines.hpp"
#include "TestRomFiles.hpp"
#include "harness/Commands.hpp"
#include "harness/Product.hpp"
#include "harness/Run.hpp"
#include "sessions/SessionPart.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>

// The product made for the harness tests (LIB-11 design §6, T5): Calc-U-Test,
// short form `test`, command word `test`; the test machines' models test-seg
// and test-dot (LIB-1 TestMachines.hpp), test-seg offering the machine state
// (AC-24); a segment rule and a three-glyph font; a product topic
// `test.count`; the commands `test mapping` (echoes its parsed value), `test
// operation <ticks>` (an operation of that length), `test fail` (a notice
// that names a file in the save folder, then refused) and `test count`; the
// full ROM set of the test catalogue in a temporary bundle. Used by the
// library tests and by `calcu_test`, never product code (LIB-1 AC-24).

namespace calcu::harness::test {

// The three glyphs of the test font (cells of 6 × 8): "A", "B" and "C".
GlyphFont testFont();

class TestProduct final : public HarnessProduct {
public:
    TestProduct();
    ~TestProduct() override;

    std::string_view name() const override { return "Calc-U-Test"; }
    std::string_view shortName() const override { return "test"; }
    std::string_view commandWord() const override { return "test"; }
    std::string_view version() const override { return version_; }
    const machine::Catalogue& catalogue() const override { return machine::test::testCatalogue(); }
    machine::MachineBuilder& builder() override { return builder_; }
    const settings::ProductDecl& settingsDecl() const override;
    roms::LookupResult lookUpRoms() const override;
    machine::BuildRequest startupRequest(const settings::SettingsFile&, const roms::RomSet&) override;
    DisplayText readDisplay(const machine::DisplayOutput&, const machine::MachineDescriptor&) override;
    void addCommands(CommandRegistry&) override;
    std::vector<sessions::SessionPart*> sessionParts() override;

    // For the tests: the bundle folder (a file can be removed: EC-1), the
    // model to start with instead of the settings' (A-1), the version.
    const roms::test::TempFolder& romFolder() const { return roms_; }
    std::optional<machine::ModelId> startModel;
    std::string version_ = "1.0-test";

private:
    struct Builder final : machine::MachineBuilder {
        machine::test::TestBuilder inner;
        machine::BuildResult build(const machine::BuildRequest&, const machine::Catalogue&) override;
    };
    struct Part;
    roms::test::TempFolder roms_;
    roms::test::TestTextReader reader_;
    Builder builder_;
    std::unique_ptr<Part> part_;
};

// The test product as the CLI's factory gives it.
std::unique_ptr<HarnessProduct> makeTestProduct();

// One run of the test product: its server, started with `options`.
struct TestRun {
    TestProduct product;
    std::unique_ptr<Server> server;
    std::string error;
    explicit TestRun(RunOptions options = {}, std::optional<machine::ModelId> model = std::nullopt) {
        product.startModel = std::move(model);
        server = Server::start(product, options, &error);
    }
    // The reply to one line, as written.
    std::string line(const std::string& l) { return server->line(l).text; }
    machine::test::TestMachine& machine() {
        return static_cast<machine::test::TestMachine&>(*server->host().session().context().machine());
    }
};

} // namespace calcu::harness::test
