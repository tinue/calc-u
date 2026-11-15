# LIB-8 Paper output: design
Status: draft for the user's review at the Part C1/C2 boundary

## 1. Scope

Part C2 works on the Library column only. The product columns are worked
in Parts D and E. Their designs come at those boundaries; §4 says only
what they replace, so that the seams here fit them.

| Column | Step | Tests from (plan §3) | Done in (plan §5.1) |
|---|---|---|---|
| Library | M-39 | AC-1–9, AC-12–23, AC-39; EC-1–8 | M-39 |
| Calc-U-1600 | M-45 (with LIB-10 1600) | AC-32–38 | M-46 |
| Calc-U-59 | M-53 (with LIB-10 59); Swift parts in C59-8 (M-58) | AC-24–31 | M-53 |

- M-39 waits for M-38 (LIB-6 L: the place for steps and the preset
  loader), M-4 and M-9 (the reference runs), and uses M-8 (the `.paper`
  files of the 14 plotter examples). It also builds on LIB-2, LIB-3,
  LIB-4 (M-37), LIB-11 and LIB-18 (M-35).
- M-39 changes `library/`. Outside it, it adds only the two converters
  of AC-39 and their reply files to the products' tests (§4.1). No
  product calls the new kind before M-45 and M-53.
- AC-39 is tested in M-39 as "the old printouts are turned into replies
  by the rules once, and the reply form is fixed". The comparison of new
  runs against these replies is C59-2 AC-11 (M-54) and C1600-4 AC-16
  (M-46). That matches the plan's "Done in M-39" for the Library column.
- The spec has no AC-10, AC-11 and no EC-9 or later. EC-1 to EC-8 are
  EC-1–8 (EC-7 and EC-8 stand after EC-2 in the spec).
- Not here: what a printer prints and when (C59-6, C1600-1); the
  controls kind (LIB-2); whether a printer is plugged in and its control
  on the bar (LIB-4, LIB-14); the CE-158's printer port; the Swift paper
  panel (C59-8); where the panel sits in a window (LIB-14 AC-23).

## 2. Modules and files

### Library (M-39): `library/paper/`, namespace `calcu::paper`

The folder of `calcu_library` that LIB-1 §2a expects. It uses
`machine`, `sessions`, `formats`, and `presets` (the step, A-2) and
`startup` (the build origin, A-3). Its tests are compiled into
`calcu_library_tests` (Frequent).

| File | Holds |
|---|---|
| `PaperOutput.hpp` | the generic kind: `Coord`, `Position`, `Rgb`, `MarkForm`, `PaperDescription`, `Dot`, `Stroke`, `Mark`, `TextLine`, `Reach`, `PaperSink`, `PaperOutput` (`kKind` `"paper-output"`), `PaperOutputBase` (§3.1); `PaperOutput.cpp` the base's keeping and replay |
| `Paper.hpp/.cpp` | `Paper`: one printer type's paper, a `PaperSink`; marks, text layer, reach, cut, binding to a printer (§3.2) |
| `Picture.hpp/.cpp` | the picture of Copy and `paper png`: `pictureSize`, `picturePng` (§3.3) |
| `Replies.hpp/.cpp` | the text forms of `paper text` and `paper marks`, `numberText`, and a reader of `paper marks` for tests and the converters (§3.4) |
| `PaperTopics.hpp/.cpp` | the payloads `PaperAppended`, `PaperList`, `CopyOutcome`; topic, command and request names (§3.5) |
| `PaperPart.hpp/.cpp` | `PaperPart : sessions::SessionPart`: the papers of a product, the topics, Copy, Cut, the requests, the step `cut-paper` (§3.6) |
| `tests/PaperFixtures.hpp/.cpp` | the descriptions of the three products' printers as test data (from AC-24, AC-32), the fixture reader |
| `tests/fixtures/` | `pc100c.marks`, `ce150.marks`, `ce1600p.marks` (copied from the converters' output, §4.1) and the reference pictures `pc100c.png`, `ce150.png`, `ce1600p.png`, `td20.png`, `tp4.png` (§3.3, §8) |
| `tests/*_tests.cpp` | one file per area (§6) |

`library/formats/` (M-39), `calcu::formats`:

| File | Holds |
|---|---|
| `Png.hpp/.cpp` | `PngWriter`: RGB 8-bit, written band by band, with a `pHYs` chunk; deflate as in Calc-U-1600's `core/Display/PngWriter.hpp` (fixed Huffman, runs at distance 1 and one scanline), extended from grey to RGB. The library's one PNG writer: the products' `screenshot` pictures (LIB-6 D-13) use it from M-46 and M-54 |

`library/harness/` (M-39):

| File | Holds |
|---|---|
| `PaperCommands.hpp/.cpp` | `addPaperCommands(CommandRegistry&)`: `cut-paper` and `paper text`, `paper marks`, `paper png`, `paper copy`, `paper cut` (AC-23); called where `addFirstCommands` is |
| `tests/TestProduct.*` (changed) | the harness's test product gets a `PaperPart` with the two test printers and the paper test catalogue |

The test machines (`library/machine/tests/TestMachines.hpp/.cpp`,
changed), as LIB-1 §2a asks ("machines built for the tests, never
product code"):

| Peripheral | Name (owner's name) | What |
|---|---|---|
| `test-dotter` | `TD-20` | dots, a text layer; fits `test.bus` and `test.chain`; key `feed`, switch `online`; variants `r1` (default), `r2` |
| `test-plotter` | `TP-4` | pen strokes in four colours, no text layer; fits `test.bus`; keys `feed`, `back` |

They are in a catalogue of their own, `paperTestCatalogue()` (the shared
test catalogue plus these two), so the lists that other areas' tests
check stay as they are. Their behaviour is in §6, "Test printers".

### Qt6 component set (M-39): `library/qt6/`, namespace `calcu::qt6`

The library has no Qt6 component set yet. The paper panel is its first
component, so M-39 creates the folder and the target (D-1); M-40 (LIB-10),
M-41 (LIB-7) and M-43 (LIB-14's shell) add to them.

| File | Holds |
|---|---|
| `library/qt6/CMakeLists.txt` | the static target `calcu_qt6` (`Qt6::Widgets`, `calcu_library`), only with `CALCU_FRONTEND_QT` and when `find_package(Qt6 COMPONENTS Widgets)` succeeds, never in an Emscripten or iOS tree; the test program `calcu_qt6_tests` (its own CTest entry, label `frequent`, `QT_QPA_PLATFORM=offscreen`) |
| `PaperPanel.hpp/.cpp` | `PaperPanel : QWidget`: header with the owner's name, the view control, the controls, Copy and Cut, the toast; the scrolling paper (§3.7) |
| `PaperPainter.hpp/.cpp` | draws marks with `QPainter` at a scale, only those in view |
| `ClipboardService.hpp/.cpp` | answers `ClipboardImage` and `ClipboardText` with `QClipboard` (`QMimeData` with the image and the text; the resolution set on the `QImage`) |
| `tests/paper_panel_tests.cpp` | AC-12–16, AC-18–20 for Qt6 (§6) |
| `library/CMakeLists.txt` (changed) | `add_subdirectory(qt6)` when Qt is on |

### React component set (M-39): `library/web/`

| File | Holds |
|---|---|
| `components/paper.js` | page side: the papers from the topics (`papers`, `paper <owner>`, `paper-copies`), `calcuPaintMarks(ctx, description, marks, scale, top, bottom)` |
| `components/PaperPanel.jsx` | `CalcuPaperPanel`: the same parts as the Qt6 panel (§3.7) |
| `src/hostServices.js` (changed) | `clipboard-image` takes `{bytes, text, name, dotsPerInch}`; `armClipboard()` for the user gesture; the fallback of AC-21 (§5, D-9) |
| `build/dist.js` (changed) | a new list `COMPONENT_SOURCES`, exported beside `LIBRARY_SOURCES` and not in its default (LIB-14 D-17), with `components/paper.js` and `components/PaperPanel.jsx`; a product takes it in its column, so no product's `calc.js` changes in M-39. M-40, M-41 and M-43 add their components to it |
| `tests/product/TestProduct.cpp`, `src/CalcUTest.jsx` (changed) | the test product gets the paper part with `TD-20` and `TP-4`; the component shows `CalcuPaperPanel` and two buttons that attach and detach a printer |
| `tests/paper.spec.js` (new) | Playwright, Chromium and WebKit (§6) |

### Products (M-39): the converters of AC-39 only (§4.1)

| File | Change |
|---|---|
| `products/calc-u-59/tests/paper_replies_tests.cpp` (new) | the TI converter; `CALCU59_PAPER_REPLIES=1` writes the reply files, the Frequent case checks them |
| `products/calc-u-59/tests/reference/v2.3.0-examples/*.paper-text`, `*.paper-marks` (new) | the replies of the examples that print |
| `products/calc-u-1600/tests/paper_replies_tests.cpp` (new) | the Sharp converter; `CALCU1600_PAPER_REPLIES=1`, as above |
| `products/calc-u-1600/tests/reference/v0.8.1-examples/*.<plotter>.paper-marks` (new) | the replies of the 14 plotter examples |
| `products/*/tests/reference/*/README.md` (changed) | a paragraph on the reply files |

## 3. Seams and interfaces

The interfaces are sketches; the step may rename members, not change
what they carry.

### 3.1 The kind (`PaperOutput.hpp`): AC-1, AC-2, AC-3, AC-7

```cpp
namespace calcu::paper {

using Coord = std::int64_t;                     // ten-thousandths of the printer's unit (D-3)
inline constexpr Coord kPerUnit = 10'000;
struct Position { Coord x = 0, y = 0; bool operator==(const Position&) const = default; };
struct Rgb { std::uint8_t r = 0, g = 0, b = 0; bool operator==(const Rgb&) const = default; };
struct Millimetres { std::int64_t numerator, denominator; };   // exact: 430/1000, 190/1920

enum class MarkForm { Dots, PenStrokes };

struct PaperDescription {                       // AC-1; capabilities.md C-2
    Millimetres unit;
    Coord width;                                // the paper, from its left edge
    Coord printableLeft, printableWidth;        // the part that can be printed on
    MarkForm form;
    Coord dotDiameter = 0;                      // Dots
    std::vector<std::uint8_t> darknessGreys;    // Dots: grey of darkness d (0 black … 255 white)
    Coord strokeWidth = 0;                      // PenStrokes, in the picture (§3.3)
    int pictureDotsPerInch = 0;                 // AC-16: 600 PC-100C, 1200 plotters
    bool textLayer = false;
    bool operator==(const PaperDescription&) const = default;
};

struct Dot { Position at; std::uint8_t darkness; };            // AC-2
struct Stroke { Rgb colour; std::vector<Position> path; };     // AC-2
using Mark = std::variant<Dot, Stroke>;
struct TextLine { std::string text; Position at; };            // AC-3: UTF-8; "" for a feed
struct Reach { Coord from = 0, to = 0; };                      // AC-4

// What a printer calls as it prints, on the session's thread, inside
// Machine::advance (ADR 0009). Positions are in the printer's own
// coordinates: y = 0 where the paper stood when the printer was plugged in.
class PaperSink {
public:
    virtual void dot(Position, std::uint8_t darkness) = 0;
    virtual void penDown(Rgb, Position) = 0;
    virtual void penTo(Position) = 0;
    virtual void penUp() = 0;
    virtual void textLine(std::string text, Position) = 0;
    virtual void fedTo(Coord y) = 0;            // the paper stands at y now (feeds back too)
protected:
    ~PaperSink() = default;
};

class PaperOutput : public machine::Capability {
public:
    static constexpr machine::KindId kKind{"paper-output"};
    machine::KindId kind() const override { return kKind; }
    virtual const PaperDescription& description() const = 0;
    virtual Coord standing() const = 0;         // where the paper stands now (printer coordinates)
    virtual void connect(PaperSink*) = 0;       // hands over what was kept while unconnected; nullptr: keep again
    virtual const PaperSink* connected() const = 0;
};

// For cores: forwards to the sink, or keeps the calls while none is
// connected and replays them at connect (D-2). A core's printer derives
// from it and calls emitDot, emitPenDown, … as it prints.
class PaperOutputBase : public PaperOutput {
protected:
    void emitDot(Position, std::uint8_t);  void emitPenDown(Rgb, Position);  void emitPenTo(Position);
    void emitPenUp();  void emitTextLine(std::string, Position);  void emitFedTo(Coord);
public:
    void connect(PaperSink*) final;  const PaperSink* connected() const final;
};

}  // namespace calcu::paper
```

- **One capability per printer**, owned by the printer
  (`PeripheralOwner`), ID `"<place-path>/paper-output"` (LIB-1 §3.7).
  It is there while the printer is plugged in (LIB-1 EC-5).
- **Push, not pull** (D-2). The printer calls the sink as each mark is
  made, so a mark made just before a detach, a replace or a cut is never
  lost (EC-1, EC-7). Until a sink is connected (a printer just attached,
  before the part has seen it) `PaperOutputBase` keeps the calls and
  hands them over at `connect`, so nothing printed in between is lost. A line cut by a detach is the printer's: it ends
  its line where the head is and calls the sink before it goes (C59-6
  EC-2, `PC100C::detaching()`).
- **A stroke is made while the pen moves.** `penDown` opens it, `penTo`
  adds positions, `penUp` closes it. Positions already given never
  change (AC-2). A move with the pen up, a feed included, gives no mark
  (AC-33); `fedTo` gives the reach (AC-4).
- **Coordinates.** x is from the paper's left edge, y in the feed
  direction. A printer whose own x starts at the printable area adds
  `printableLeft` (the plotters). `Coord` is an integer in
  ten-thousandths of the unit: the PC-100C's 1.53, 7.09 and 10.18 are
  exact; the plotters' margins (6.625 mm = 66.947… units) are rounded
  once, by the product, to `669474`. No floating point is in a mark, so
  AC-7 holds exactly and the replies are the same on every platform.
- **Conformance.** `checkPaperOutput(Machine&, PeripheralId,
  const PaperDescription&)` (in `PaperPart.hpp`): the capability is
  there, its description equals the one the product declared, its
  standing is 0 after a plug. The products' tests call it (M-45, M-53).

### 3.2 The paper (`Paper.hpp`): AC-4, AC-5, AC-8, EC-1

```cpp
class Paper final : public PaperSink {
public:
    explicit Paper(PaperDescription);
    void bind(PaperOutput*);                 // nullptr: unbound; see "Offset"
    void cut();                              // AC-5
    bool empty() const;                      // no marks, no text, reach 0..0
    Reach reach() const;                     // AC-4
    const std::vector<Mark>& marks() const;  // in the order made
    const std::vector<TextLine>& text() const;
    const PaperDescription& description() const;
    struct Delta { bool emptied; bool continuesStroke; std::vector<Mark> marks;
                   std::vector<TextLine> lines; Reach reach; };
    std::optional<Delta> takeDelta();        // what changed since the last take; nullopt: nothing
    // PaperSink: as §3.1, in the bound printer's coordinates
};
```

- **Offset.** At `bind`, the paper sets `offset = paperStanding −
  printer.standing()`. Every position the printer gives is moved by it.
  So a printer attached again prints on, from where its paper was left,
  on any model (AC-8). At `cut`, the paper is emptied and
  `offset = −printer.standing()` (`paperStanding` 0): y counts again
  from where the paper stands at the cut (spec, "position").
- **Reach** is from the least to the greatest y of any mark position and
  any `fedTo` since the paper was last empty, 0..0 while empty (AC-4,
  EC-2). A dot's own size does not count; the picture adds it (§3.3).
- **A stroke open at a cut** is closed on the old paper; the new paper
  gets a stroke that starts at the pen's position (moved by the new
  offset) with the next `penTo` (EC-1, D-5). A stroke open at an unbind
  is closed.
- **No limit** on the length (AC-4; LIB-3 D-8).

### 3.3 The picture (`Picture.hpp`): AC-16, AC-17

```cpp
struct PictureSize { int width, height; double dotsPerInch; Coord top, bottom; };
PictureSize pictureSize(const Paper&);                  // AC-16, AC-17
std::vector<std::uint8_t> picturePng(const Paper&);     // empty for an empty paper
```

- **Extent.** The full width (`0..width`), and from `min(reach.from,
  the top of the highest mark)` to `max(reach.to, the bottom of the
  lowest mark)`, so that no mark is cut (a dot's radius, half a stroke).
  Nothing else is added (D-6).
- **Size.** `dpi = pictureDotsPerInch`; a side is `units × unit mm /
  25.4 × dpi`, rounded. If a side would exceed 16000 pixels or the area
  64 000 000 pixels, `dpi` is lowered to the highest value at which
  neither is so, and the sides are rounded down (AC-17). The paper is
  never cropped.
- **Drawing.** White, then every mark in the order made. A dot is a disc
  of `dotDiameter` in `darknessGreys[darkness]`; where dots overlap, the
  darker grey wins. A stroke is a polyline of `strokeWidth` with round
  ends and joins, in its colour, over what is there. The edge of a mark
  is anti-aliased from the distance of the pixel centre to the shape,
  over one pixel. One position alone is a round dot of the stroke width
  (as Calc-U-1600's `lineTo(+0.01)` today).
- **Bands.** The picture is drawn in bands of 256 rows into `PngWriter`,
  marks sorted once by their top. A 64-megapixel picture thus never
  needs its whole RGB buffer (D-7).
- **The same pixels everywhere.** `Picture.cpp` is compiled with
  `-ffp-contract=off` (MSVC: `/fp:precise`, its default). The reference
  pictures (§6, T5) are compared byte for byte. The `pHYs` chunk carries
  the resolution. What a frontend writes into its own clipboard form
  (`QImage::setDotsPerMeterX`) it takes from `dotsPerInch` (TR).
- **The panel** does not use this. It draws on screen with `QPainter` or
  canvas 2D (§3.7).

### 3.4 Replies (`Replies.hpp`): AC-23, AC-39, EC-2

```cpp
std::string numberText(Coord);                          // 7, 6.53, -2, 66.9474
std::vector<std::string> textReply(const Paper&);       // one line per text line
std::vector<std::string> marksReply(const Paper&);      // "reach <from> <to>", then a line per mark
std::optional<Paper> readMarksReply(const std::vector<std::string>&, const PaperDescription&);
```

- A number is written as `Coord / 10000` with at most four decimals, no
  trailing zeros, and no point for a whole number (AC-23: "a whole
  number without a point").
- `dot <x> <y> <darkness>`; `stroke <colour> <x>,<y> <x>,<y> …` on one
  line, the colour as `#rrggbb` in lower case (D-4).
- An empty paper gives no text lines and `reach 0 0` (EC-2).
- The form is fixed once the reference replies exist (OQ-4). A change is
  a finding.

### 3.5 Topics, commands and requests (`PaperTopics.hpp`): AC-6, AC-9, AC-16, AC-18, AC-20

| Name | Shape | Payload, arguments |
|---|---|---|
| `papers` | state | `PaperList`: per printer of the product, in the order declared: peripheral ID, owner's name, `PaperDescription` |
| `paper <owner>` (`paper PC-100C`) | stream, `History::SinceReset` | `PaperAppended`: `continuesStroke`, marks, text lines, reach now. The reset marker says the paper was emptied (AC-6) |
| `paper-copies` | stream, `History::None` | `CopyOutcome`: owner, `copied` / `text-only` / `failed`, reason (AC-20) |
| `paper.copy` | command | `{owner?}` (AC-16) |
| `paper.cut` | command | `{owner?, copy: bool}` (AC-18; `copy: false` for `cut-paper`, AC-22) |
| `paper.text`, `paper.marks` | request | `{owner?}` → the reply lines (§3.4) |
| `paper.picture` | request | `{owner?}` → `{bytes, dotsPerInch, width, height}` |

- `owner` is the owner's name, any case (D-8). Without it: the printer
  plugged in; refused "No printer is plugged in." when there is none
  (EC-8). An unknown name: "Unknown printer: <name>. Printers: CE-150,
  CE-1600P.".
- `paper.copy`, `paper.cut` with `copy`, `paper.picture` on an empty
  paper are refused: "The paper is empty." (AC-19). `paper.cut` with
  `copy: false` on an empty paper is done.
- `paper.text` without a text layer is refused: "The CE-150 has no text
  layer." (AC-23).
- `toValue()` of `PaperAppended` gives dots as one flat integer array
  `[x, y, d, …]` and each stroke as `{colour, path: [x, y, …]}`, so that
  the worker sends long papers without a tree of small objects (as LIB-15
  N5 does for the display).

### 3.6 The part (`PaperPart.hpp`): AC-6, AC-8, AC-9, AC-16–19, AC-22, EC-1–5, EC-7, EC-8

```cpp
struct PrinterPaper { machine::PeripheralId printer; PaperDescription description; };

class PaperPart final : public sessions::SessionPart {
public:
    PaperPart(const machine::Catalogue&, std::vector<PrinterPaper>,
              const startup::MachineControl&);                       // owner's name: the descriptor's name; lastBuild() (A-3)
    void attach(sessions::SessionSetup&) override;     // §3.5; the step (A-2)
    void beforeCall(sessions::SessionContext&, pacing::WallTime) override;  // bind (A-1)
    void afterCall(sessions::SessionContext&, pacing::WallTime) override;   // bind; deltas → topics
    void machineChanged(sessions::SessionContext&) override;            // bind; a user's load (A-3)
    void cutPlugged(sessions::SessionContext&);                         // the step `cut-paper`
};
void addPaperStep(presets::Vocabulary&, PaperPart&);                    // A-2
std::optional<std::string> checkPaperOutput(machine::Machine&, const machine::PeripheralId&,
                                            const PaperDescription&);   // §3.1, conformance
```

- **One paper per declared printer**, made with the part, kept until
  the session ends (AC-8). The product hands the list: Calc-U-59 the
  PC-100C, Calc-U-1600 the CE-150 and the CE-1600P. A product without
  printers adds no part, so there is no paper topic (EC-2).
- **Binding.** On `machineChanged`, in `beforeCall`, in `afterCall`
  and before every paper command and request, the part lists the
  machine's `paper-output` capabilities (A-1). A paper whose printer is
  no longer there is unbound; a capability whose `connected()` is not
  its paper is bound to the paper of its owner's peripheral ID. A
  capability whose peripheral is not declared, or a second one of a
  type, is logged and left unbound (D-10). Binding sends nothing
  (AC-9).
- **`afterCall`** and before every paper command and request: each
  paper's `takeDelta()` becomes one `PaperAppended`, or a reset of the
  stream when `emptied` (and then the rest). A frontend that starts to
  listen gets the stream's history since its reset: the whole paper
  (AC-6, EC-3). The marks are shown at the end of the host call in which
  they came (EC-4). Copy takes what was made up to the command (EC-5).
- **Copy** (`paper.copy`, and `paper.cut` with `copy`): the PNG of
  §3.3, then `askHost(ClipboardImage, {bytes, text, name:
  "<owner>.png", dotsPerInch})`, `text` only with a text layer, the
  lines joined by `\n` (AC-16). The answer becomes a `CopyOutcome`. When
  no listener offers the service, `paper.copy` is refused "No frontend
  offers the clipboard." and `paper.cut` cuts and is done (AC-18).
- **Cut** cuts after the ask is sent, whatever the answer (AC-18). The
  picture is made before.
- **A load by the user** (A-3) empties the papers of the printers the
  preset names, before its steps start (AC-8 table; LIB-6 AC-41). A
  startup preset at a model switch leaves them (OQ-5).
- **Reset, Cold boot, power cycle** do nothing to a paper; the printer
  is told of the operation by the machine (LIB-2 AC-10), keeps its
  coordinates and so stays bound (D-11).

### 3.7 The paper panel (Qt6 `PaperPanel`, React `CalcuPaperPanel`): AC-12–15, AC-19–21

Both are built from the same inputs; neither names a printer (TR;
`capabilities.md` I-1).

- **Inputs.** The `papers` topic; the `paper <owner>` streams (every
  paper is kept on the page side, the one shown follows the owner); the
  `paper-copies` stream; the owner, from LIB-4's control state (A-4),
  handed in by the shell (`setOwner(std::optional<PeripheralId>)`,
  `owner` prop); the owner's controls (LIB-2 AC-14) from the control
  state, and a sender for the commands.
- **Header.** The owner's name, or nothing (AC-12). With a text layer,
  a control that switches between marks and text and shows the view
  that is on (AC-14). The owner's controls: a key as a button that is
  down while held, a switch as a button that shows its position (AC-15).
  Copy and Cut, usable only with an owner and a paper that is not empty
  (AC-12, AC-19).
- **Paper.** The whole width scaled to the panel's width, every mark at
  its position: a dot as a disc of its scaled diameter in its grey, a
  stroke in its colour, at its scaled width but at least one point wide
  (AC-34 asks one point, today's width, D-12). Only marks in view are
  drawn. 8 points of blank above and below the paper in the view; no
  side margin.
- **Scrolling** (AC-13). Wheel and touch; no scroll bar (Qt6:
  `Qt::ScrollBarAlwaysOff`; Web: `scrollbar-width: none` and the
  `::-webkit-scrollbar` rule). When the paper grows and the view is at
  its end or no more than 64 points from it, it scrolls to the end;
  otherwise it stays.
- **Text view** (AC-14). The text lines in a fixed-width font,
  selectable (Qt6: a read-only `QPlainTextEdit`; Web: a `<pre>`).
- **Toast** (AC-20). On a `CopyOutcome` for its owner: "Copied to
  clipboard" for `copied` and `text-only`, "Copy failed" for `failed`,
  for 1.8 seconds, inside the panel. It is not a notice.
- **Web gesture** (AC-21, D-9). The click on Copy or Cut calls
  `armClipboard()` before it sends the command. It writes a
  `ClipboardItem` whose PNG and text are promises, inside the gesture
  (as Calc-U-1600 does today, `frontends/web/src/painters.js`
  `writeClipboard`); the `clipboard-image` answer of the page resolves
  them. If the browser refuses the picture: with text, the text alone
  (`text-only`); without, the PNG is a download named `<owner>.png` and
  the answer is not given (`failed`).

### 3.8 What this design assumes of other features (A-1 to A-7)

- **A-1 LIB-18 (M-35).** `machineChanged` comes for a new machine
  only (`AppSession.cpp:360`, on `MachineSession::generation()`; LIB-18's
  draft §3.4 keeps it so). An attach or detach in place is seen by the
  part's own check in `beforeCall`, `afterCall` and before its commands,
  as LIB-2's host links see it by their pump; `PaperOutputBase` keeps
  what a printer makes until then. Beyond `lastBuild()` (A-3), nothing
  more is needed of LIB-18.
- **A-2 LIB-6 (M-38).** LIB-6's draft (§3.3) gives the place:
  `PresetsPart::vocabulary().add(StepDef{"cut-paper", Value::None, …,
  command = false})`, its `Item` with `After::TakenEffect` and
  `needsRunningMachine() == false` (no emulated time, also while paused:
  LIB-6 AC-19), carrying out `PaperPart::cutPlugged`. `command` is false
  because the harness command `cut-paper` takes an owner's name, which
  the step does not (AC-22, AC-23); `addPaperCommands` gives it. The
  product, which owns both parts, adds the step (`addPaperStep(Vocabulary&,
  PaperPart&)` in `PaperPart.hpp`). A preset with `- cut-paper:` is
  refused by LIB-6 AC-30 in a program without it.
- **A-3 LIB-6 and LIB-18 (M-38, M-35).** LIB-6 (its D-24) builds a
  user's load with `startup::BuildOrigin::Preset` and a startup preset
  with `BuildOrigin::StartupPreset` (LIB-18 §3.1). On `machineChanged`
  the part reads `MachineControl::lastBuild()` (LIB-18 §3.4): with
  `Preset` it empties the papers of the printers in the plan's
  peripherals, before the first step; with any other origin it leaves
  them.
- **A-4 LIB-4 (M-37).** The control state's panel entry `paper` names
  its owner as the peripheral ID (and place) of the printer plugged in,
  or none (LIB-4 AC-1), and lists that printer's controls with their
  state.
- **A-5 LIB-14 (M-43).** The shells put `PaperPanel` and
  `CalcuPaperPanel` in their area for panels and wire owner, topics and
  sender (LIB-14 AC-23). Until then the panels are tested alone. M-39
  makes `library/qt6/`, `calcu_qt6`, `calcu_qt6_tests`,
  `library/web/components/` and `COMPONENT_SOURCES`; M-43 adds the shell
  to them (LIB-14 D-16). LIB-14's `SessionHost` answers the host
  services; `ClipboardService` is answered through it from then on.
- **A-6 LIB-3.** `AppSession::advanceUnpaced` calls the parts'
  `afterCall` before its flush, as `call()` does; today it does not
  (`AppSession.cpp:527-537`), so the harness's `wait` would leave new
  marks off the topic. M-39 adds the call (one line), with a test in
  `sessions/tests/`.
- **A-7 LIB-6 (M-38).** LIB-6 adds `set-control {owner, control, on}`
  to LIB-2's `PeripheralsPart` (LIB-6 D-16): it sets a switch, and
  presses (`on: true`) or releases a key. The panel sends it with the
  owner's peripheral ID from the control state (A-4).

## 4. Product side

### 4.1 The converters of AC-39 (M-39)

Run once, with an environment variable; their output is committed next
to the references. A Frequent case in each product checks that the
committed files equal what the converter gives, so a change to the
converter is seen. The comparison of new runs with these files is in
M-54 (C59-2 AC-11) and M-46 (C1600-4 AC-16).

- **Calc-U-59** (`tests/paper_replies_tests.cpp`), from the
  `--- printed` lines of each M-9 file with lines: per line, the text
  between the bars is a text line at `(0, top)`. An empty line is a feed:
  7.09 units, no mark. Any other line is split into the 20 strings of
  the PC-100C's table (`core/PC100C.cpp` `PRN_CODE`, longest match; `x̄`
  is two code points), each mapped to its code, and drawn in the 5 by 7
  font (`app/PrinterFont.hpp` `kPC100CFont`): the dot of column c, row r
  of character i at `(i × 6.53 + c + 0.5, top + r + 0.5)`, darkness full;
  the line takes 10.18 units (AC-25; the placement of
  `frontends/qt6/app/PrinterWidget.cpp:52-62`). Reach `0` to the sum of
  the heights. A text that does not split into 20 table strings is an
  error of the converter, not a skipped line.
- **Cut-short lines** (AC-26) are not in the M-9 files: they hold the
  text only. The examples that use the printer interrupt
  (`debug_printer-interrupt-bits`, `debug_printer-interrupt-sweep`,
  `printer-interrupt`) get a `.paper-text` and no `.paper-marks` (D-13).
  Their marks are checked by the golden runs `ti59-printer` and
  `ti58c-printer` (M-8), which hold the code lines with their rows.
- **Calc-U-1600** (`tests/paper_replies_tests.cpp`), from each `.paper`
  file of M-8, one block per plotter: a stroke starts at each point with
  `newStroke` 1, its colour the pen's (black, blue (0, 0, 255), dark
  green (0, 128, 0), red (255, 0, 0)), its positions `(printableLeft +
  x, y)` in units of 190/1920 mm (AC-32, AC-33). Reach: the least and
  greatest y of the points; the files hold no feeds (D-14).

### 4.2 What the product columns replace (M-45, M-53; outline)

- **Calc-U-1600 (M-45).** The CE-150 and CE-1600P (`core/Connector/Ce150Card.hpp`,
  `CE1600PCard.hpp`, on `AlpsPlotterMechanism`) offer `paper-output`
  and call the sink from the mechanism's pen events, `standing()` the
  Y stepper; the product kind `c1600.plotter-paper` goes. `SharpAppPart`
  hands `PaperPart` its two printers. Gone: `app/PlotterPaper.*`,
  `SharpAppPart::plotPoints`, `plotRevision`, `clearPaper`, the
  `c1600.paper` topic and the `paperExport` request of `SharpWebPart`,
  `setPaperWidth`, `frontends/qt6/app/PlotterPaperWidget.*`, the paper
  parts of `frontends/web/src/Panels.jsx` and `painters.js`. The panel
  is always shown (AC-36; ledger 1600-12).
- **Calc-U-59 (M-53).** `PC100C` (`core/PC100C.*`) offers
  `paper-output`; the font and the dot rules move from
  `app/PrinterFont.hpp` into the core with the placement of §4.1; the
  product kind `c59.printer-output` (`core/PrinterOutput.hpp`) and
  `PrinterCodeLine` above the core go (C59-6 §5, C59-1 D-7). `TIAppPart`
  hands `PaperPart` the PC-100C; the interim topic `c59.printer`
  (`TIPrinterState`), `c59.cut-paper` and the printer lines in
  `TIAppPart` and `app/FrameSampler.cpp` go. Gone in Qt6 and Web:
  `frontends/qt6/app/PrinterWidget.*`, `frontends/web/src/Printer.jsx`
  and the strip code of `painters.js`. The Swift `PrinterView` is C59-8
  (M-58); M-53's design says how it reads the paper until then.

## 5. Adapters

M-39 adds no adapter. Until their steps, the old paper paths stay as they
are, beside the new kind that no product uses yet:

| Old call | Where | Removed in |
|---|---|---|
| `c59.printer-output` (`PrinterOutput::takeLines`, `takeCodeLines`, `endLineHere`), the interim topic `c59.printer`, `c59.cut-paper` | `products/calc-u-59/core/PrinterOutput.hpp`, `app/TIAppPart.*`, `app/FrameSampler.cpp` | M-53 |
| `PrinterWidget`/`PaperView`, `Printer.jsx`, strip painting of `painters.js` | Calc-U-59 Qt6 and Web | M-53 |
| `PrinterView.swift` on the interim topic | Calc-U-59 Swift | M-58 (C59-8) |
| `c1600.plotter-paper` (`sharp::PlotterPaper`), `shared::PlotterPaper.*` (`PaperGeometry`, `buildPaperStrokes`, `paperExportSize`), the `c1600.paper` topic and `paperExport` request | `products/calc-u-1600/core/Machine/SharpProductKinds.hpp`, `app/` | M-45 |
| `PlotterPaperWidget`, the paper panel of `Panels.jsx` | Calc-U-1600 Qt6 and Web | M-45 |
| the `clipboard-image` answer with bytes only | `products/calc-u-59/frontends/swift/Bridge/TIAppSession.mm` | M-58 (C59-8); nobody asks it for a paper before M-53 |

`hostServices.js` keeps answering `clipboard-image` with bytes alone (no
`text`), so the products' Web frontends are unchanged by M-39.
`core/Display/PngWriter.hpp` of Calc-U-1600 stays for `LcdScreenshot`
until M-46, where the product's `screenshot` rule (LIB-6 D-13, C1600-4)
moves onto `formats/Png` and it goes.

## 6. Tasks

Library, M-39. C++ cases are in `library/paper/tests/` (and the named
areas), program `calcu_library_tests`, Frequent; they use the session
test frontend (`sessions/tests/TestFrontend.hpp`), `ScriptedClock.hpp`
and the paper test catalogue. Qt6 cases are in `calcu_qt6_tests`
(Frequent, offscreen). Playwright cases are in `LibraryWebPlaywright`
(Longer). Tests are written before the code of each task.

**Test printers** (`TestMachines`): `TD-20`: unit 1/2 mm, width 40,
printable 2..38, dots of 0.8 units, greys `{160, 0}` (darkness 0 faint,
1 full), 300 dpi, a text layer. While `online` is on, each key pressed
on the machine prints a line: text `KEY <id>` padded to 8 characters at
`(2, top)`, then 5 dot rows, one per 20 ticks, each a fixed pattern per
character; a complete line takes 7 units. A detach in a line ends it
there: its last row with darkness 0 (EC-7). Key `feed`: 4 units and an
empty text line. `TP-4`: unit 1/10 mm, width 600, printable 50..550,
stroke width 3, 254 dpi (10 pixels a millimetre), four colours. Each
key `<k>` pressed on the machine draws one stroke through four
positions, one per 10 ticks, from `(100 + 50k, standing)`, colour k mod 4;
key `feed` feeds 20 units, `back` feeds back 30 (y below zero).

| Task | What | ACs/ECs | Test (file: cases) |
|---|---|---|---|
| T1 | CMake: `library/paper/`, `library/qt6/` (`calcu_qt6`, `calcu_qt6_tests`), the test printers | (support) | `paper_kind_tests.cpp`: `test_printers_build_and_offer_paper_output` |
| T2 | the kind, descriptions | AC-1 | `paper_kind_tests.cpp`: `ac1_description_states_unit_width_form_drawing_resolution_text_layer` (both test printers), `ac1_conformance_check_finds_a_different_description` |
| T3 | `Paper`: marks, text, reach, cut | AC-2, AC-3, AC-4, AC-5, EC-1 | `paper_tests.cpp`: `ac2_marks_in_order_never_changed` (a stroke's earlier positions stay while it grows), `ac3_each_line_and_each_feed_in_the_text_layer`, `ac4_reach_from_least_to_greatest_mark_or_feed` (a feed without a mark, a feed back below zero), `ac5_cut_empties_and_next_print_starts_a_new_paper`, `ec1_cut_while_printing_puts_later_marks_on_the_new_paper` (TD-20 in a line; TP-4 in a stroke) |
| T4 | `Replies` | AC-23, EC-2 | `replies_tests.cpp`: `ac23_numbers_whole_without_point_else_four_decimals`, `ac23_marks_reply_reach_then_dots_and_strokes`, `ac23_text_reply_one_line_per_text_line`, `ec2_empty_paper_replies_reach_0_0_and_no_lines` |
| T5 | `Picture`, `PngWriter` | AC-16, AC-17 | `picture_tests.cpp`: `ac16_full_width_all_it_reaches_at_stated_resolution` (sizes of TD-20 and TP-4 papers), `ac16_reference_pictures_of_the_test_printers` (`td20.png`, `tp4.png`), `ac16_reference_pictures_of_the_three_printers` (`pc100c`, `ce150`, `ce1600p` fixtures: description + marks → PNG byte for byte; 1311 and 9921 pixels wide, AC-27, AC-35 as data), `ac17_long_paper_lowered_to_16000_pixels_never_cropped`, `ac17_wide_long_paper_lowered_to_64_megapixels`; `formats/tests/png_tests.cpp`: `png_rgb_bands_and_phys` |
| T6 | topics and the part | AC-6, AC-7, EC-2, EC-3, EC-4 | `paper_part_tests.cpp`: `ac6_one_stream_per_printer_type_plugged_or_not`, `ac6_message_carries_appended_marks_lines_and_reach`, `ac6_cut_sends_the_reset`, `ec3_late_listener_gets_whole_paper_once_then_appended` (both test printers), `ac7_same_commands_same_ticks_same_marks` (two sessions, `paper marks` equal), `ec2_product_without_printers_has_no_paper_topic`, `ec4_full_speed_loses_no_mark_shown_at_end_of_call` |
| T7 | what the paper lives through | AC-8, AC-9, EC-7 | `paper_part_tests.cpp`: `ac8_reset_cold_boot_power_cycle_keep_the_paper`, `ac8_detach_keeps_attach_again_prints_on` (on `test-seg`, again on `test-dot` and in variant `r2`), `ac8_machine_replaced_without_printer_keeps_paper`, `ac8_user_preset_naming_printer_starts_empty_other_paper_stays` (A-3, a LIB-6 test preset with both test printers' places), `ac8_startup_preset_at_model_switch_keeps_paper`, `ac8_new_session_starts_with_empty_papers`, `ac9_detach_and_attach_send_nothing_for_papers`, `ac8_attach_in_place_keeps_marks_made_before_the_part_binds` (`PaperOutputBase`), `ec7_detach_in_a_line_keeps_what_was_printed` |
| T8 | `advanceUnpaced` calls `afterCall` (A-6) | AC-6 (support) | `sessions/tests/app_session_tests.cpp`: `advance_unpaced_runs_parts_after_call` |
| T9 | Copy and Cut in the session | AC-16, AC-18, AC-19, EC-5 | `copy_cut_tests.cpp`: `ac16_copy_asks_clipboard_with_picture_and_text` (TD-20: text joined by line ends; TP-4: picture only), `ac18_cut_copies_then_cuts_also_when_copy_failed` (answer not given; no listener offers it), `ac19_empty_paper_copy_and_cut_refused`, `ec5_copy_holds_marks_up_to_the_command`, `ac20_copy_outcome_on_paper_copies` |
| T10 | the step | AC-22 | `paper_step_tests.cpp`: `ac22_cut_paper_cuts_the_plugged_printers_paper_only`, `ac22_without_printer_no_effect_no_failure`, `ac22_while_paused_finishes_at_once`, `ac22_cut_paper_in_the_harness` |
| T11 | harness commands | AC-23, EC-2, EC-6, EC-8 | `harness/tests/paper_commands_tests.cpp`: `ac23_cut_paper_plugged_or_by_name`, `ac23_paper_text_refused_without_text_layer`, `ac23_paper_marks_reply`, `ac23_paper_png_writes_the_picture`, `ac23_paper_copy_and_cut_refused_for_empty_paper`, `ac23_state_reads_what_was_appended`, `ec2_kept_paper_by_name_without_printer_refused_without_name`, `ec6_paper_png_unwritable_path_refused_with_reason`, `ec8_commands_on_a_kept_paper_by_name` |
| T12 | AC-39 replies | AC-39 | `replies_tests.cpp`: `ac39_marks_reply_read_and_written_unchanged` (the three fixtures); products: `products/calc-u-59/tests/paper_replies_tests.cpp` `lib8_ac39_ti_printouts_turned_into_paper_replies`, `products/calc-u-1600/tests/paper_replies_tests.cpp` `lib8_ac39_sharp_plots_turned_into_paper_replies` (committed files equal the converter's output; Frequent) |
| T13 | Qt6 panel | AC-12, AC-13, AC-14, AC-15 | `qt6/tests/paper_panel_tests.cpp`: `ac12_owner_name_whole_width_marks_at_their_position` (rendered with `QWidget::grab`, pixels at dot and stroke positions), `ac12_no_owner_empty_paper_no_name_copy_cut_disabled`, `ac13_follows_within_64_points_stays_when_scrolled_back` (wheel events), `ac13_no_scroll_bar`, `ac14_view_control_shows_view_on_text_selectable_fixed_width`, `ac15_key_down_while_held_switch_shows_position` |
| T14 | Qt6 Copy and Cut | AC-16, AC-18, AC-19, AC-20 | `qt6/tests/paper_panel_tests.cpp`: `ac16_copy_puts_image_and_text_on_clipboard` (`ClipboardService`; image size and resolution), `ac18_cut_empties_and_disables`, `ac19_empty_paper_copy_cut_disabled`, `ac20_toast_for_1_8_seconds` (`Copied to clipboard`; `Copy failed` with a refusing service) |
| T15 | React panel | AC-12–15, AC-19–21 | `library/web/tests/paper.spec.js`: `ac12 owner name and marks at their position`, `ac12 no owner no name copy and cut disabled`, `ac13 follows near the end stays when scrolled back`, `ac14 view control shows the view that is on`, `ac15 printer controls held and switched`, `ac19 empty paper copy and cut disabled`, `ac20 copied to clipboard for 1.8 seconds`, `ac21 picture refused text alone copied` (TD-20), `ac21 picture refused offered as download` (TP-4, `TP-4.png`); Chromium and WebKit |

Coverage: T2 to T15 cover AC-1 to AC-9, AC-12 to AC-23, AC-39 (22 of
22) and EC-1 to EC-8 (8 of 8). AC-12 to AC-15, AC-19 and AC-20 are
shown in both component sets; AC-21 is the Web frontend's. Case names
follow LIB-1 §2a (`acN_`, `ecN_`; `lib8_acN_` in the products).

The product rows' criteria (AC-24 to AC-38) are tested in M-45 and M-53
from these seams: `checkPaperOutput`, the reference pictures (AC-27,
AC-35 by the same files), the reply files (AC-39), and the GUI scenarios
of AC-31 and AC-38 (ADR 0032).

## 7. Tools and agent knowledge

Today's tools are unaffected in M-39: Calc-U-59's
`tools/headless/capi.cpp` printer calls and `tools/agentic/ti59file.py`
(printout) stay until M-59; Calc-U-1600's `CALCU_PAPER_OUT` of
`tools/CliCommon.hpp` until M-51 (LIB-11 product columns). The harness
commands of AC-23 deliver what they do (LIB-11 spec, the tools table).

| Step | Files |
|---|---|
| M-39 | `docs/test-groups.md` (`calcu_qt6_tests`; the new files of `calcu_library_tests`; `paper.spec.js` in `LibraryWebPlaywright`; the two product converter cases); LIB-1 design §2a: the areas table gets `library/paper/` and `library/qt6/`, the list of exceptions gets `calcu_qt6` (D-1) and the include rule `paper`: `machine`, `sessions`, `formats`, `presets`, `startup`; `harness` and `qt6` may include `paper`. `docs/GLOSSARY.md` already has paper panel, mark and text layer |
| M-45 | `products/calc-u-1600/docs/developer/Shared-Architecture.md` (`PlotterPaper`); `docs/glossary-mapping.md`: `PlotterPaperWidget`, `PlotterPaper` (Web), `PaperKind` → paper panel, paper |
| M-53 | `products/calc-u-59/reference/AppArchitecture.md`, `WebClient.md`, `FrontendParity.md`, the `calc-u-59-qt` skill (they name `PrinterWidget`, `PaperView`, `printerCodeLines`, `Printer.jsx`); `docs/glossary-mapping.md`: `PrinterWidget`, `PaperView`, `Printer.jsx`, "printer strip" → paper panel, paper |
| M-58 | `PrinterView.swift` → the Swift paper panel (C59-8), the `calc-u-59-swift` skill |

## 8. Golden runs and risks

- **Golden and reference runs.** M-39 touches no core and no product
  session code. The golden runs (M-8) and the reference runs (M-4, M-9)
  are unchanged and replay as before. The converters read them; they
  write new files and change none.
- **Reference pictures are new.** No old picture can be matched byte
  for byte: today Qt (`QPainter` with anti-aliasing) and the browser
  canvas draw them. The five reference pictures are made by
  `Picture.cpp` in M-39 and looked at once against a Copy of 2.3.0 and
  0.8.1 for the same printout; the qa-report records that look. From
  then on they fix the pixels (TR). A difference afterwards is a
  finding.
- **The same bytes on every platform.** Integer marks and
  `-ffp-contract=off` should give equal pixels on macOS, Linux (x86_64,
  aarch64), Windows and WASM. If CI shows a difference of one grey level
  on one platform, the comparison moves to pixels with a tolerance of one
  level, and the qa-report says so (D-7).
- **Clipboard in a user gesture (WebKit).** The picture is made in the
  worker and the ask reaches the page after the click. `armClipboard()`
  writes inside the gesture with promises, as Calc-U-1600 does today. If
  a browser does not take promises in a `ClipboardItem`, the fallback of
  AC-21 applies; Playwright checks both engines.
- **Memory of long papers.** A 64-megapixel CE-1600P picture is drawn in
  bands (D-7); the PNG of a white roll is small. The papers themselves
  have no limit (LIB-3 D-8), and the stream history holds the paper a
  second time.
- **Qt offscreen clipboard.** If the offscreen platform gives no
  working `QClipboard`, T14 checks the `QMimeData` that
  `ClipboardService` would set, through a seam of the service, and the
  clipboard itself is seen in the products' GUI tests (AC-31, AC-38).
- **Seams of parallel designs** (A-1 to A-7). If LIB-6's, LIB-18's or
  LIB-4's approved designs give these differently, M-39 follows theirs
  and only the calls in `PaperPart` change.

## 9. Decided (best guess)

- **D-1 (component sets).** M-39 creates `library/qt6/` and the static
  target `calcu_qt6`, built only with Qt, in the form LIB-14 gives it
  (its §2.2). It is the fourth exception in LIB-1 §2a's list (after the
  `calcu` CLI, `calcu_web_glue` and the `CALCU_ATTACH` option):
  `calcu_library` must stay free of Qt for the Web and Swift builds.
  The React panel goes to `library/web/components/`, listed in
  `COMPONENT_SOURCES`. M-40 (LIB-10), M-41 (LIB-7) and M-43 (LIB-14)
  add to both.
- **D-2 (the kind pushes).** The printer calls a sink as it prints,
  through `PaperOutputBase`, which keeps the calls while none is
  connected; the paper lives in the session part, not in the machine,
  because it outlives every machine (AC-8). "Cut through the capability" and "the
  capability gives how far it reaches" (AC-4, AC-5) are read as the
  generic kind as a whole: `Paper` holds them.
- **D-3 (coordinates).** Integers in ten-thousandths of the unit. The
  plotters' margin is rounded once to four decimals (66.9474 and
  101.0526 units). The spec's "whole number without a point" is read as
  "a whole number is written without a point"; others get up to four
  decimals.
- **D-4 (colour in replies).** `#rrggbb`, lower case. The spec does not
  say how a colour is written.
- **D-5 (stroke at a cut).** The new paper's stroke starts at the pen's
  position when the pen moves on.
- **D-6 (picture margins).** No margin besides what keeps a mark whole.
  Calc-U-1600's Copy loses the 24 points of white above and below
  (`PaperGeometry::kVerticalInsetPt`); proposed ledger row.
- **D-7 (bands, determinism).** Bands of 256 rows; byte-equal reference
  pictures, with the fallback of §8.
- **D-8 (owner's names in commands).** Matched in any case, as product
  names in the harness (LIB-11 AC-1).
- **D-9 (clipboard shape).** `ClipboardImage` takes `{bytes, text,
  name, dotsPerInch}`: one clipboard entry with both forms (as all
  frontends today), the download name for AC-21. A download counts as
  "Copy failed", since the clipboard took nothing (AC-20). The text
  alone counts as copied (Calc-U-59's Web today).
- **D-10 (one printer per type).** A second paper output of a declared
  type in one machine is logged and unbound. Neither product can build
  one.
- **D-11 (operations).** Reset, Cold boot and a power cycle keep the
  printer object, so its coordinates go on; a product whose printer
  re-zeroes its y at an operation must keep `standing()` continuous.
- **D-12 (stroke width on screen).** The scaled stroke width, at least
  one point (AC-34's "one point wide" is today's width at every panel
  size).
- **D-13 (cut-short TI lines in AC-39).** Open point: the M-9 files have
  no code lines, so the three printer-interrupt examples are compared by
  `paper text` only, their marks through the golden runs. If the user
  wants their `paper marks` too, M-9's driver is run once more at the
  `v2.3.0` pin (a scratch build, the old repository unchanged) with the
  code lines.
- **D-14 (reach of the Sharp references).** Open point: the `.paper`
  files have no feeds, so the converted `reach` is the points' range.
  A new run that reaches further only by a feed (AC-4: blank paper) is
  reported in the qa-report of M-46 as that, not as a finding; the
  marks lines must be equal.
- **D-15 (PC-100C description, open point).** AC-24 gives unit and
  width but not the printable part. The fixture takes the whole width
  (0 to 129.07); M-53 confirms or changes it with the user. The greys
  of the darkness levels are today's `inkAlpha` (`app/PrinterFont.hpp`),
  21 levels for 0 to 20 heating cycles.
