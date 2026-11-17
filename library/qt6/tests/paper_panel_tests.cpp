// LIB-8 T13, T14 (design §6): the paper panel of the Qt6 component set and
// its clipboard service, on the offscreen platform, driven by a real app
// session with LIB-8's PaperPart and the test printers TD-20 (test-seg
// `printer`, online) and TP-4 (`aux`) through a listener, as a frontend
// hosts them (AC-12 to AC-16, AC-18 to AC-20).
#include "TestHarness.hpp"
#include "TestMachines.hpp"
#include "TestProductMachines.hpp"
#include "paper/PaperPart.hpp"
#include "paper/Picture.hpp"
#include "peripherals/PeripheralsPart.hpp"
#include "qt6/ClipboardService.hpp"
#include "qt6/MacClipboard.h"
#include "qt6/PaperPanel.hpp"
#include "qt6/PaperPainter.hpp"
#include "sessions/AppSession.hpp"
#include "sessions/Listener.hpp"
#include "sessions/MachineSession.hpp"
#include <QApplication>
#include <QClipboard>
#include <QElapsedTimer>
#include <QImage>
#include <QLabel>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QTimer>
#include <QToolButton>
#include <QWheelEvent>
#include <cmath>
#include <memory>

namespace {

using namespace calcu;
using namespace calcu::sessions;
namespace mt = calcu::machine::test;
using calcu::qt6::PaperPanel;

const machine::PlacePath kPrinterPlace{{machine::PlaceName{"printer"}}};
const machine::PlacePath kAuxPlace{{machine::PlaceName{"aux"}}};

void settle() {
    for (int i = 0; i < 5; ++i) QCoreApplication::processEvents();
}

// One window: the session in virtual time, its listener forwarding every
// message to the panel and answering the clipboard asks after each call.
struct PanelRig {
    peripherals::PeripheralsPart peripherals;
    startup::test::TestProductMachines machines;
    startup::MachineControl control{machines, nullptr};   // no build: no preset's origin (A-3)
    paper::PaperPart paperPart{mt::paperTestCatalogue(),
                               {paper::PrinterPaper{mt::kDotter, mt::td20Description()},
                                paper::PrinterPaper{mt::kPlotter, mt::tp4Description()}},
                               control};
    std::unique_ptr<AppSession> session;
    Listener listener;
    PaperPanel panel;
    qt6::ClipboardService clip;
    std::vector<ServiceAsk> asks;
    std::vector<std::unique_ptr<QMimeData>> entries;   // what the clipboard got
    bool refuse = false;                               // the clipboard takes nothing
    mt::TestMachine* m = nullptr;

    PanelRig() {
        AppSessionSetup s;
        s.clock = [] { return pacing::WallTime{0}; };
        s.hostTime = [] { return std::chrono::system_clock::time_point{}; };
        s.parts = {&peripherals, &paperPart};
        s.hosting = Hosting::VirtualTime;
        session = std::make_unique<AppSession>(std::move(s));
        listener.onMessage = [this](const Message& msg) {
            panel.deliver(msg);
            if (const auto* a = std::get_if<ServiceAsk>(&msg)) asks.push_back(*a);
        };
        clip.setSink([this](QMimeData* d) { entries.emplace_back(d); });
        panel.setSender([this](const CommandName& n, Value args) {
            listener.send(n, std::move(args));
            answerAsks();
        });
        listener.listen(*session, HostServices{{HostService::ClipboardImage, HostService::ClipboardText}});
        panel.resize(400, 300);
        panel.show();
        settle();
    }

    void build() {
        const machine::Catalogue& cat = mt::paperTestCatalogue();
        mt::TestBuilder b;
        auto dotter = mt::peripheralAt(cat, "printer", mt::kDotter);
        dotter.switches = {machine::SwitchPosition{machine::ControlId{"online"}, true}};
        auto plotter = mt::peripheralAt(cat, "aux", mt::kPlotter);
        machine::BuildResult r = machine::build(cat, b, mt::requestFor(cat, mt::kSeg, std::nullopt, {dotter, plotter}));
        m = static_cast<mt::TestMachine*>(r.machine.get());
        session->machineSession().setMachine(std::move(r.machine), {}, 0);
        session->advanceUnpaced(1);
        settle();
    }
    // A key pressed on the machine and the printing it starts.
    void press(const char* key, machine::Ticks ticks = 200) {
        m->keyDown(machine::KeyId{key});
        session->advanceUnpaced(ticks);
        m->keyUp(machine::KeyId{key});
        session->advanceUnpaced(1);
        settle();
    }
    void answerAsks() {
        std::vector<ServiceAsk> now;
        now.swap(asks);
        for (const auto& a : now) {
            ServiceAnswer ans = refuse ? ServiceAnswer{false, Value(), "refused"} : clip.answer(a);
            session->answer(a.ask, std::move(ans));
        }
        settle();
    }
    void own(const machine::PeripheralId& id, const machine::PlacePath& place) {
        panel.setOwner(id);
        panel.setControls(peripherals::controlsOf(*m, machine::PeripheralOwner{id, place}));
        settle();
    }
    const paper::Paper& paperOf(std::size_t i) const { return *paperPart.papers()[i].second; }
    template <class W> W* child(const char* name) { return panel.findChild<W*>(QString::fromLatin1(name)); }
};

QImage grabView(PanelRig& r) {
    return r.panel.paperView()->viewport()->grab().toImage().convertToFormat(QImage::Format_RGB32);
}

// The point in the view's viewport of a paper position.
QPoint pointOf(PanelRig& r, const paper::Paper& p, paper::Position at) {
    const qt6::PaperExtent e = qt6::paperExtent(p.description(), p.marks(), p.reach());
    const double k = r.panel.paperView()->pointsPerCoord();
    return QPoint(static_cast<int>(std::floor(static_cast<double>(at.x) * k)),
                  static_cast<int>(std::floor(PaperPanel::kBlankPoints - r.panel.paperView()->scrollPosition() +
                                              static_cast<double>(at.y - e.top) * k)));
}

} // namespace

TEST(ac12_owner_name_whole_width_marks_at_their_position) {
    PanelRig r;
    r.build();
    r.press("1");   // TD-20 prints a line; TP-4 draws a stroke in colour 1 (blue)
    r.own(mt::kDotter, kPrinterPlace);
    CHECK_EQ(r.child<QLabel>("paper-owner")->text().toStdString(), std::string("TD-20"));
    // The whole width of the paper over the width of the panel.
    const double k = r.panel.paperView()->pointsPerCoord();
    CHECK(std::abs(k * static_cast<double>(mt::td20Description().width) - r.panel.paperView()->viewport()->width()) < 0.5);
    const paper::Paper& dots = r.paperOf(0);
    const paper::Dot* dot = nullptr;
    for (const auto& mk : dots.marks())
        if (const auto* d = std::get_if<paper::Dot>(&mk); d && d->darkness == 1) {
            dot = d;
            break;
        }
    CHECK(dot != nullptr);
    if (!dot) return;
    QImage img = grabView(r);
    const QColor atDot = img.pixelColor(pointOf(r, dots, dot->at));
    CHECK(atDot.red() < 80 && atDot.green() < 80 && atDot.blue() < 80);   // grey 0 of darkness 1
    CHECK(img.pixelColor(img.width() - 2, 2) == QColor(Qt::white));       // no mark in the right margin

    r.own(mt::kPlotter, kAuxPlace);
    CHECK_EQ(r.child<QLabel>("paper-owner")->text().toStdString(), std::string("TP-4"));
    const paper::Paper& plot = r.paperOf(1);
    CHECK(!plot.marks().empty());
    if (plot.marks().empty()) return;
    const auto& stroke = std::get<paper::Stroke>(plot.marks().front());
    CHECK(stroke.colour == (paper::Rgb{0, 0, 255}));
    img = grabView(r);
    const QColor atStroke = img.pixelColor(pointOf(r, plot, stroke.path.front()));
    CHECK(atStroke.blue() > 150 && atStroke.red() < 150);
}

TEST(ac12_no_owner_empty_paper_no_name_copy_cut_disabled) {
    PanelRig r;
    r.build();
    r.press("1");
    r.own(mt::kDotter, kPrinterPlace);
    CHECK(r.child<QToolButton>("paper-copy")->isEnabled());
    r.panel.setOwner(std::nullopt);
    settle();
    CHECK(r.child<QLabel>("paper-owner")->text().isEmpty());
    CHECK(!r.child<QToolButton>("paper-copy")->isEnabled());
    CHECK(!r.child<QToolButton>("paper-cut")->isEnabled());
    const QImage img = grabView(r);
    bool white = true;
    for (int y = 0; y < img.height(); y += 7)
        for (int x = 0; x < img.width(); x += 7) white = white && img.pixelColor(x, y) == QColor(Qt::white);
    CHECK(white);   // an empty paper
    // The paper is kept: the printer as owner again shows it.
    r.own(mt::kDotter, kPrinterPlace);
    CHECK(r.child<QToolButton>("paper-copy")->isEnabled());
}

TEST(ac13_follows_within_64_points_stays_when_scrolled_back) {
    PanelRig r;
    r.build();
    r.own(mt::kDotter, kPrinterPlace);
    qt6::PaperView* view = r.panel.paperView();
    for (int i = 0; i < 8; ++i) r.press("1");   // 8 lines of 7 units, 70 points each
    CHECK(view->scrollEnd() > 0);
    CHECK_EQ(view->scrollPosition(), view->scrollEnd());   // followed
    // The wheel scrolls back further than 64 points.
    QWidget* vp = view->viewport();
    for (int i = 0; i < 6; ++i) {
        QWheelEvent wheel(QPointF(10, 10), vp->mapToGlobal(QPointF(10, 10)), QPoint(), QPoint(0, 120), Qt::NoButton,
                          Qt::NoModifier, Qt::NoScrollPhase, false);
        QCoreApplication::sendEvent(vp, &wheel);
    }
    settle();
    const int back = view->scrollPosition();
    CHECK(view->scrollEnd() - back > PaperPanel::kFollowPoints);
    r.press("1");
    CHECK_EQ(view->scrollPosition(), back);   // stays
    // Within 64 points of the end, it follows again.
    view->verticalScrollBar()->setValue(view->scrollEnd() - 40);
    r.press("1");
    CHECK_EQ(view->scrollPosition(), view->scrollEnd());
}

TEST(ac13_no_scroll_bar) {
    PanelRig r;
    r.build();
    r.own(mt::kDotter, kPrinterPlace);
    for (int i = 0; i < 8; ++i) r.press("1");
    qt6::PaperView* view = r.panel.paperView();
    CHECK(view->verticalScrollBarPolicy() == Qt::ScrollBarAlwaysOff);
    CHECK(view->horizontalScrollBarPolicy() == Qt::ScrollBarAlwaysOff);
    CHECK(!view->verticalScrollBar()->isVisible());
    CHECK(view->scrollEnd() > 0);   // there is something to scroll
}

TEST(ac14_view_control_shows_view_on_text_selectable_fixed_width) {
    PanelRig r;
    r.build();
    r.press("1");
    r.press("2");
    r.own(mt::kDotter, kPrinterPlace);
    auto* toggle = r.child<QToolButton>("paper-view-toggle");
    auto* text = r.child<QPlainTextEdit>("paper-text");
    CHECK(toggle->isVisible());
    CHECK(!r.panel.showsText());
    CHECK(!toggle->isChecked());
    CHECK_EQ(toggle->text().toStdString(), std::string("Marks"));   // the view that is on
    toggle->click();
    settle();
    CHECK(r.panel.showsText());
    CHECK(toggle->isChecked());
    CHECK_EQ(toggle->text().toStdString(), std::string("Text"));
    CHECK_EQ(text->toPlainText().toStdString(), std::string("KEY 1   \nKEY 2   "));
    CHECK(text->isReadOnly());
    CHECK((text->textInteractionFlags() & Qt::TextSelectableByMouse) != 0);
    // A fixed-width font asked for (what the platform resolves it to is its own;
    // the offscreen platform of the tests may have no monospaced family).
    CHECK(text->font().fixedPitch());
    CHECK(text->font().styleHint() == QFont::TypeWriter);
    toggle->click();
    settle();
    CHECK(!r.panel.showsText());
    // A paper without a text layer has no such control.
    r.own(mt::kPlotter, kAuxPlace);
    CHECK(!toggle->isVisible());
    CHECK(!r.panel.showsText());
}

TEST(ac15_key_down_while_held_switch_shows_position) {
    PanelRig r;
    r.build();
    r.own(mt::kDotter, kPrinterPlace);
    auto* feed = r.child<QToolButton>("paper-control-feed");
    auto* online = r.child<QToolButton>("paper-control-online");
    CHECK(feed && online);
    if (!feed || !online) return;
    CHECK(!feed->isCheckable());
    CHECK(online->isCheckable());
    CHECK(online->isChecked());   // the switch at its position: on
    const auto controls = [&] { return peripherals::controlsOf(*r.m, machine::PeripheralOwner{mt::kDotter, kPrinterPlace}); };
    const auto isOn = [&](const char* id) {
        for (const auto& c : controls())
            if (c.id.value == id) return c.on;
        return false;
    };
    // The key: down while held, sent as set-control.
    emit feed->pressed();
    r.panel.setControls(controls());
    CHECK(isOn("feed"));
    CHECK(feed->isDown());
    emit feed->released();
    r.panel.setControls(controls());
    CHECK(!isOn("feed"));
    CHECK(!feed->isDown());
    // The switch: a click sets it; the button shows where it stands.
    online->click();
    r.panel.setControls(controls());
    CHECK(!isOn("online"));
    CHECK(!online->isChecked());
}

TEST(ac16_copy_puts_image_and_text_on_clipboard) {
    PanelRig r;
    r.build();
    r.press("1");
    r.own(mt::kDotter, kPrinterPlace);
    r.child<QToolButton>("paper-copy")->click();
    settle();
    CHECK_EQ(r.entries.size(), std::size_t(1));
    if (r.entries.empty()) return;
    const QMimeData* d = r.entries.front().get();
    CHECK(d->hasImage());
    const QImage img = qvariant_cast<QImage>(d->imageData());
    const paper::PictureSize size = paper::pictureSize(r.paperOf(0));
    CHECK_EQ(img.width(), size.width);
    CHECK_EQ(img.height(), size.height);
    CHECK_EQ(img.dotsPerMeterX(), static_cast<int>(std::lround(300 / 0.0254)));
    CHECK_EQ(d->text().toStdString(), std::string("KEY 1   "));
    // The plotter's paper: the picture only.
    r.own(mt::kPlotter, kAuxPlace);
    r.child<QToolButton>("paper-copy")->click();
    settle();
    CHECK_EQ(r.entries.size(), std::size_t(2));
    if (r.entries.size() == 2) {
        CHECK(r.entries[1]->hasImage());
        CHECK(!r.entries[1]->hasText());
    }
    // The service on the application's clipboard itself (offscreen: in process).
    qt6::ClipboardService onClipboard;
    const ServiceAnswer a = onClipboard.answer(ServiceAsk{1, HostService::ClipboardImage,
                                                          Value::object({{"bytes", paper::picturePng(r.paperOf(0))},
                                                                         {"text", "KEY 1   "},
                                                                         {"name", "TD-20.png"},
                                                                         {"dotsPerInch", 300.0}})});
    CHECK(a.given);
    const QMimeData* c = QGuiApplication::clipboard()->mimeData();
    CHECK(c && c->hasImage());
    if (c && c->hasImage()) CHECK_EQ(qvariant_cast<QImage>(c->imageData()).width(), size.width);
    // Text alone.
    CHECK(onClipboard.answer(ServiceAsk{2, HostService::ClipboardText, Value::object({{"text", "abc"}})}).given);
    const ServiceAnswer read = onClipboard.answer(ServiceAsk{3, HostService::ClipboardText, Value::object({})});
    CHECK(read.given && read.result.find("text") && *read.result.find("text")->string() == "abc");
}

TEST(ac18_cut_empties_and_disables) {
    PanelRig r;
    r.build();
    r.press("1");
    r.own(mt::kDotter, kPrinterPlace);
    auto* cut = r.child<QToolButton>("paper-cut");
    CHECK(cut->isEnabled());
    cut->click();
    settle();
    CHECK_EQ(r.entries.size(), std::size_t(1));   // copied first
    CHECK(r.paperOf(0).empty());
    CHECK(!cut->isEnabled());
    CHECK(!r.child<QToolButton>("paper-copy")->isEnabled());
    // Cut also when the copy did not succeed.
    r.press("2");
    CHECK(cut->isEnabled());
    r.refuse = true;
    cut->click();
    settle();
    CHECK(r.paperOf(0).empty());
    CHECK(!cut->isEnabled());
}

TEST(ac19_empty_paper_copy_cut_disabled) {
    PanelRig r;
    r.build();
    r.own(mt::kDotter, kPrinterPlace);
    CHECK_EQ(r.child<QLabel>("paper-owner")->text().toStdString(), std::string("TD-20"));
    CHECK(!r.child<QToolButton>("paper-copy")->isEnabled());
    CHECK(!r.child<QToolButton>("paper-cut")->isEnabled());
    r.press("1");
    CHECK(r.child<QToolButton>("paper-copy")->isEnabled());
    CHECK(r.child<QToolButton>("paper-cut")->isEnabled());
}

TEST(ac20_toast_for_1_8_seconds) {
    PanelRig r;
    r.build();
    r.press("1");
    r.own(mt::kDotter, kPrinterPlace);
    auto* toast = r.child<QLabel>("paper-toast");
    auto* timer = r.child<QTimer>("paper-toast-timer");
    CHECK(!toast->isVisible());
    r.child<QToolButton>("paper-copy")->click();
    settle();
    CHECK(toast->isVisible());
    CHECK_EQ(toast->text().toStdString(), std::string("Copied to clipboard"));
    CHECK_EQ(timer->interval(), 1800);
    CHECK(timer->isActive());
    // A clipboard that takes nothing.
    r.refuse = true;
    r.child<QToolButton>("paper-copy")->click();
    settle();
    CHECK_EQ(toast->text().toStdString(), std::string("Copy failed"));
    CHECK(toast->isVisible());
    // Gone after 1.8 s, and not before.
    QElapsedTimer t;
    t.start();
    while (toast->isVisible() && t.elapsed() < 3000) QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    CHECK(!toast->isVisible());
    CHECK(t.elapsed() >= 1700);
}

#ifdef Q_OS_MACOS
// LIB-8 design §10.6, P16-9 (AC-35, the platform part; D-17): on macOS the
// picture goes through Cocoa with its physical size, which Qt's clipboard
// loses. On a pasteboard of the test's own, not the user's clipboard.
TEST(ac16_mac_clipboard_keeps_physical_size) {
    QImage image(9921, 120, QImage::Format_RGB32);   // a CE-1600P paper, 210 mm at 1200 dpi
    image.fill(Qt::white);
    const char* board = "calcu.qt6.tests.mac-clipboard";
    CHECK(calcu::qt6::macSetClipboardImage(image, 9921 * 72.0 / 1200, 120 * 72.0 / 1200, QString(), board));
    double wPt = 0, hPt = 0;
    int wPx = 0, hPx = 0;
    CHECK(calcu::qt6::macPasteboardImageSize(board, &wPt, &hPt, &wPx, &hPx));
    CHECK(std::fabs(wPt - 210.0 / 25.4 * 72.0) < 0.1);   // 210 mm in points
    CHECK(std::fabs(hPt - 7.2) < 0.01);
    CHECK_EQ(wPx, 9921);
    CHECK_EQ(hPx, 120);
}
#endif
