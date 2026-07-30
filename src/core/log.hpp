#pragma once

#include <Windows.h>
#include <string>

namespace got {

void Log(const char* fmt, ...);
void InitLog();
void CloseLog();

}  // namespace got
