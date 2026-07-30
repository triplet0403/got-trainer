#pragma once

#include <cstdint>

namespace got {

void InitItemFeatures();
void ShutdownItemFeatures();
void UpdateItemFeatures();
bool AddResourceToInventory(uint32_t resourceId, uint32_t amount);

bool GiveGear(uint64_t gearHash);

}  // namespace got
