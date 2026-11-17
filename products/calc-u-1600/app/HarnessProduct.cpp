#include "app/HarnessProduct.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Machine/SharpCatalogue.hpp"
#include "Machine/SharpRoms.hpp"
#include "app/AppPaths.hpp"
#include "app/LcdTextPart.hpp"
#include "app/SerialScripts.hpp"
#include "app/Settings.hpp"
#include "app/SharpAppPart.hpp"
#include "app/SharpControlRules.hpp"
#include "app/SharpHarnessCommands.hpp"
#include "app/SharpMediaKinds.hpp"
#include "app/SharpPresetRules.hpp"
#include "app/SharpProductMachines.hpp"
#include "app/dap/DapPart.hpp"
#include "harness/Run.hpp"
#include "harness/ServeAddOn.hpp"
#include "roms/Bundle.hpp"
#include "sessions/AppSession.hpp"
#ifdef CALCU_ATTACH
#include "harness/attach/LocalStream.hpp"
#endif

// The product folder (design §8, O-2), set by CMake.
#ifndef CALCU1600_DIR
#define CALCU1600_DIR "."
#endif

namespace cu1600 {

namespace {

using namespace calcu;

const std::string kDir = CALCU1600_DIR;
const std::string kModules = kDir + "/frontends/qt6/resources/cards";

// ---- C1600-3 AC-22 (design §3.5): `serve --dap <port>` ----
//
// The DAP server of the harness's session (SharpAppPart's DapPart) on
// attach's one-client listener (LocalStream; D-3: built where attach is).
// What the stream's I/O thread gets is posted into the serve loop and handed
// to the part there. The add-on exists before a run; it reaches the part of
// the run it is started in.
class DapServeAddOn final : public harness::ServeAddOn
#ifdef CALCU_ATTACH
    ,
                            public dap::DapTransport
#endif
{
public:
    explicit DapServeAddOn(std::function<dap::DapPart*()> part) : partOf_(std::move(part)) {}
    std::string_view option() const override { return "--dap"; }
    std::string_view help() const override {
        return "accept a debugger (DAP, VS Code) on 127.0.0.1:<port>; emulated time passes, unpaced, while it has "
               "the machine running. A scenario that pauses by a breakpoint repeats exactly, one that pauses by hand "
               "does not.";
    }
    std::optional<std::string> start(std::string_view value, sessions::AppSession&, harness::ServeLoop loop) override {
        part_ = partOf_();
        if (!part_) return std::string("The DAP server is not in this build.");
#ifdef CALCU_ATTACH
        const std::string text(value);
        long port = -1;
        if (!text.empty() && text.size() <= 6 && text.find_first_not_of("0123456789") == std::string::npos) port = std::stol(text);
        if (std::optional<std::string> refused = part_->setRunPort(port)) return *refused;   // EC-1
        loop_ = std::move(loop);
        part_->setTransport(this);
        if (!part_->status().client && part_->status().status.rfind("Listening", 0) != 0) {
            const std::string why = part_->status().status;
            part_->setTransport(nullptr);
            return why;   // "Port <port> not available: <why>" (AC-2)
        }
        return std::nullopt;
#else
        (void)value;
        (void)loop;
        return std::string("The DAP server is not in this build.");   // D-3
#endif
    }
    bool wantsTime() const override { return part_ && part_->wantsTime(); }
    void stop() override {
#ifdef CALCU_ATTACH
        if (part_) part_->setTransport(nullptr);
#endif
        part_ = nullptr;
    }

#ifdef CALCU_ATTACH
    // ---- dap::DapTransport, on the loop's thread ----
    std::optional<std::string> listen(std::uint16_t port, std::string busy) override {
        close();
        stream_ = std::make_unique<harness::LocalStream>(
            [this](harness::StreamEvent e) {
                if (loop_.post) loop_.post([this, e = std::move(e)]() mutable { arrived(std::move(e)); });
            },
            port, std::move(busy));
        const harness::LocalStream::Opened o = stream_->open();
        if (!o.ok) {
            stream_.reset();
            return o.problem;
        }
        return std::nullopt;
    }
    void close() override {
        stream_.reset();
        client_ = 0;
    }
    void send(std::string bytes) override {
        if (stream_ && client_) stream_->write(client_, std::move(bytes));
    }
    void dropClient() override {
        if (stream_ && client_) stream_->close(client_);
        client_ = 0;
    }
    void quitProgram() override {
        if (loop_.quit) loop_.quit();   // the run ends after this request (exit 0)
    }

private:
    void arrived(harness::StreamEvent e) {
        if (!part_) return;
        switch (e.kind) {
        case harness::StreamEvent::Kind::Connected:
            client_ = e.client;
            part_->take(dap::DapInput{dap::DapInput::Kind::Connected, {}});
            break;
        case harness::StreamEvent::Kind::Bytes:
            if (e.client == client_) part_->take(dap::DapInput{dap::DapInput::Kind::Bytes, std::move(e.bytes)});
            break;
        case harness::StreamEvent::Kind::Disconnected:
            if (e.client != client_) break;   // one the part sent away already
            client_ = 0;
            part_->take(dap::DapInput{dap::DapInput::Kind::Disconnected, {}});
            break;
        }
    }

    harness::ServeLoop loop_;
    std::unique_ptr<harness::LocalStream> stream_;
    std::uint64_t client_ = 0;
#endif

private:
    std::function<dap::DapPart*()> partOf_;
    dap::DapPart* part_ = nullptr;
};

class SharpHarnessProduct final : public harness::HarnessProduct {
public:
    ~SharpHarnessProduct() override { endRun(); }

    std::string_view name() const override { return "Calc-U-1600"; }
    std::string_view shortName() const override { return "c1600"; }
    std::string_view version() const override { return CALCU1600_VERSION; }   // APP_VERSION of the Qt build (CMake)
    const settings::ProductDecl& settingsDecl() const override { return cu1600::settingsDecl(); }

    // D-11: the app layer of this run, on its settings and folders (LIB-11
    // AC-8: the save and output folders as the run passes them, the run's own
    // home); its one ROM lookup (LIB-12) is the run's.
    roms::LookupResult prepare(harness::RunSetup& r) override {
        endRun();
        settings_ = std::make_unique<Settings>(r.settings());
        // AC-20: the bundled files where the source tree has them.
        BundleDirs bundle;
        bundle.bundledResources = kModules;
        bundle.bundledExamples = kDir + "/examples";
        bundle.roms = kDir + "/roms";
        bundle.modules = kModules;
        bundle.hostDriveRom = kDir + "/firmware/pc1600-hostdrive";
        paths_ = std::make_unique<AppPaths>(*settings_, r.hostDirs(), bundle);
        SharpAppPartOptions o;
        o.canMountHostFolder = true;   // C1600-5 AC-11 (LIB-4 Part D, D-12)
        o.hosting = sessions::Hosting::VirtualTime;
        o.serialLinksOnRequest = true;   // C1600-5 AC-9
        part_ = std::make_unique<SharpAppPart>(*settings_, *paths_, std::move(o));
        SharpAppPart* part = part_.get();
        serial_ = std::make_unique<SerialScriptsPart>([part](const std::string& port) {
            return part->serialLinked(port == "ce158" ? "calcu1600-ce158.serial" : "calcu1600-rs232c.serial");
        });
        return part_->romLookup();
    }

    calcu::parts::LibraryParts& libraryParts() override { return part_->libraryParts(); }

    // The app layer's parts (the library's among them), then the display
    // text and the serial ports' scripts (Part D, D.3).
    std::vector<sessions::SessionPart*> sessionParts(const std::vector<sessions::SessionPart*>& host) override {
        return part_->sessionParts(host, {&lcd_, serial_.get()});
    }

    harness::DisplayText readDisplay(const machine::DisplayOutput& o, const machine::MachineDescriptor& d) override {
        return lcd_.read(o, d);
    }

    std::vector<harness::ServeAddOn*> serveAddOns() override { return {&dapAddOn_}; }

    // C1600-2 AC-21, C1600-5: the product's commands under its word (LIB-11 AC-13).
    void addCommands(harness::CommandRegistry& r) override {
        SharpHarnessParts parts;
        parts.app = [this] { return part_.get(); };
        parts.serial = [this] { return serial_.get(); };
        addSharpHarnessCommands(r, std::move(parts));
    }

private:
    void endRun() {
        serial_.reset();
        part_.reset();
        paths_.reset();
        settings_.reset();
    }

    // The run's app layer (prepare).
    std::unique_ptr<Settings> settings_;
    std::unique_ptr<AppPaths> paths_;
    std::unique_ptr<SharpAppPart> part_;
    std::unique_ptr<SerialScriptsPart> serial_;
    LcdTextPart lcd_;
    DapServeAddOn dapAddOn_{[this]() -> dap::DapPart* { return part_ ? part_->dapPart() : nullptr; }};
};

} // namespace

std::unique_ptr<calcu::harness::HarnessProduct> makeHarnessProduct() { return std::make_unique<SharpHarnessProduct>(); }

} // namespace cu1600
