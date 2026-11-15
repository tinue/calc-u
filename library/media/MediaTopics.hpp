#pragma once
#include "media/Shelf.hpp"
#include "sessions/Messages.hpp"
#include "sessions/Payload.hpp"
#include "sessions/Value.hpp"
#include <optional>
#include <string>
#include <vector>

// The topic `media` and the commands (LIB-7 design §3.6; spec AC-7, AC-8,
// AC-9, AC-16, AC-20, AC-29 to AC-31): the media of the product by kind, and
// per place what it holds, whether the machine waits for a medium, whether
// Name & Save can be used, and its picker in the order of AC-8. A frontend
// draws them as they are; no rule lives in it.

namespace calcu::media {

inline const sessions::TopicName kMediaTopic{"media"};

inline const sessions::CommandName kRefreshCommand{"media.refresh"};          // —
inline const sessions::CommandName kChooseCommand{"media.choose"};            // {place, entry}
inline const sessions::CommandName kEjectCommand{"media.eject"};              // {place}
inline const sessions::CommandName kTurnCommand{"media.turn"};                // {place}
inline const sessions::CommandName kArmCommand{"media.arm"};                  // {place, entry}
inline const sessions::CommandName kFeedCommand{"media.feed"};                // {place, side}
inline const sessions::CommandName kNameAndSaveCommand{"media.name-and-save"};   // {place, name, protect?, confirmed?}
inline const sessions::CommandName kStartNewCommand{"media.start-new"};       // {place, template, name, confirmed?}
inline const sessions::CommandName kOpenFileCommand{"media.open-file"};       // {name, place?, path?, bytes?, upload?, confirmed?}
inline const sessions::CommandName kImportCommand{"media.import"};            // {paths?}
inline const sessions::CommandName kExportCommand{"media.export"};            // {kind, name}
inline const sessions::CommandName kDeleteCommand{"media.delete"};            // {kind, name, confirmed?}

struct PickerEntry {
    enum class Group { Empty, BundledTemplate, UserTemplate, Instance, Module } group = Group::Empty;
    std::string id;                         // what `media.choose` takes back
    std::string label;                      // the name; a module that is not a medium: the listing's label (AC-8)
    std::optional<std::string> tooltip;     // the description
    bool canChoose = true;                  // false: outdated (AC-9) or not now
    std::optional<std::string> hint;        // the outdated hint, or why not now
    bool operator==(const PickerEntry&) const = default;
};

struct PlaceState {
    std::string place;                      // the place word (§3.3)
    std::string kind;
    bool slot = false;                      // a module slot
    bool passThrough = false;
    std::vector<int> sides;
    struct Held {
        std::string name;
        std::optional<int> side;
        bool isTemplate = false, unsaved = false, writeProtected = false;
        bool operator==(const Held&) const = default;
    };
    std::optional<Held> holds;              // unsaved: a template written to (AC-15) or a started name not yet written (AC-20)
    std::optional<std::string> armed;       // a pass-through place: the armed medium (AC-27)
    bool waits = false;                     // the machine waits for a medium now
    bool nameAndSave = false;               // AC-16, EC-9
    bool askWriteProtect = false;           // AC-16: the medium has a switch still off
    std::vector<PickerEntry> picker;        // AC-8 order
    bool operator==(const PlaceState&) const = default;
};

struct KindState {
    std::string kind, label, suffix;
    struct Item {
        std::string name, description, fileName;
        Origin origin = Origin::Saved;
        bool isTemplate = false, outdated = false;
        std::optional<std::string> inPlace;
        bool operator==(const Item&) const = default;
    };
    std::vector<Item> templates, instances;   // AC-7; the saved media list reads the Saved items (AC-31)
    bool operator==(const KindState&) const = default;
};

struct MediaState final : sessions::PayloadOf<MediaState> {
    static constexpr sessions::PayloadKind kKind{"media"};
    std::vector<KindState> kinds;
    std::vector<PlaceState> places;
    bool same(const MediaState& o) const { return kinds == o.kinds && places == o.places; }
    sessions::Value toValue() const override;
    const PlaceState* place(const std::string& word) const;
    const KindState* kindState(const std::string& id) const;
};

const char* groupText(PickerEntry::Group);

} // namespace calcu::media
