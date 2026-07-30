#pragma once

#include <cstdint>

namespace got {

void InitItemFeatures();
void ShutdownItemFeatures();
void UpdateItemFeatures();
bool AddResourceToInventory(uint32_t resourceId, uint32_t amount);

// Grants the gear identified by gearHash using the already-scanned
// pGear / pTemplates / pPermGear / AddGear pointers. Returns false (and
// logs the reason) if any required pointer is missing.
//
// NOTE: the exact AddGear calling convention below (pGear, gearHash,
// pPermGear) matches the layout implied by the existing pointer scans in
// game_pointers.cpp. If a future game patch changes the function
// signature, update AddGearFn in item_features.cpp to match.
bool GiveGear(uint64_t gearHash);

}  // namespace got
