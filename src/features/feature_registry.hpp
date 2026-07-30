#pragma once

namespace got {

class FeatureRegistry {
public:
    static FeatureRegistry& Instance();
    void Initialize();
    void Update();
    void Shutdown();

private:
    FeatureRegistry() = default;
    bool initialized_ = false;
};

}  // namespace got
