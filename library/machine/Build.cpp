#include "machine/Build.hpp"
#include <algorithm>

namespace calcu::machine {

namespace {
// The variant to build: the one named, or the default; refused when it does
// not belong to the descriptor.
template <class D>
bool resolveVariant(const D& d, std::optional<VariantId>& variant) {
    if (d.variants.empty()) return !variant;
    if (!variant) variant = d.defaultVariant;
    return variant && std::any_of(d.variants.begin(), d.variants.end(),
                                  [&](const VariantDescriptor& v) { return v.id == *variant; });
}

void addMissing(const std::vector<RomEntry>& needed, const std::vector<RomImage>& images,
                std::vector<std::string>& missing) {
    for (const auto& e : needed)
        if (std::none_of(images.begin(), images.end(), [&](const RomImage& i) { return i.fileName == e.fileName; }))
            missing.push_back(e.fileName);
}

BuildResult refused(std::string why) { return BuildResult{nullptr, {}, std::move(why)}; }
} // namespace

BuildResult build(const Catalogue& catalogue, MachineBuilder& builder, const BuildRequest& request) {
    const MachineDescriptor* model = catalogue.model(request.model);
    if (!model) return refused("unknown model \"" + request.model.value + "\"");
    BuildRequest resolved = request;
    if (!resolveVariant(*model, resolved.variant))
        return refused("model \"" + model->model.value + "\" has no variant \"" +
                       (request.variant ? request.variant->value : std::string()) + "\"");
    std::vector<std::string> missing;
    addMissing(model->romsFor(resolved.variant), resolved.roms, missing);
    for (auto& p : resolved.peripherals) {
        const PeripheralDescriptor* d = catalogue.peripheral(p.peripheral);
        if (!d) return refused("unknown peripheral \"" + p.peripheral.value + "\"");
        if (!resolveVariant(*d, p.variant))
            return refused("peripheral \"" + d->id.value + "\" has no variant \"" +
                           (p.variant ? p.variant->value : std::string()) + "\"");
        addMissing(d->romsFor(p.variant), p.roms, missing);
    }
    if (!missing.empty()) return BuildResult{nullptr, std::move(missing), "ROM files missing"};
    return builder.build(resolved, catalogue);
}

} // namespace calcu::machine
