#pragma once
#include "machine/Ids.hpp"
#include "paper/Paper.hpp"
#include "sessions/Messages.hpp"
#include "sessions/Payload.hpp"
#include <string>
#include <vector>

// The topics, commands and requests of paper output. Coordinates cross as
// integers in ten-thousandths of the printer's unit (Coord), so a Web page
// reads them as the C++ side holds them.
//
//   papers              state: PaperList, the product's printers in the order declared
//   paper <owner>       stream, History::SinceReset: PaperAppended; the reset
//                       marker says the paper was emptied
//   paper-copies        stream, History::None: CopyOutcome
//   paper.copy          command {owner?}
//   paper.cut           command {owner?, copy: bool}     (copy false: cut-paper)
//   paper.text          request {owner?} → {lines: [...]}
//   paper.marks         request {owner?} → {lines: [...]}
//   paper.picture       request {owner?} → {bytes, dotsPerInch, width, height}
//
// `owner` is the owner's name in any case; without it, the printer
// that is plugged in.

namespace calcu::paper {

inline const sessions::TopicName kPapersTopic{"papers"};
inline const sessions::TopicName kPaperCopiesTopic{"paper-copies"};
inline sessions::TopicName paperTopic(const std::string& owner) { return sessions::TopicName{"paper " + owner}; }

inline const sessions::CommandName kPaperCopyCommand{"paper.copy"};
inline const sessions::CommandName kPaperCutCommand{"paper.cut"};
inline const sessions::RequestName kPaperTextRequest{"paper.text"};
inline const sessions::RequestName kPaperMarksRequest{"paper.marks"};
inline const sessions::RequestName kPaperPictureRequest{"paper.picture"};

// The sentences.
inline constexpr const char* kNoPrinterPlugged = "No printer is plugged in.";
inline constexpr const char* kPaperEmpty = "The paper is empty.";
inline constexpr const char* kNoClipboard = "No frontend offers the clipboard.";
std::string noTextLayerSentence(const std::string& owner);                               // "The CE-150 has no text layer."
std::string unknownPrinterSentence(const std::string& name, const std::vector<std::string>& owners);

struct PaperEntry {
    machine::PeripheralId printer;
    std::string owner;   // the owner's name
    PaperDescription description;
    bool operator==(const PaperEntry&) const = default;
};

struct PaperList final : sessions::PayloadOf<PaperList> {
    static constexpr sessions::PayloadKind kKind{"papers"};
    std::vector<PaperEntry> papers;
    bool same(const PaperList& o) const { return papers == o.papers; }
    // [{printer, owner, description}]
    sessions::Value toValue() const override;
};

struct PaperAppended final : sessions::PayloadOf<PaperAppended> {
    static constexpr sessions::PayloadKind kKind{"paper"};
    bool continuesStroke = false;
    std::vector<Mark> marks;
    std::vector<TextLine> lines;
    Reach reach;
    bool same(const PaperAppended& o) const {
        return continuesStroke == o.continuesStroke && marks == o.marks && lines == o.lines && reach == o.reach;
    }
    // {continuesStroke, marks, lines: [{text, x, y}], reach: [from, to]};
    // `marks` in the order made: a run of dots as one flat array
    // [x, y, darkness, …], a stroke as {colour: "#rrggbb", path: [x, y, …]}.
    sessions::Value toValue() const override;
};

struct CopyOutcome final : sessions::PayloadOf<CopyOutcome> {
    static constexpr sessions::PayloadKind kKind{"paper-copy"};
    enum class Kind { Copied, TextOnly, Failed };
    std::string owner;
    Kind outcome = Kind::Copied;
    std::string reason;
    bool same(const CopyOutcome& o) const { return owner == o.owner && outcome == o.outcome && reason == o.reason; }
    // {owner, outcome: "copied" | "text-only" | "failed", reason}
    sessions::Value toValue() const override;
};

// The reply of paper.text and paper.marks: {lines: [...]}.
sessions::PayloadPtr linesPayload(const std::vector<std::string>&);

} // namespace calcu::paper
