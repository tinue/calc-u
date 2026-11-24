// library/web/components/shellModel.js under
// `node --test` (CTest LibraryWebNodeTests, Frequent): the page-side rules,
// mirroring the C++ tests in library/shell/tests/ case by case, under the
// same names; the hit cases are the C++ ones (library/shell/tests/hit_cases.txt).
const test = require("node:test");
const assert = require("node:assert/strict");
const fs = require("fs");
const path = require("path");
const m = require("../components/shellModel.js");

const FIXTURES = path.join(__dirname, "..", "..", "shell", "tests");

// The About texts of library/shell/tests/TestShellPart.hpp's product().
const product = () => ({
  name: "Test-Product", version: "1.2.3", build: "abc123", aboutLabel: "",
  aboutLines: ["A product made for the tests.", "No code of a product."],
  aboutLinks: [["Source", "https://example.org/source"], ["Privacy", "https://example.org/privacy"]],
});

test("loading_after_500_ms_at_full_speed", () => {
  const r = m.calcuShellLoading();
  r.busy(true, true, 1000);
  assert.equal(r.shown(1000), false);
  assert.equal(r.due(), 1500);
  r.busy(true, true, 1499);
  assert.equal(r.shown(1499), false);
  assert.equal(r.shown(1500), true);
  r.busy(true, false, 1500);
  assert.equal(r.shown(1500), false);
  assert.equal(r.due(), null);
  r.busy(true, true, 1500);
  assert.equal(r.shown(2100), true);
  r.busy(false, false, 2100);
  assert.equal(r.shown(2100), false);
  r.busy(false, true, 2100);
  assert.equal(r.shown(3100), false);
});

test("loading_short_operation_shows_nothing", () => {
  const r = m.calcuShellLoading();
  r.busy(true, true, 0);
  assert.equal(r.shown(450), false);
  r.busy(false, false, 450);
  assert.equal(r.shown(550), false);
});

test("loading_back_to_full_speed_counts_again", () => {
  const r = m.calcuShellLoading();
  r.busy(true, true, 0);
  r.busy(true, false, 400);
  r.busy(true, true, 450);
  assert.equal(r.shown(850), false);
  assert.equal(r.shown(950), true);
});

test("notices_one_after_the_other", () => {
  const q = m.calcuShellNotices();
  assert.equal(q.current(), null);
  q.push("The first went wrong.");
  q.push("The second went wrong.");
  q.push("The first went wrong.");
  assert.equal(q.current().text, "The first went wrong.");
  q.close();
  assert.equal(q.current().text, "The second went wrong.");
  q.close();
  assert.equal(q.current().text, "The first went wrong.");
  q.close();
  assert.equal(q.current(), null);
});

test("notice_over_loading", () => {
  const r = m.calcuShellLoading();
  const q = m.calcuShellNotices();
  r.busy(true, true, 0);
  q.push("A file could not be read.");
  assert.ok(r.shown(600) && q.current());
  q.close();
  assert.ok(r.shown(600));
});

test("refusal_equal_to_last_notice_not_repeated", () => {
  const q = m.calcuShellNotices();
  q.push("The disk is full.");
  q.refusal("The disk is full.");
  assert.equal(q.waiting(), 1);
  q.refusal("Another operation is running.");
  q.refusal("");
  q.refusal("Stopped.");
  assert.equal(q.waiting(), 2);
});

test("asks_refusal_is_a_question_yes_resends_confirmed", () => {
  const q = m.calcuShellNotices();
  const sent = { name: "media.delete", args: { kind: "card", name: "A" } };
  q.refusal("asks: Delete the card A?", sent);
  assert.ok(q.current().question);
  assert.equal(q.current().text, "Delete the card A?");
  assert.deepEqual(q.answer(true), { name: "media.delete", args: { kind: "card", name: "A", confirmed: true } });
  q.refusal("asks: Replace it?", sent);
  assert.equal(q.answer(false), null);
  assert.equal(q.current(), null);
});

// The hit rule, the same cases as the C++ test.
function hitMap() {
  return { keys: [["1", { x: 0, y: 20, w: 10, h: 10 }], ["2", { x: 20, y: 20, w: 10, h: 10 }], ["3", { x: 40, y: 20, w: 10, h: 10 }]],
    display: { x: 0, y: 0, w: 50, h: 15 }, distance: 4 };
}

test("hit_rule_cases_of_the_file", () => {
  let map = null;
  let cases = 0;
  for (const line of fs.readFileSync(path.join(FIXTURES, "hit_cases.txt"), "utf8").split("\n")) {
    const words = line.trim().split(/\s+/);
    if (!words[0] || words[0].startsWith("#")) continue;
    if (words[0] === "map") {
      map = { distance: Number(words[1]), display: null, keys: words.slice(2).map((k) => {
        const [key, nums] = k.split(":");
        const [x, y, w, h] = nums.split(",").map(Number);
        return [key, { x, y, w, h }];
      }) };
    } else if (words[0] === "at") {
      const got = m.calcuShellKeyAt(map, Number(words[1]), Number(words[2]), Number(words[3]));
      assert.equal(got === null ? "-" : got, words[4], line);
      cases += 1;
    }
  }
  assert.ok(cases >= 25);
});

const describeOut = (out) => out.map((c) => `${c.name} ${JSON.stringify(c.args)}`).join(" ");

test("several_pointers_hold_keys", () => {
  const t = m.calcuShellPointer(hitMap(), 1);
  assert.equal(describeOut(t.down(1, 5, 25)), 'screen-key-down {"key":"1","pointer":1}');
  assert.equal(describeOut(t.down(2, 25, 25)), 'screen-key-down {"key":"2","pointer":2}');
  assert.equal(describeOut(t.down(3, 6, 26)), 'screen-key-down {"key":"1","pointer":3}');
  assert.equal(describeOut(t.move(1, 45, 25)), "");
  assert.equal(describeOut(t.up(1)), 'screen-key-up {"key":"1","pointer":1}');
  assert.equal(describeOut(t.up(1)), "");
  assert.equal(describeOut(t.releaseAll()), 'screen-key-up {"key":"2","pointer":2} screen-key-up {"key":"1","pointer":3}');
  const big = m.calcuShellPointer(hitMap(), () => 2);
  assert.equal(describeOut(big.down(7, 50, 50)), 'screen-key-down {"key":"2","pointer":7}');
});

test("display_hold_full_speed_until_up_or_leave", () => {
  const t = m.calcuShellPointer(hitMap(), 1);
  assert.equal(describeOut(t.down(1, 10, 5)), 'speed {"full":true}');
  assert.equal(describeOut(t.move(1, 20, 6)), "");
  assert.equal(describeOut(t.up(1)), 'speed {"full":false}');
  t.down(2, 10, 5);
  assert.equal(describeOut(t.move(2, 10, 40)), 'speed {"full":false}');
  assert.equal(describeOut(t.move(2, 10, 5)), "");
  assert.equal(describeOut(t.up(2)), "");
  assert.equal(describeOut(t.down(3, 10, 5)), 'speed {"full":true}');
  assert.equal(describeOut(t.down(4, 30, 5)), "");
  assert.equal(describeOut(t.up(3)), "");
  assert.equal(describeOut(t.up(4)), 'speed {"full":false}');
});

test("no_key_at_down_nothing_on_move", () => {
  const t = m.calcuShellPointer(hitMap(), 1);
  assert.equal(describeOut(t.down(1, 15, 40)), "");
  assert.equal(describeOut(t.move(1, 5, 25)), "");
  assert.equal(describeOut(t.up(1)), "");
});

// About.
test("about_shows_product_texts", () => {
  const p = product();
  const a = m.calcuShellAbout(p);
  assert.equal(a.title, "About Test-Product");
  assert.equal(a.version, "Version 1.2.3 (abc123)");
  assert.equal(a.lines[0], "A product made for the tests.");
  assert.equal(a.links[1][1], "https://example.org/privacy");
  p.aboutLabel = "About Test-Product…";
  assert.equal(m.calcuShellAbout(p).title, "About Test-Product");
  p.build = "";
  assert.equal(m.calcuShellAbout(p).version, "Version 1.2.3");
});

// The same text as shell::aboutText (help_about_tests.cpp).
test("about_support_rows_and_copied_text", () => {
  const p = product();
  assert.deepEqual(m.calcuShellAbout(p).support, []);
  assert.equal(m.calcuShellAboutText(m.calcuShellAbout(p)),
    "Test-Product\nVersion 1.2.3 (abc123)\nA product made for the tests.\nNo code of a product.\n");
  p.buildTime = "2026-11-24T11:49:08Z";
  const a = m.calcuShellAbout(p, [["OS", "Darwin arm64 27.2.0"], ["Hardware", ""], ["Browser", "Safari 27.2"]]);
  assert.equal(a.support.length, 3);
  assert.equal(m.calcuShellAboutText(a),
    "Test-Product\nVersion 1.2.3 (abc123)\nA product made for the tests.\nNo code of a product.\n\n" +
    "Support Information\nBuilt: 2026-11-24T11:49:08Z\nOS: Darwin arm64 27.2.0\nBrowser: Safari 27.2\n");
  p.package = "DMG";   // first, before Built
  assert.deepEqual(m.calcuShellAbout(p, []).support.map((r) => r[0]), ["Package", "Built"]);
});

test("host_rows_from_the_browser", () => {
  const safari = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/27.2 Safari/605.1.15";
  assert.deepEqual(m.calcuShellHostRows({ userAgent: safari }, null), [["Page", ""], ["Browser", "Safari 27.2"], ["OS", "macOS"], ["Hardware", ""]]);
  const firefox = "Mozilla/5.0 (X11; Linux x86_64; rv:140.0) Gecko/20100101 Firefox/140.0";
  assert.deepEqual(m.calcuShellHostRows({ userAgent: firefox }, null), [["Page", ""], ["Browser", "Firefox 140.0"], ["OS", "Linux"], ["Hardware", ""]]);
  const hints = (platform, platformVersion, architecture) => ({
    platform, platformVersion, architecture, bitness: "64", model: "",
    fullVersionList: [{ brand: "Not)A;Brand", version: "99.0.0.0" }, { brand: "Chromium", version: "150.0.7871.250" },
                      { brand: "Google Chrome", version: "150.0.7871.250" }],
  });
  assert.deepEqual(m.calcuShellHostRows({ userAgent: "" }, hints("Windows", "19.0.0", "x86")),
    [["Page", ""], ["Browser", "Chrome 150.0.7871.250"], ["OS", "Windows 11"], ["Hardware", "x86_64"]]);
  assert.deepEqual(m.calcuShellHostRows({ userAgent: "" }, hints("macOS", "27.2.0", "arm"), { host: "calc-u.org", pathname: "/1600/beta/app/" }),
    [["Page", "calc-u.org/1600/beta/app/"], ["Browser", "Chrome 150.0.7871.250"], ["OS", "macOS 27.2.0"], ["Hardware", "arm64"]]);
  assert.equal(m.calcuShellHostRows({ userAgent: "" }, hints("Windows", "10.0.0", "x86"))[2][1], "Windows 10");
  assert.equal(m.calcuShellHostRows({ userAgent: "" }, null, { host: "", pathname: "/x.html" })[0][1], "");   // a file: no host
});

// The bar below a faceplate (barFit, bar_model_tests.cpp).
test("bar_fit_labels_and_width", () => {
  assert.deepEqual(m.calcuShellBarFit(400, 250, 380), { labels: true, width: 400 });
  assert.deepEqual(m.calcuShellBarFit(300, 250, 380), { labels: false, width: 300 });
  assert.deepEqual(m.calcuShellBarFit(200, 250, 380), { labels: false, width: 250 });
  assert.equal(m.calcuShellBarFit(200, 250, 380, 220).width, 220);
});

// The paper panel's control row (paperRowCompact, chrome_tests.cpp); the
// library's chrome compacts below 280.
test("chrome_paper_row_compact", () => {
  const s = { paperRowCompactWidth: 280 };
  assert.equal(m.calcuShellPaperRowCompact(s, 279), true);
  assert.equal(m.calcuShellPaperRowCompact(s, 280), false);
  assert.equal(m.calcuShellPaperRowCompact(s, 400), false);
  assert.equal(m.calcuShellPaperRowCompact({ paperRowCompactWidth: 0 }, 100), false);
});


// The view rule (shell/View, library/shell/tests/view_tests.cpp): the
// TI-59's tall faceplate with its column's margins and bar, the PC-1500's
// wide one with its bar; a desktop window has the Web's menu bar.
const VIEW_MENU = 26;
const tallView = (width, height, mobile) => ({ width, height, faceplateAspect: 2064 / 4075, fixedWidth: 16,
  fixedHeight: 50 + 16 + (mobile ? 0 : VIEW_MENU), panelsMinWidth: 256, panelsMinHeight: 0, mobile });
const wideView = (width, height, mobile) => ({ width, height, faceplateAspect: 4000 / 1760, fixedWidth: 0,
  fixedHeight: 40 + (mobile ? 0 : VIEW_MENU), panelsMinWidth: 0, panelsMinHeight: 200, mobile,
  faceplateUsableWidth: 600 });

test("view_tall_desktop_ordinary", () => {
  const v = m.calcuShellView(tallView(1400, 900, false));
  assert.equal(v.side, "beside");
  assert.equal(v.panelsFit, true);
  assert.equal(v.panelsAlone, false);
});

test("view_tall_desktop_narrow", () => {
  const v = m.calcuShellView(tallView(400, 900, false));
  assert.equal(v.side, "below");
  assert.equal(v.panelsFit, false);
});

test("view_tall_phone_portrait", () => {
  const v = m.calcuShellView(tallView(393, 852, true));
  assert.equal(v.side, "below");
  assert.equal(v.panelsFit, false);
  assert.equal(v.panelsAlone, false);
});

test("view_tall_phone_landscape", () => {
  const v = m.calcuShellView(tallView(852, 393, true));
  assert.equal(v.side, "beside");
  assert.equal(v.panelsFit, true);
});

test("view_wide_desktop_ordinary", () => {
  const v = m.calcuShellView(wideView(1400, 900, false));
  assert.equal(v.side, "below");
  assert.equal(v.panelsFit, true);
  assert.equal(v.panelsAlone, false);
});

test("view_wide_desktop_narrow", () => {
  const v = m.calcuShellView(wideView(600, 900, false));
  assert.equal(v.side, "below");
  assert.equal(v.panelsFit, true);
});

test("view_wide_phone_portrait", () => {
  const v = m.calcuShellView(wideView(393, 852, true));
  assert.equal(v.side, "below");
  assert.equal(v.panelsAlone, true);
});

test("view_wide_phone_landscape", () => {
  const v = m.calcuShellView(wideView(852, 393, true));
  assert.equal(v.side, "beside");
  assert.equal(v.panelsFit, false);
  assert.equal(v.panelsAlone, false);
});

test("view_wide_phone_landscape_safe_area", () => {
  const v = m.calcuShellView(wideView(838, 440, true));
  assert.equal(v.side, "below");
  assert.equal(v.panelsFit, false);
  assert.equal(v.panelsAlone, false);
});

test("view_wide_tablet_landscape", () => {
  const v = m.calcuShellView(wideView(1366, 1024, true));
  assert.equal(v.side, "below");
  assert.equal(v.panelsFit, true);
  assert.equal(v.panelsAlone, false);
});

test("view_wide_tablet_portrait", () => {
  const v = m.calcuShellView(wideView(1024, 1366, true));
  assert.equal(v.side, "below");
  assert.equal(v.panelsFit, true);
  assert.equal(v.panelsAlone, false);
});

test("view_tall_tablet_landscape", () => {
  const v = m.calcuShellView(tallView(1366, 1024, true));
  assert.equal(v.side, "beside");
  assert.equal(v.panelsFit, true);
});

test("view_faceplate_fitted", () => {
  const box = { width: 1000, height: 540, faceplateAspect: 2, fixedWidth: 0, fixedHeight: 40 };
  let v = m.calcuShellView(box);
  assert.equal(v.faceplateWidth, 1000);
  assert.equal(v.faceplateHeight, 500);
  assert.equal(v.side, "below");
  assert.equal(v.room, 0);
  v = m.calcuShellView(Object.assign(box, { width: 1300 }));
  assert.equal(v.side, "beside");
  assert.equal(v.faceplateWidth, 1000);
  assert.equal(v.room, 300);
});

test("view_no_panels_on_a_side", () => {
  const box = { width: 4000, height: 500, faceplateAspect: 2, panelsMinWidth: 0, panelsMinHeight: 100 };
  assert.equal(m.calcuShellView(box).panelsFit, false);
  assert.equal(m.calcuShellView(Object.assign(box, { panelsMinWidth: 100 })).panelsFit, true);
});

test("view_debugger_fits", () => {
  const debugged = (v, extra) => m.calcuShellView(Object.assign(v, extra)).debuggerFits;
  assert.equal(debugged(tallView(1400, 900, false), { debuggerMinWidth: 220, debuggerMinHeight: 560 }), true);
  assert.equal(debugged(tallView(1400, 500, false), { debuggerMinWidth: 220, debuggerMinHeight: 560 }), false);
  assert.equal(m.calcuShellView(tallView(800, 900, false)).panelsFit, true);
  assert.equal(debugged(tallView(800, 900, false), { debuggerMinWidth: 220 }), false);
  assert.equal(debugged(wideView(1000, 900, false), { debuggerMinWidth: 700 }), true);
  assert.equal(debugged(wideView(600, 900, false), { debuggerMinWidth: 700 }), false);
  const alone = m.calcuShellView(Object.assign(wideView(393, 852, true), { debuggerMinWidth: 700 }));
  assert.equal(alone.panelsAlone, true);
  assert.equal(alone.debuggerFits, false);
});

test("view_panels_are_the_debugger", () => {
  const debugged = (v, extra) => m.calcuShellView(Object.assign(v, extra)).debuggerFits;
  const own = { debuggerMinWidth: 220, debuggerMinHeight: 560, panelsAreDebugger: true };
  assert.equal(debugged(tallView(800, 900, false), { debuggerMinWidth: 220, debuggerMinHeight: 560 }), false);
  assert.equal(debugged(tallView(800, 900, false), own), true);
  assert.equal(m.calcuShellView(Object.assign(tallView(800, 500, false), own)).panelsFit, true);
  assert.equal(debugged(tallView(800, 500, false), own), false);
  assert.equal(debugged(tallView(600, 900, false), own), false);
});
