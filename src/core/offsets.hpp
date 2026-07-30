#pragma once

#include <cstdint>

namespace got {

struct Offsets {
    static constexpr uint32_t PlayerHealth = 0x1974;
    static constexpr uint32_t PlayerMaxHealth = 0x1970;
    static constexpr uint32_t PlayerResolve = 0x43A4;
    static constexpr uint32_t PlayerVelocityX = 0x1E0;
    static constexpr uint32_t PlayerPosition = 0x120;
    static constexpr uint32_t PlayerPositionW = 0x12C;
    static constexpr uint32_t CamPosition = 0x550;
    static constexpr uint32_t CamPositionZ = 0x558;
    static constexpr uint32_t CamPositionW = 0x55C;
    static constexpr uint32_t CamRotBasis = 0x100;
    static constexpr uint32_t Detection = 0xA8;
    static constexpr uint32_t PlayerEvadeFlags = 0x4260;
    static constexpr uint32_t GearTechniquePointsIndex = 0xCB4;
    static constexpr uint32_t GearFlagsTable = 0x6B4;
};

constexpr const char* kGameVersion = "v1053.5.0625.1621";
constexpr const wchar_t* kGameModule = L"GhostOfTsushima.exe";
constexpr uint64_t kAddResourceHash = 0xFBEF564206CD1F52ULL;

}  // namespace got
