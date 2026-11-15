#include "Machine/SharpRomImages.hpp"

#include "../Resources/BundledRomCatalog.hpp"

using namespace calcu::machine;

namespace sharp {

namespace {

bool read(const std::vector<RomEntry>& entries, const std::vector<std::string>& dirs, std::vector<RomImage>* out,
          std::string* error) {
    for (const RomEntry& e : entries) {
        std::string path;
        RomImage image{e.fileName, {}};
        if (!BundledRoms::resolveBundledRomPath(dirs, e.fileName, &path, error)) return false;
        if (!BundledRoms::detail::readWholeFileCached(path, &image.bytes)) {
            if (error) *error = "could not read the ROM " + path;
            return false;
        }
        out->push_back(std::move(image));
    }
    return true;
}

} // namespace

bool modelRomImages(const Catalogue& c, const ModelId& model, const std::optional<VariantId>& variant,
                    const std::vector<std::string>& dirs, std::vector<RomImage>* out, std::string* error) {
    const MachineDescriptor* d = c.model(model);
    if (!d) {
        if (error) *error = "unknown model " + model.value;
        return false;
    }
    return read(d->romsFor(variant), dirs, out, error);
}

bool peripheralRomImages(const Catalogue& c, const PeripheralId& id, const std::optional<VariantId>& variant,
                         const std::vector<std::string>& dirs, std::vector<RomImage>* out, std::string* error) {
    const PeripheralDescriptor* d = c.peripheral(id);
    if (!d) {
        if (error) *error = id.value + " is not a known peripheral";
        return false;
    }
    return read(d->romsFor(variant), dirs, out, error);
}

} // namespace sharp
