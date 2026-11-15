#pragma once
#include "TestMachines.hpp"
#include "media/Kind.hpp"
#include <optional>
#include <string>
#include <vector>

// The kinds of medium built for the tests (LIB-7 design §2, §6): a YAML-like
// codec of their own, one `key: value` per line, version 2 current and 1
// outdated:
//
//   format: test-disk        (test-card, test-disk, test-module)
//   format-version: 2
//   name: Work               the name tag; optional (AC-2)
//   description: …
//   template: true           optional
//   write-protect: false     test-module: the switch (always has one)
//   companion: Work.dat      optional: the content after "+" lives there
//   saved: …                 written by the codec
//   content: abc             the content's bytes (TestMediumContent)
//
//   test-card    `.testcard.yaml`, the pass-through `reader` of test-seg,
//                sides 0 and 1, the implicit template `Fresh` (content ""),
//                in-file media; steps test-card: (needs time), test-arm:;
//                saveas: { test-card: … }; the header key test-cards: of the
//                media tests defines in-file cards (C59-7's
//                magnetic-cards: in the library's form).
//   test-disk    `.testdisk.yaml`, the holding `drive` of test-dot and
//                `hub-disk` of test-hub; step test-disk:, eject: disk,
//                saveas: { place: disk }; a companion file when the content
//                holds "+".
//   test-module  `.testmodule.yaml`, held by a module slot: the medium of
//                test-memory-module, with the write-protect switch.

namespace calcu::media::test {

namespace mt = calcu::machine::test;

// The bundled folders of the kinds (empty: none).
struct TestKindFolders {
    std::string disks, modules, cards;
};
std::vector<KindDecl> testKinds(const TestKindFolders& = {});

// A file of a test kind, as the codec reads it.
struct TestFile {
    std::string format = "test-disk";
    int version = 2;
    std::string name, description, content;
    bool isTemplate = false;
    std::optional<bool> writeProtect;
    std::string companion;
    std::string text() const;
};
std::string contentOf(const Content&);   // the bytes of a TestMediumContent, "" otherwise
Content contentFrom(std::string bytes);

} // namespace calcu::media::test
