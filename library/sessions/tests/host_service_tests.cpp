// Host services.
#include "TestHarness.hpp"
#include "TestParts.hpp"

using namespace calcu::sessions;
using namespace calcu::sessions::test;

namespace {
Value askFor(HostService s) { return Value::object({{"service", static_cast<int>(s)}}); }

RigOptions withPicker() {
    RigOptions o;
    o.services.offered = {HostService::FilePicker, HostService::Haptics};
    return o;
}
} // namespace

TEST(listener_states_services) {
    Rig r(withPicker());
    SessionContext& c = r.session->context();
    CHECK(c.hostOffers(HostService::FilePicker));
    CHECK(c.hostOffers(HostService::Haptics));
    CHECK(!c.hostOffers(HostService::ClipboardText));
    CHECK(!c.hostOffers(HostService::AudioSink));
    TestFrontend harness;   // an attached harness states none
    r.session->listen(harness, {});
    CHECK(c.hostOffers(HostService::FilePicker));
    r.session->stopListening(r.id);
    CHECK(!c.hostOffers(HostService::FilePicker));
}

TEST(one_ask_one_answer_session_runs_meanwhile) {
    Rig r(withPicker());
    TestFrontend harness;
    r.session->listen(harness, {});
    r.build();
    const CallId cmd = r.send("tp.ask", askFor(HostService::FilePicker));
    CHECK(r.fe.reply(cmd)->done);
    const CallId askId = static_cast<CallId>(payloadAs<TestValue>(r.fe.reply(cmd)->result)->v);
    CHECK_EQ(r.fe.count<ServiceAsk>(), 1);   // one message, to the frontend that offers it
    CHECK_EQ(harness.count<ServiceAsk>(), 0);
    const ServiceAsk* ask = nullptr;
    for (const auto& m : r.fe.log)
        if (const auto* a = std::get_if<ServiceAsk>(&m)) ask = a;
    CHECK(ask && ask->ask == askId && ask->service == HostService::FilePicker);
    // It goes on running while it waits.
    const Ticks t = r.ticks();
    for (int i = 0; i < 3; ++i) r.call();
    CHECK(r.ticks() > t);
    // One answer; its effect goes out at once (a flush).
    const std::size_t mark = r.fe.log.size();
    r.session->answer(askId, ServiceAnswer{true, Value("/tmp/a.yaml"), {}});
    CHECK_EQ(r.fe.stateMessages(r.part.stateName(), mark), 1);
    CHECK_EQ(r.part.answers.size(), std::size_t(1));
    CHECK(*r.part.answers[0].result.string() == "/tmp/a.yaml");
    r.session->answer(askId, ServiceAnswer{false, {}, "again"});   // a second answer is ignored
    CHECK_EQ(r.part.answers.size(), std::size_t(1));
}

TEST(not_busy_while_asking) {
    Rig r(withPicker());
    r.build();
    r.send("tp.ask", askFor(HostService::FilePicker));
    CHECK(!r.session->busy());
    CHECK(!r.fe.busy()->operationRuns);
    r.part.operation = runningFor(1'000);
    const CallId op = r.send("tp.op");   // an operation may start meanwhile
    r.callWhileBusy();
    CHECK(r.fe.reply(op) && r.fe.reply(op)->done);
}

TEST(service_not_offered_not_asked) {
    Rig r(withPicker());
    const CallId cmd = r.send("tp.ask", askFor(HostService::ClipboardImage));
    CHECK(!r.fe.reply(cmd)->done);   // the part learns nobody offers it
    CHECK_EQ(r.fe.count<ServiceAsk>(), 0);
    CHECK(!r.session->context().askHost(HostService::OpenUrl, Value(), nullptr).has_value());
}
