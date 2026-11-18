#pragma once
#include <functional>
#include <optional>
#include <string>
#include <vector>

// The file named when a desktop program is started (both products): the
// last argument after the program's own name that names an existing
// file. Options, their values and words that name no file are passed over
// (the system or an IDE may add some: `-NSDocumentRevisionsDebugMode YES`).
// Toolkit-free, so the tests reach it; the frontend opens it with
// `open {way: start}`.

namespace calcu::opening {

std::optional<std::string> startFile(const std::vector<std::string>& arguments,
                                     const std::function<bool(const std::string&)>& isFile);

} // namespace calcu::opening
