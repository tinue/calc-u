#include "Cli.hpp"
#include "harness/Commands.hpp"
#include "harness/Run.hpp"
#include "harness/attach/AttachClient.hpp"

namespace calcu::harness::cli {

namespace {

std::string productList(const std::vector<std::unique_ptr<HarnessProduct>>& products) {
    std::string s = "The products of this build:";
    for (const auto& p : products) s += "\n  " + std::string(p->name()) + " (" + std::string(p->shortName()) + ")";
    return s + "\n";
}

std::string usageText(const std::vector<std::unique_ptr<HarnessProduct>>& products) {
    std::string s =
        "usage: calcu --product <name> [options] <command> [then <command> ...]\n"
        "       calcu --product <name> [options] serve\n"
        "       calcu attach [--port N] [--json] [--messages]   (a running app's session, C59-5)\n"
        "options:\n"
        "  --product <name>          the product, by name or short form, any case\n"
        "  --json                    each reply as one JSON object on one line\n"
        "  --settings <file>         use this settings file (else the defaults; nothing read or written)\n"
        "  --save-folder <dir>       use this save folder (else an empty one, removed at the end)\n"
        "  --output-folder <dir>     traces and screenshots go here (else the save folder)\n"
        "  --clock <YYYY-MM-DDThh:mm:ss> | host\n"
        "                            the date and time handed to a real-time clock\n"
        "                            (else 2000-01-01T00:00:00)\n"
        "commands:\n";
    CommandRegistry reg;
    addFirstCommands(reg);
    addPresetCommands(reg, nullptr);
    for (const auto& p : products) p->addCommands(reg);
    for (const auto& u : reg.usage()) s += "  " + u + "\n";
    return s + productList(products);
}

} // namespace

int run(const std::vector<std::string>& args, const Io& io, std::vector<std::unique_ptr<HarnessProduct>> products) {
    if (!args.empty() && args[0] == "attach")
        return runAttach(std::vector<std::string>(args.begin() + 1, args.end()), io.readLine, io.out, io.err);
    RunOptions options;
    std::optional<std::string> productName;
    std::vector<std::string> words;
    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string& a = args[i];
        if (!words.empty()) {   // the commands start: everything after belongs to them
            words.push_back(a);
            continue;
        }
        auto value = [&](const char* opt) -> std::optional<std::string> {
            if (i + 1 >= args.size()) {
                io.err(std::string(opt) + " needs a value.\n");
                return std::nullopt;
            }
            return args[++i];
        };
        if (a == "--help" || a == "-h") {
            io.out(usageText(products));
            return 0;
        } else if (a == "--json") {
            options.json = true;
        } else if (a == "--product") {
            if (!(productName = value("--product"))) return 2;
        } else if (a == "--settings") {
            if (!(options.settingsFile = value("--settings"))) return 2;
        } else if (a == "--save-folder") {
            if (!(options.saveFolder = value("--save-folder"))) return 2;
        } else if (a == "--output-folder") {
            if (!(options.outputFolder = value("--output-folder"))) return 2;
        } else if (a == "--clock") {
            auto v = value("--clock");
            if (!v) return 2;
            if (*v == "host") {
                options.clock = ClockMode::Host;
            } else if (auto t = parseDateTime(*v)) {
                options.clock = ClockMode::Given;
                options.given = *t;
            } else {
                io.err("--clock takes YYYY-MM-DDThh:mm:ss or host: " + *v + "\n");
                return 2;
            }
        } else if (a.rfind("--", 0) == 0) {
            io.err("Unknown option " + a + ".\n" + usageText(products));
            return 2;
        } else {
            words.push_back(a);
        }
    }
    // EC-4, D-5: no product, or none of this build.
    if (!productName) {
        io.err("calcu needs --product.\n" + productList(products));
        return 2;
    }
    HarnessProduct* product = findProduct(products, *productName);
    if (!product) {
        io.err("There is no product " + *productName + " in this build.\n" + productList(products));
        return 2;
    }
    const bool serve = words.size() == 1 && words[0] == "serve";
    if (words.empty()) {
        io.err("calcu needs a command, or serve.\n" + usageText(products));
        return 2;
    }
    std::string error;
    std::unique_ptr<Server> server = Server::start(*product, options, &error);   // EC-1
    if (!server) {
        io.err(error + (error.empty() || error.back() == '\n' ? "" : "\n"));
        return 2;
    }
    if (serve) return server->serve(io.readLine, io.out);
    return server->oneShot(splitOneShot(words), io.out);
}

} // namespace calcu::harness::cli
