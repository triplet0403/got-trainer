#pragma once

#include <Windows.h>

namespace got {

void InitPresentHook();
void ShutdownPresentHook();
bool IsMenuOpen();

}  // namespace got
