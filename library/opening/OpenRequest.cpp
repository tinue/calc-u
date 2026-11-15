#include "opening/OpenRequest.hpp"
#include "sessions/Args.hpp"

namespace calcu::opening {

using sessions::Value;

namespace {
struct WayName {
    Way way;
    std::string_view name;
};
constexpr WayName kWays[] = {
    {Way::Drop, "drop"}, {Way::OpenWith, "open-with"}, {Way::Start, "start"}, {Way::Dialog, "dialog"}, {Way::Harness, "harness"}};
} // namespace

std::string_view wayName(Way w) {
    for (const auto& x : kWays)
        if (x.way == w) return x.name;
    return "drop";
}

std::optional<Way> wayNamed(std::string_view n) {
    for (const auto& x : kWays)
        if (x.name == n) return x.way;
    return std::nullopt;
}

std::string_view openedAsName(OpenedAs a) {
    switch (a) {
    case OpenedAs::Preset: return "preset";
    case OpenedAs::Medium: return "medium";
    case OpenedAs::FileKind: return "file-kind";
    case OpenedAs::Kept: return "kept";
    }
    return "preset";
}

Value OpenReply::toValue() const {
    return Value::object({{"as", std::string(openedAsName(as))}, {"file", file}, {"detail", detail}});
}

Value OpenRequest::toValue() const {
    Value::Array list;
    for (const OpenedFile& f : files) {
        Value::Object o{{"name", Value(f.name)}};
        if (f.path) o.push_back({"path", Value(*f.path)});
        if (f.bytes) o.push_back({"bytes", Value(Value::Bytes(*f.bytes))});
        list.push_back(Value(std::move(o)));
    }
    return Value::object({{"files", Value(std::move(list))},
                          {"way", std::string(wayName(way))},
                          {"upload", upload},
                          {"confirmed", confirmed}});
}

std::optional<OpenRequest> OpenRequest::fromValue(const Value& v, std::string* why) {
    OpenRequest r;
    const Value* files = v.find("files");
    if (!files || !files->elements() || files->elements()->empty()) {
        if (why) *why = "The open command needs files: [{name, path?, bytes?}].";
        return std::nullopt;
    }
    for (const Value& f : *files->elements()) {
        OpenedFile o;
        o.name = sessions::textArg(f, "name");
        if (const Value* p = f.find("path"); p && p->string()) o.path = *p->string();
        if (auto b = sessions::bytesArg(f, "bytes")) o.bytes = std::move(*b);
        if (o.name.empty() && !o.path) {
            if (why) *why = "A file of the open command needs a name or a path.";
            return std::nullopt;
        }
        r.files.push_back(std::move(o));
    }
    const std::string way = sessions::textArg(v, "way");
    const auto w = way.empty() ? std::optional<Way>(Way::Drop) : wayNamed(way);
    if (!w) {
        if (why) *why = "There is no way in " + way + ": drop, open-with, start, dialog or harness.";
        return std::nullopt;
    }
    r.way = *w;
    r.upload = sessions::flagArg(v, "upload");
    r.confirmed = sessions::flagArg(v, "confirmed");
    return r;
}

} // namespace calcu::opening
