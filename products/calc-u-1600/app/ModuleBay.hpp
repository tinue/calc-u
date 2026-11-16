#pragma once
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "Connector/ExpansionCard.hpp"
#include "Connector/MemoryCardCatalog.hpp"
#include "Connector/MemoryCardDefinition.hpp"  // CardHost
#include "app/DirectoryScanCache.hpp"
#include "app/MachineModel.hpp"

namespace shared {

class AppPaths;
class SharpAppPart;

// ── ModuleBay: the memory-module slots ───────────────────────────────────
//
// All memory-module selection, attach and battery-save state behind the
// control bar's per-slot pickers. Slot 1 is always meaningful (the
// PC-1500/1500A's single expansion connector, or PC-1600 Slot 1); slot 2
// only on the PC-1600.
//
// A module change is a plug change of LIB-18 (`c1600.select-module`):
// selectModule() (state only), cardFor() builds the card the request
// carries, followMachine() once it is in. Until M-47 (LIB-7 A-1) the bay
// follows every new machine: followPreset() after a preset plugged its
// modules in, followCarriedMachine() after a variant switch carried the
// cards cardFor() gave, onModelChanged() after a model switch.
//
// Battery cards loaded from a saved instance autosave in place: tick(),
// called once per frame, writes a card kPersistDebounceSeconds after its
// content revision moved away from the one last read or written.
//
// Messages for the user come back in English (the app has no
// translations).
class ModuleBay {
public:
    struct Events {
        // A slot's selection changed outside the picker (a preset, a save).
        std::function<void(int slot)> moduleChanged;
        std::function<void(const std::string&)> error;
    };

    struct ModuleEntry {
        std::string moduleName;
        std::string description;  // the card's `description:`, for a tooltip; "" = none
        bool battery = false;
        bool rom = false;  // a ROM module -- listed in its own picker section
    };
    // `templates` are the files declaring `template: true` -- every bundled
    // card plus any the user dropped into the save folder; `instances` are
    // the save folder's other cards; `roms` the ROM-module templates, a
    // picker section of their own. A save-folder card sharing a bundled
    // card's name is left out: lookup is bundled-first, so it could never
    // be loaded.
    struct ModuleLists {
        std::vector<ModuleEntry> templates;  // RAM
        std::vector<ModuleEntry> instances;
        std::vector<ModuleEntry> roms;
    };

    static constexpr double kPersistDebounceSeconds = 0.5;

    ModuleBay(SharpAppPart& session, const AppPaths& paths, Events events);

    // The catalogue for `host`.
    ModuleLists moduleLists(CardHost host) const;
    // Which CardHost a slot resolves to under `model`.
    static CardHost hostForModel(int slot, Model model);
    // The bundled / save-folder cards, re-parsed only when a file changed.
    const std::vector<MemoryCardCatalogEntry>& bundledCards() const;
    const std::vector<MemoryCardCatalogEntry>& instanceCards() const;

    // slot is 1 or 2. Empty moduleName => detach. Saves a pending write for
    // the slot being replaced first.
    void selectModule(int slot, const std::string& moduleNameOrEmpty);
    std::string selectedModuleName(int slot) const { return m_slots[slot - 1].moduleName; }
    bool slotHasInstanceFile(int slot) const { return !m_slots[slot - 1].instanceFilePath.empty(); }
    // Name & Save is offered only for a battery-backed template; an
    // instance is kept up to date by autosave instead.
    bool canNameAndSave(int slot) const {
        const SlotState& st = m_slots[slot - 1];
        return !st.moduleName.empty() && st.isTemplate && st.battery;
    }

    // The card of the selected module of `slot`, read anew from its file
    // (a pending write saved first); nullptr with the error reported.
    std::unique_ptr<ExpansionCard> cardFor(int slot);
    // The selected module is in the slot now (select-module).
    void followMachine(int slot);
    // A variant switch carried the cards cardFor() gave: each is what its
    // file holds again.
    void followCarriedMachine();
    // After a preset plugged its own modules in: the module's name is read
    // from the slot (a module by name is a template); an empty slot clears
    // the selection; a `slot-N-rom:` (its bytes) makes the slot unsaveable.
    void followPreset(int slot, const std::vector<std::uint8_t>* romBytes);

    // The Name & Save dialog may offer "write-protect": the slot's card has
    // a write-protect switch (CE-161) that is still off.
    bool canWriteProtect(int slot) const;

    // "Name & Save". A template's name is always refused. `writeProtect`
    // saves the card with its write-protect switch on and turns it on in
    // the live card; refused for a card without a switch.
    bool nameAndSave(int slot, const std::string& instanceName, std::string* error, bool writeProtect = false);
    // Preset `saveas: template|live s1:<name>` / `s2:`. Like nameAndSave(),
    // the slot is then retargeted at the saved file, but it works when the
    // slot is already saved, and silently overwrites a file of that name.
    // A by-name save refuses a bundled name; a live one also refuses the
    // name of a user template and never overwrites a template file.
    // `filePath` (the `file:` form) writes exactly that file.
    bool saveAsFromPreset(int slot, const std::string& instanceName, const std::string& filePath, bool asTemplate,
                          std::string* error, bool writeProtect = false);

    // Once per frame: notices changed cards and writes them once the
    // debounce has passed. `nowSeconds` is any monotonic host clock.
    void tick(double nowSeconds);
    // Writes pending autosaves now (before a select, rebuild, quit).
    void flushPendingPersist();

    // A model switch: nothing carries over, so both slots are cleared
    // (after saving any pending write). Call BEFORE the switch.
    void onModelChanged();

private:
    struct SlotState {
        std::string moduleName;        // empty = no module selected
        std::string sourcePath;        // the file the card was loaded from (template or instance)
        bool isTemplate = false;       // that file declares `template: true`
        bool battery = false;          // that file declares `battery: true`
        std::string instanceFilePath;  // == sourcePath for an instance (autosaved there), else empty
        bool slotRom = false;          // a preset's `slot-N-rom:`: never saved
        std::vector<std::uint8_t> romBytes;   // its bytes, for a variant switch
        // The card's contentRevision() when last attached from / written to
        // instanceFilePath; unset = unknown, so the next persist writes.
        std::optional<std::uint64_t> persistedRevision;
    };

    // Records `resolvedPath` as the slot's source and classifies it from the
    // file's own `template:` key. Empty path = no file.
    void classifySlot(SlotState& st, const std::string& resolvedPath) const;
    bool saveSlotAs(int slot, const std::string& instanceName, bool fromPreset, std::string* error,
                    const std::string& filePath = {}, bool asTemplate = false, bool writeProtect = false);
    bool nameCollides(const std::string& instanceName) const;
    CardHost hostFor(int slot) const;
    std::uint64_t currentSlotRevision(int slot) const;
    void writeInstance(int slot);
    std::string attachedModuleName(int slot) const;
    void reportError(const std::string& text) const {
        if (m_events.error) m_events.error(text);
    }

    SharpAppPart& m_session;
    const AppPaths& m_paths;
    Events m_events;
    SlotState m_slots[2];  // index 0 = slot 1, index 1 = slot 2
    mutable DirectoryScanCache<MemoryCardCatalogEntry> m_bundledScan;
    mutable DirectoryScanCache<MemoryCardCatalogEntry> m_instanceScan;
    // Set by tick() on the first changed frame; it persists once reached.
    std::optional<double> m_persistDue;
};

} // namespace shared
