#include "typing/Awaitable.hpp"
#include "sessions/LibraryTopics.hpp"
#include "sessions/SessionPart.hpp"

namespace calcu::typing {

namespace {

sessions::Task<TypeResult> waitFor(sessions::OperationContext& ctx, TypingEngine& engine, Queued q) {
    if (q.refused) co_return TypeResult{false, *q.refused};
    std::optional<TypeResult> result;
    for (;;) {
        if (!result) result = engine.takeResult(q.token);
        if (result && engine.idle()) co_return *result;
        const machine::Machine* m = ctx.session().machine();
        if (!m) co_return TypeResult{false, sessions::kNoMachine};
        co_await ctx.run(sessions::OperationContext::runPiece(m->descriptor().tickRate));
    }
}

} // namespace

sessions::Task<TypeResult> typeAndWait(sessions::OperationContext& ctx, TypingEngine& engine,
                                       std::vector<Press> presses, TypeOptions o) {
    const Queued q = engine.typeKeys(std::move(presses), o.origin, o.firstPressAtOnce);
    co_return co_await waitFor(ctx, engine, q);
}

sessions::Task<TypeResult> typeTextAndWait(sessions::OperationContext& ctx, TypingEngine& engine, std::string text,
                                           bool enter, TypeOptions o) {
    const Queued q = engine.typeText(text, enter, true, o.origin, o.firstPressAtOnce);
    co_return co_await waitFor(ctx, engine, q);
}

} // namespace calcu::typing
