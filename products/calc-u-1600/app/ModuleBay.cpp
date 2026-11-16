#include "app/ModuleBay.hpp"

#include <algorithm>
#include <filesystem>
#include <set>
#include <system_error>

#include "Connector/MediaSave.hpp"
#include "Connector/MemoryCardCatalog.hpp"
#include "Connector/SoftwareDefinedCard.hpp"
#include "Machine/SharpRoms.hpp"
#include "app/AppPaths.hpp"
#include "app/SharpAppPart.hpp"
#include "app/Text.hpp"
#include "app/Utf8Path.hpp"

namespace shared {

namespace {

ModuleBay::ModuleEntry entryFrom(const MemoryCardCatalogEntry& e) { return {e.moduleName, e.description, e.battery, e.rom}; }

} // namespace

ModuleBay::ModuleBay(SharpAppPart& session, const AppPaths& paths, Events events)
    : m_session(session), m_paths(paths), m_events(std::move(events)) {}

CardHost ModuleBay::hostFor(int slot) const { return hostForModel(slot, m_session.currentModel()); }

CardHost ModuleBay::hostForModel(int slot, Model model) {
    if (model == Model::PC1600) return slot == 2 ? CardHost::PC1600Slot2 : CardHost::PC1600Slot1;
    return model == Model::PC1500A ? CardHost::PC1500A : CardHost::PC1500;
}

const std::vector<MemoryCardCatalogEntry>& ModuleBay::bundledCards() const {
    // The folder's definitions (the scan passes over the ROM set's files)
    // and the ROM modules of the set: the CE-502B (LIB-12 AC-14).
    return m_bundledScan.get(m_paths.bundledResourcesDir(), [this](const std::string& dir) {
        std::vector<MemoryCardCatalogEntry> cards = scanMemoryCardDirectory(dir, nullptr);
        if (const auto& roms = m_session.romSet())
            for (const sharp::RomSetModule& m : sharp::romSetModules(*roms)) {
                const MemoryCardDefinition& d = m.definition;
                cards.push_back({d.moduleName, d.description, d.compatibleHosts, {}, d.battery, d.isRom(), d.isTemplate});
            }
        std::sort(cards.begin(), cards.end(),
                  [](const MemoryCardCatalogEntry& a, const MemoryCardCatalogEntry& b) { return a.moduleName < b.moduleName; });
        return cards;
    });
}

const std::vector<MemoryCardCatalogEntry>& ModuleBay::instanceCards() const {
    return m_instanceScan.get(m_paths.instanceDir(),
                              [](const std::string& dir) { return scanMemoryCardDirectory(dir, nullptr); });
}

ModuleBay::ModuleLists ModuleBay::moduleLists(CardHost host) const {
    ModuleLists lists;
    const auto add = [&lists](ModuleEntry entry, bool isTemplate) {
        (!isTemplate ? lists.instances : entry.rom ? lists.roms : lists.templates).push_back(std::move(entry));
    };
    std::set<std::string> bundled;
    for (const auto& e : bundledCards()) {
        bundled.insert(e.moduleName);
        if (e.compatibleWith(host)) add(entryFrom(e), /*isTemplate=*/true);
    }
    for (const auto& e : instanceCards())
        if (e.compatibleWith(host) && !bundled.count(e.moduleName)) add(entryFrom(e), e.isTemplate);
    return lists;
}

void ModuleBay::selectModule(int slot, const std::string& moduleNameOrEmpty) {
    const int idx = slot - 1;
    flushPendingPersist();
    m_slots[idx] = SlotState{};
    m_slots[idx].moduleName = moduleNameOrEmpty;
}

std::unique_ptr<ExpansionCard> ModuleBay::cardFor(int slot) {
    flushPendingPersist();
    SlotState& st = m_slots[slot - 1];
    if (st.moduleName.empty()) return nullptr;
    const CardHost host = hostFor(slot);
    std::string path, err;
    // A ROM module of the set (the CE-502B): from its bytes in the set, a
    // template that is never saved.
    if (const auto& roms = m_session.romSet()) {
        if (auto m = sharp::romSetModule(*roms, st.moduleName)) {
            const std::vector<uint8_t>* rom = st.slotRom ? &st.romBytes : nullptr;
            if (auto card = makeSoftwareDefinedCardFromText(m->text, m->fileName, host, &err, rom)) {
                if (!st.slotRom) {
                    classifySlot(st, {});
                    st.isTemplate = m->definition.isTemplate;
                }
                return card;
            }
            reportError("Couldn't attach " + inQuotes(st.moduleName) + ": " + err);
            st = SlotState{};
            return nullptr;
        }
    }
    if (resolveModuleSpecByName({m_paths.bundledResourcesDir(), m_paths.instanceDir()}, st.moduleName, &path, &err)) {
        // A preset's ROM module under development: its ROM again, never saved.
        const std::vector<uint8_t>* rom = st.slotRom ? &st.romBytes : nullptr;
        if (auto card = makeSoftwareDefinedCard(path, host, &err, rom)) {
            if (!st.slotRom) classifySlot(st, path);
            return card;
        }
    }
    reportError("Couldn't attach " + inQuotes(st.moduleName) + ": " + err);
    st = SlotState{};
    return nullptr;
}

void ModuleBay::classifySlot(SlotState& st, const std::string& resolvedPath) const {
    st.sourcePath = resolvedPath;
    st.isTemplate = false;
    st.battery = false;
    st.instanceFilePath.clear();
    if (resolvedPath.empty()) return;
    MemoryCardCatalogEntry entry;
    if (readMemoryCardCatalogEntry(resolvedPath, &entry, nullptr)) {
        st.isTemplate = entry.isTemplate;
        st.battery = entry.battery;
    }
    // An instance autosaves in place, wherever it was loaded from -- but
    // never into the bundle, even if a bundled file lacked `template: true`.
    if (!st.isTemplate && !isUnderDir(resolvedPath, m_paths.bundledResourcesDir())) st.instanceFilePath = resolvedPath;
}

std::string ModuleBay::attachedModuleName(int slot) const {
    const ExpansionCard* card = m_session.slotCard(slot);
    return card ? card->moduleName() : std::string();
}

void ModuleBay::followMachine(int slot) {
    SlotState& st = m_slots[slot - 1];
    // Freshly loaded from the file -- identical to it until the machine
    // writes to the card.
    if (!st.instanceFilePath.empty()) st.persistedRevision = currentSlotRevision(slot);
    if (m_events.moduleChanged) m_events.moduleChanged(slot);
}

void ModuleBay::followCarriedMachine() {
    for (int slot = 1; slot <= 2; ++slot) {
        SlotState& st = m_slots[slot - 1];
        if (st.moduleName.empty()) continue;
        if (attachedModuleName(slot).empty()) {
            st = SlotState{};
            if (m_events.moduleChanged) m_events.moduleChanged(slot);
            continue;
        }
        if (!st.instanceFilePath.empty()) st.persistedRevision = currentSlotRevision(slot);
    }
}

void ModuleBay::followPreset(int slot, const std::vector<std::uint8_t>* romBytes) {
    const int idx = slot - 1;
    m_slots[idx] = SlotState{};
    m_slots[idx].moduleName = attachedModuleName(slot);
    if (!m_slots[idx].moduleName.empty()) {
        std::string path, err;
        if (resolveModuleSpecByName({m_paths.bundledResourcesDir(), m_paths.instanceDir()}, m_slots[idx].moduleName, &path,
                                    &err))
            classifySlot(m_slots[idx], path);
        else
            m_slots[idx].isTemplate = true;   // a ROM module of the set
        if (romBytes) {
            m_slots[idx].slotRom = true;
            m_slots[idx].romBytes = *romBytes;
            m_slots[idx].instanceFilePath.clear();
        }
        if (!m_slots[idx].instanceFilePath.empty()) m_slots[idx].persistedRevision = currentSlotRevision(slot);
    }
    if (m_events.moduleChanged) m_events.moduleChanged(slot);
}

bool ModuleBay::nameCollides(const std::string& instanceName) const {
    std::error_code ec;
    if (std::filesystem::exists(toPath(m_paths.instancePathFor(instanceName)), ec)) return true;
    for (const auto& e : instanceCards())
        if (e.moduleName == instanceName) return true;
    return false;
}

bool ModuleBay::canWriteProtect(int slot) const {
    const ExpansionCard* card = m_session.slotCard(slot);
    return card && card->hasWriteProtectSwitch() && !card->writeProtected();
}

bool ModuleBay::nameAndSave(int slot, const std::string& instanceName, std::string* error, bool writeProtect) {
    return saveSlotAs(slot, instanceName, /*fromPreset=*/false, error, {}, false, writeProtect);
}

bool ModuleBay::saveAsFromPreset(int slot, const std::string& instanceName, const std::string& filePath,
                                 bool asTemplate, std::string* error, bool writeProtect) {
    return saveSlotAs(slot, instanceName, /*fromPreset=*/true, error, filePath, asTemplate, writeProtect);
}

bool ModuleBay::saveSlotAs(int slot, const std::string& instanceName, bool fromPreset, std::string* error,
                           const std::string& filePath, bool asTemplate, bool writeProtect) {
    const std::string name = trimmed(instanceName);
    if (name.empty()) {
        *error = "Name cannot be empty.";
        return false;
    }
    SlotState& st = m_slots[slot - 1];
    if (st.moduleName.empty()) {
        *error = "No module attached.";
        return false;
    }
    if (st.slotRom) {
        *error = "The ROM of " + inQuotes(st.moduleName) + " comes from the preset's slot ROM file"
                 ", not from its module file, so it can't be saved.";
        return false;
    }
    if (!fromPreset && !st.isTemplate) {
        *error = inQuotes(st.moduleName) + " is already saved; changes are saved automatically.";
        return false;
    }
    if (st.sourcePath.empty()) {
        *error = inQuotes(st.moduleName) + " has no source file to save from.";
        return false;
    }
    std::string newPath;
    MediaSaveRefusal refusal = m_paths.planNamedSave(MediaKind::Card, name, filePath, asTemplate, &newPath);
    // A ROM module of the set is bundled too, if not in the bundled folder's scan.
    if (refusal == MediaSaveRefusal::None && filePath.empty() && m_session.romSet() &&
        sharp::romSetModule(*m_session.romSet(), name))
        refusal = MediaSaveRefusal::TemplateName;
    if (refusal != MediaSaveRefusal::None) {
        *error = mediaSaveRefusalMessage(refusal, name, newPath);
        return false;
    }
    if (!fromPreset && nameCollides(name)) {
        *error = "A card named " + inQuotes(name) + " already exists. Choose a different name.";
        return false;
    }

    // Splice from the file the card was loaded from -- a template, or (a
    // preset save-as) an instance; either way its layout is the card's.
    const ExpansionCard* card = m_session.slotCard(slot);
    if (!card) {
        *error = "Couldn't read the live card contents.";
        return false;
    }
    if (writeProtect && !card->hasWriteProtectSwitch()) {
        *error = inQuotes(st.moduleName) + " has no write-protect switch.";
        return false;
    }
    const std::uint64_t revision = card->contentRevision();
    std::string spliced, splErr;
    if (!formatCardSave(*card, st.sourcePath, name, asTemplate, &spliced, &splErr,
                        writeProtect ? std::optional<bool>(true) : std::nullopt)) {
        *error = "Couldn't generate the instance file: " + splErr;
        return false;
    }
    if (!atomicWriteFile(newPath, spliced)) {
        *error = "Couldn't write " + inQuotes(newPath) + ".";
        return false;
    }

    // The switch flips in the running card too: writes stop now, and the
    // host ROM sees the module as unwritable at its next power-on scan.
    if (writeProtect) m_session.setSlotWriteProtected(slot, true);

    // Retarget the slot at the saved copy: it now shows under its new name
    // and -- unless it was saved as a template -- autosaves there.
    st.moduleName = name;
    st.sourcePath = newPath;
    st.isTemplate = asTemplate;  // `battery` carries over
    st.instanceFilePath = asTemplate ? std::string() : newPath;
    st.persistedRevision = revision;
    if (m_events.moduleChanged) m_events.moduleChanged(slot);
    return true;
}

void ModuleBay::writeInstance(int slot) {
    SlotState& st = m_slots[slot - 1];
    if (st.instanceFilePath.empty()) return;

    // Runs on every debounce: skip the image copy, file read and rewrite
    // unless the card's content changed since it was last attached from or
    // written to the file.
    const ExpansionCard* card = m_session.slotCard(slot);
    if (!card) return;
    const std::uint64_t revision = card->contentRevision();
    if (st.persistedRevision == revision) return;

    std::string spliced, err;
    if (!formatCardSave(*card, st.instanceFilePath, st.moduleName, false, &spliced, &err)) return;
    if (atomicWriteFile(st.instanceFilePath, spliced)) st.persistedRevision = revision;
}

std::uint64_t ModuleBay::currentSlotRevision(int slot) const {
    const ExpansionCard* card = m_session.slotCard(slot);
    return card ? card->contentRevision() : 0;
}

void ModuleBay::tick(double nowSeconds) {
    if (!m_persistDue) {
        // The first change starts the debounce; later ones coalesce.
        for (int slot = 1; slot <= 2; ++slot) {
            const SlotState& st = m_slots[slot - 1];
            if (!st.instanceFilePath.empty() && st.persistedRevision != currentSlotRevision(slot)) {
                m_persistDue = nowSeconds + kPersistDebounceSeconds;
                break;
            }
        }
        return;
    }
    if (nowSeconds >= *m_persistDue) flushPendingPersist();
}

void ModuleBay::flushPendingPersist() {
    m_persistDue.reset();
    writeInstance(1);
    writeInstance(2);
}

void ModuleBay::onModelChanged() {
    flushPendingPersist();
    for (auto& st : m_slots) st = SlotState{};
}

} // namespace shared
