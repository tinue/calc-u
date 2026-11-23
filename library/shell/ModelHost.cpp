#include "shell/ModelHost.hpp"
#include "shell/BarModel.hpp"
#include "shell/ValueForms.hpp"
#include <utility>

namespace calcu::shell {

ModelHost::ModelHost(InputNames names) : names_(std::move(names)) {}

void ModelHost::set(ModelSetup setup, const sessions::TopicMirror& states) {
    followProductTopics(inputs_, setup.product);
    setup_ = std::move(setup);
    for (auto& [name, value] : inputs_.productTopics)
        if (const sessions::PayloadPtr* p = states.state(sessions::TopicName{name}))
            value = *p ? (*p)->toValue() : sessions::Value();
}

bool ModelHost::take(const sessions::Message& m) {
    const auto* s = std::get_if<sessions::StateMessage>(&m);
    if (!s || !takeState(inputs_, names_, *s)) return false;
    if (s->topic == names_.settings) presetListWanted_ = true;
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
