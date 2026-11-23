#include "shell/ModelHost.hpp"
#include "shell/BarModel.hpp"
#include "shell/ValueForms.hpp"
#include <utility>

namespace calcu::shell {

ModelHost::ModelHost(InputNames names) : names_(std::move(names)) {}

void ModelHost::set(ModelSetup setup) { setup_ = std::move(setup); }

bool ModelHost::take(const sessions::Message& m) {
    const auto* s = std::get_if<sessions::StateMessage>(&m);
    if (!s) return false;
    sessions::Value* input = nullptr;
    // `busy` is left out: no model reads it (the page keeps "Loading…" on its own clock).
    if (s->topic == names_.controlState) input = &inputs_.controlState;
    else if (s->topic == names_.runState) input = &inputs_.runState;
    else if (s->topic == names_.romsMissing) input = &inputs_.romsMissing;
    else if (s->topic == names_.settings) input = &inputs_.settings;
    else if (s->topic == names_.media) input = &inputs_.media;
    else if (setup_)   // a topic the product's items follow
        for (const auto& t : productTopics(setup_->product))
            if (s->topic == t) input = &inputs_.productTopics[t.value];
    if (!input) return false;
    sessions::Value v = s->state ? s->state->toValue() : sessions::Value();
    if (v == *input) return false;
    if (input == &inputs_.settings) presetListWanted_ = true;
    *input = std::move(v);
    return true;
}

void ModelHost::takePresetList(const sessions::ReplyMessage& r) {
    inputs_.presetList = r.done && r.result ? r.result->toValue() : sessions::Value();
}

bool ModelHost::presetListWanted() { return std::exchange(presetListWanted_, false); }

sessions::Value ModelHost::value() const {
    if (!setup_) return {};
    const ShellProduct& p = setup_->product;
    const Traits t = setup_->traits;
    return sessions::Value::object({{"menus", toValue(menus(inputs_, p, t))},
                                    {"bar", toValue(bar(inputs_, p, t))},
                                    {"panels", toValue(panels(inputs_, p))},
                                    {"dialog", toValue(dialog(inputs_.settings, inputs_.presetList, setup_->spec))}});
}

} // namespace calcu::shell
