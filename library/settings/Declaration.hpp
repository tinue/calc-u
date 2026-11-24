#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Each product's settings keys, declared in one place: section, key, kind,
// default, accepted values. Every frontend of the product reads its settings
// through this declaration, and the help tool writes the help's settings
// chapter from it. There is no kind for hardware: a setting never chooses or
// remembers modules, peripherals or media.

namespace calcu::settings {

enum class ValueKind {
    Text,         // any text
    Path,         // empty, or an absolute path; a leading `~` is the home folder
    Boolean,      // `true` or `false`
    Integer,      // a decimal number within [min, max]; empty too where the default is empty (not set yet)
    Choice,       // one of `choices`
    ModelOrLast,  // `last` or one of the product's models
    ModelName,    // one of the product's models, or empty
};

struct KeyDecl {
    std::string section, key;          // kebab-case; [preset] keys are model names
    ValueKind kind = ValueKind::Text;
    std::string defaultValue;          // as written in the file
    std::vector<std::string> choices;  // Choice
    std::optional<long> min, max;      // Integer; outside -> the default
    bool writtenByApp = false;         // [window], [folders], last-model
    std::string help;                  // one line; above the key in a new file, and for the help's settings chapter
    // Written in place of the default into a new settings file (a
    // frontend's first start), where a fresh install starts with more
    // than the default: a model's startup preset. A run without a file
    // (the harness) and a file that lacks the key take the default.
    std::string firstValue;
};

struct ProductDecl {
    std::string productName;           // as written: "Calc-U-59", "Calc-U-1600"
    std::vector<std::string> models;   // the glossary's model names
    std::string fallbackModel;         // the startup model when no model was used
    std::vector<KeyDecl> keys;         // the family:: keys the product has, then its own

    const KeyDecl* find(std::string_view section, std::string_view key) const;  // ASCII case ignored
    bool isModel(std::string_view name) const;
};

// The top-level key that states the file's format version.
inline constexpr const char* kFormatVersionKey = "format-version";
inline constexpr int kFormatVersion = 1;

// The keys both products may have, with their defaults; a product lists
// those it has.
namespace family {
KeyDecl startupModel();                              // [startup] model = last
KeyDecl startupLastModel();                          // [startup] last-model (app)
KeyDecl presetFor(const std::string& model, const std::string& firstPreset = {});   // [preset] <model>
KeyDecl saveFolder();                                // [storage] save-folder
KeyDecl cassetteFolder();                                // [storage] cassette-folder
KeyDecl traceFolder();                               // [trace] folder
KeyDecl traceMaxFileSizeMb();                        // [trace] max-file-size-mb = 50
KeyDecl traceLocation();                             // [trace] location = app-support
KeyDecl appLogLevel();                               // [app-log] level = default
KeyDecl serialLinkFolder();                          // [serial] link-folder
KeyDecl openFolder(const std::string& kind);         // [folders] <kind> (app)
KeyDecl windowNumber(const std::string& key, long defaultValue);  // [window] <key> (app)
} // namespace family

// Whether `value` is accepted for `decl`; a value that is not counts as
// missing, so the setting has its default.
bool accepts(const ProductDecl& product, const KeyDecl& decl, std::string_view value);

// The problems of a declaration: names that are not kebab-case (the
// keys of [preset] are model names of the product), a key declared twice,
// a default its own key does not accept. Empty when the declaration is sound.
std::vector<std::string> checkDeclaration(const ProductDecl& product);

std::string lowerAscii(std::string_view s);

} // namespace calcu::settings
