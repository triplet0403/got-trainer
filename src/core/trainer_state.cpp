#include "core/trainer_state.hpp"

namespace got {

TrainerVars& GetTrainerVars() {
    static TrainerVars vars;
    return vars;
}

TrainerSettings& GetTrainerSettings() {
    static TrainerSettings settings;
    return settings;
}

TrainerActions& GetTrainerActions() {
    static TrainerActions actions;
    return actions;
}

TrainerHotkeys& GetTrainerHotkeys() {
    static TrainerHotkeys hotkeys;
    return hotkeys;
}

}  // namespace got
