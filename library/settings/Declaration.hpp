#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Each product's settings keys, declared in one place (LIB-9 Technical
// Requirements; design §3): section, key, kind, default, accepted values.
// Every frontend of the product reads its settings through this
// declaration, and LIB-16 writes the help's settings chapter from it.
// There is no kind for hardware: a setting never chooses or remembers
// modules, peripherals or media (AC-8; ADR 0021).

namespace calcu::settings {

enum class ValueKind {
    Text,         // any text
    Path,         // empty, or an absolute path; a leading `~` is the home folder (AC-33)
    Boolean,      // `true` or `false` (AC-33)
    Integer,      // a decimal number within [min, max] (AC-11)
    Choice,       // one of `choices`
    ModelOrLast,  // `last` or one of the product's models (AC-9)
    ModelName,    // one of the product's models, or empty
};

struct KeyDecl {
    std::string section, key;          // kebab-case; [preset] keys are model names (AC-1)
    ValueKind kind = ValueKind::Text;
    std::string defaultValue;          // as written in the file (AC-3, AC-5)
    std::vector<std::string> choices;  // Choice
    std::optional<long> min, max;      // Integer; outside -> the default (AC-11)
    bool writtenByApp = false;         // [window], [folders], last-model
    std::string help;                  // one line; above the key in a new file, and for LIB-16
};

struct ProductDecl {
    std::string productName;           // as written: "Calc-U-59", "Calc-U-1600" (AC-1, AC-2)
    std::vector<std::string> models;   // the glossary's model names
    std::string fallbackModel;         // the startup model when no model was used (AC-20, AC-27, EC-1)
    std::vector<KeyDecl> keys;         // the family:: keys the product has, then its own

    const KeyDecl* find(std::string_view section, std::string_view key) const;  // ASCII case ignored
    bool isModel(std::string_view name) const;
};

// The top-level key that states the file's format version (AC-7).
inline constexpr const char* kFormatVersionKey = "format-version";
inline constexpr int kFormatVersion = 1;

// The keys `settings.md` ST-4 gives both products; a product lists those
// it has. Defaults as the spec gives them.
namespace family {
KeyDecl startupModel();                              // [startup] model = last (AC-9)
KeyDecl startupLastModel();                          // [startup] last-model (app)
KeyDecl presetFor(const std::string& model);         // [preset] <model> (AC-10)
KeyDecl saveFolder();                                // [storage] save-folder (AC-13)
KeyDecl tapeFolder();                                // [storage] tape-folder (AC-16)
KeyDecl traceFolder();                               // [trace] folder (AC-17, AC-24)
KeyDecl traceMaxFileSizeMb();                        // [trace] max-file-size-mb = 50 (AC-11)
KeyDecl traceLocation();                             // [trace] location = app-support (AC-24)
KeyDecl serialLinkFolder();                          // [serial] link-folder (AC-17)
KeyDecl openFolder(const std::string& kind);         // [folders] <kind> (app; AC-29)
KeyDecl windowNumber(const std::string& key, long defaultValue);  // [window] <key> (app)
} // namespace family

// Whether `value` is accepted for `decl`; a value that is not counts as
// missing, so the setting has its default (AC-3, EC-5).
bool accepts(const ProductDecl& product, const KeyDecl& decl, std::string_view value);

// The problems of a declaration: names that are not kebab-case (AC-1; the
// keys of [preset] are model names of the product), a key declared twice,
// a default its own key does not accept. Empty when the declaration is sound.
std::vector<std::string> checkDeclaration(const ProductDecl& product);

bool isKebabCase(std::string_view name);
std::string lowerAscii(std::string_view s);

} // namespace calcu::settings
