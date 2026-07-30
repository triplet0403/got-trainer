#include "features/feature_registry.hpp"

#include "features/camera_features.hpp"
#include "features/combat_features.hpp"
#include "features/item_features.hpp"
#include "features/movement_features.hpp"
#include "features/player_features.hpp"

namespace got {

FeatureRegistry& FeatureRegistry::Instance() {
    static FeatureRegistry inst;
    return inst;
}

void FeatureRegistry::Initialize() {
    if (initialized_) return;
    InitPlayerFeatures();
    InitCombatFeatures();
    InitMovementFeatures();
    InitCameraFeatures();
    InitItemFeatures();
    initialized_ = true;
}

void FeatureRegistry::Update() {
    if (!initialized_) return;
    UpdatePlayerFeatures();
    UpdateCombatFeatures();
    UpdateMovementFeatures();
    UpdateCameraFeatures();
    UpdateItemFeatures();
}

void FeatureRegistry::Shutdown() {
    if (!initialized_) return;
    ShutdownItemFeatures();
    ShutdownCameraFeatures();
    ShutdownMovementFeatures();
    ShutdownCombatFeatures();
    ShutdownPlayerFeatures();
    initialized_ = false;
}

}  // namespace got
