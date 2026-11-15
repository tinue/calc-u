#pragma once
#include "web/glue/WorkerHost.hpp"
#include <emscripten/bind.h>

// The embind registration a product's bindings.cpp makes (LIB-15 design
// §3.6): one class, named as the worker config's `host`, built from the
// start object the library worker passes, with the three calls the worker
// makes. All that is left in a product's bindings.cpp:
//
//   CALCU_WEB_PRODUCT(CalcU1600Host, shared::makeWebAppSession)

namespace calcu::web {
template <std::unique_ptr<sessions::AppSession> (*Factory)(const WebStart&)>
WorkerHost* makeWorkerHost(emscripten::val start) {
    return new WorkerHost(AppSessionFactory(Factory), start);
}
} // namespace calcu::web

#define CALCU_WEB_PRODUCT(name, factory)                                                       \
    EMSCRIPTEN_BINDINGS(calcu_web_product_##name) {                                            \
        emscripten::class_<calcu::web::WorkerHost>(#name)                                      \
            .constructor(&calcu::web::makeWorkerHost<&factory>, emscripten::allow_raw_pointers()) \
            .function("failure", &calcu::web::WorkerHost::failure)                             \
            .function("receive", &calcu::web::WorkerHost::receive)                             \
            .function("call", &calcu::web::WorkerHost::call);                                  \
    }
