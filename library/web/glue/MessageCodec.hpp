#pragma once
#include "sessions/HostServices.hpp"
#include "sessions/Messages.hpp"
#include "sessions/Value.hpp"
#include <emscripten/val.h>
#include <optional>
#include <string>
#include <string_view>

// The JS form of the session's messages: a Message as
// a plain object for postMessage, and the arguments the page sends back. The
// codec translates every payload through Payload::toValue() without reading
// it: a frontend only translates.
//
//   worker → page  {kind: "state", topic, value}
//                  {kind: "stream", topic, reset, value: [item, ...]}
//                  {kind: "reply", call, done, reason, value}
//                  {kind: "ask", call, name (the host service), value (its arguments)}
//                  {kind: "sound-start", value: sample rate}
//                  {kind: "sound", value: Int16Array}
//                  {kind: "sound-stop"}
//   page → worker  {kind: "command" | "request", name, args, call}
//                  {kind: "answer", call, done, reason, value}
//                  {kind: "listen", args: {services: [name, ...]}}
//
// Bytes are Uint8Array; their buffers are added to `transfer`, so a
// frame crosses to the page without a copy.

namespace calcu::web {

// Host services by name: sessions::serviceName, sessions::serviceNamed.

emscripten::val toJs(const sessions::Value&, emscripten::val transfer);
sessions::Value fromJs(const emscripten::val&);

emscripten::val encode(const sessions::Message&, emscripten::val transfer);

} // namespace calcu::web
