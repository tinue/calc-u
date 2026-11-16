#pragma once
#include <functional>
#include <optional>
#include <string>
#include <vector>

// The file named when the Qt6 program is started (LIB-13 AC-21, OQ-2;
// Part D, D4.4, D-14): the last argument after the program's own name that
// names an existing file. Options, their values and words that name no file
// are passed over (the system or an IDE may add some:
// `-NSDocumentRevisionsDebugMode YES`). Qt-free, so the tests reach it.

namespace shared {

std::optional<std::string> startFile(const std::vector<std::string>& arguments,
                                     const std::function<bool(const std::string&)>& isFile);

} // namespace shared
