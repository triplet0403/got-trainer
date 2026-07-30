#include "core/log.hpp"

#include <cstdarg>
#include <cstdio>
#include <mutex>

namespace got {

namespace {
FILE* g_logFile = nullptr;
std::mutex g_logMutex;
}  // namespace

void InitLog() {
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (!g_logFile) {
        g_logFile = fopen("got_trainer.log", "w");
        if (g_logFile) {
            fprintf(g_logFile, "=== Ghost of Tsushima Trainer Log Initialized ===\n");
            fflush(g_logFile);
        }
    }
}

void Log(const char* fmt, ...) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    char buffer[1024];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    OutputDebugStringA(buffer);
    OutputDebugStringA("\n");

    if (g_logFile) {
        fprintf(g_logFile, "%s\n", buffer);
        fflush(g_logFile);
    }
}

void CloseLog() {
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (g_logFile) {
        fprintf(g_logFile, "=== Trainer Log Closed ===\n");
        fclose(g_logFile);
        g_logFile = nullptr;
    }
}

}  // namespace got
