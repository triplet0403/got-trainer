#include "proxy/version_proxy.hpp"
#include <mutex>
#include <winver.h>

namespace {

using PfnGetFileVersionInfoA = BOOL(WINAPI*)(LPCSTR, DWORD, DWORD, LPVOID);
using PfnGetFileVersionInfoByHandle = BOOL(WINAPI*)(DWORD, LPCWSTR);
using PfnGetFileVersionInfoExA = BOOL(WINAPI*)(DWORD, LPCSTR, DWORD, DWORD, LPVOID);
using PfnGetFileVersionInfoExW = BOOL(WINAPI*)(DWORD, LPCWSTR, DWORD, DWORD, LPVOID);
using PfnGetFileVersionInfoSizeA = DWORD(WINAPI*)(LPCSTR, LPDWORD);
using PfnGetFileVersionInfoSizeExA = DWORD(WINAPI*)(DWORD, LPCSTR, LPDWORD);
using PfnGetFileVersionInfoSizeExW = DWORD(WINAPI*)(DWORD, LPCWSTR, LPDWORD);
using PfnGetFileVersionInfoSizeW = DWORD(WINAPI*)(LPCWSTR, LPDWORD);
using PfnGetFileVersionInfoW = BOOL(WINAPI*)(LPCWSTR, DWORD, DWORD, LPVOID);
using PfnVerFindFileA = DWORD(WINAPI*)(DWORD, LPSTR, LPSTR, LPSTR, LPSTR, PUINT, LPSTR, PUINT);
using PfnVerFindFileW = DWORD(WINAPI*)(DWORD, LPWSTR, LPWSTR, LPWSTR, LPWSTR, PUINT, LPWSTR, PUINT);
using PfnVerInstallFileA = DWORD(WINAPI*)(DWORD, LPSTR, LPSTR, LPSTR, LPSTR, LPSTR, LPSTR, PUINT);
using PfnVerInstallFileW = DWORD(WINAPI*)(DWORD, LPWSTR, LPWSTR, LPWSTR, LPWSTR, LPWSTR, LPWSTR, PUINT);
using PfnVerLanguageNameA = DWORD(WINAPI*)(DWORD, LPSTR, DWORD);
using PfnVerLanguageNameW = DWORD(WINAPI*)(DWORD, LPWSTR, DWORD);
using PfnVerQueryValueA = BOOL(WINAPI*)(LPCVOID, LPCSTR, LPVOID*, PUINT);
using PfnVerQueryValueW = BOOL(WINAPI*)(LPCVOID, LPCWSTR, LPVOID*, PUINT);

HMODULE g_hRealVersion = nullptr;
std::once_flag g_proxyOnce;

PfnGetFileVersionInfoA g_pfnGetFileVersionInfoA = nullptr;
PfnGetFileVersionInfoByHandle g_pfnGetFileVersionInfoByHandle = nullptr;
PfnGetFileVersionInfoExA g_pfnGetFileVersionInfoExA = nullptr;
PfnGetFileVersionInfoExW g_pfnGetFileVersionInfoExW = nullptr;
PfnGetFileVersionInfoSizeA g_pfnGetFileVersionInfoSizeA = nullptr;
PfnGetFileVersionInfoSizeExA g_pfnGetFileVersionInfoSizeExA = nullptr;
PfnGetFileVersionInfoSizeExW g_pfnGetFileVersionInfoSizeExW = nullptr;
PfnGetFileVersionInfoSizeW g_pfnGetFileVersionInfoSizeW = nullptr;
PfnGetFileVersionInfoW g_pfnGetFileVersionInfoW = nullptr;
PfnVerFindFileA g_pfnVerFindFileA = nullptr;
PfnVerFindFileW g_pfnVerFindFileW = nullptr;
PfnVerInstallFileA g_pfnVerInstallFileA = nullptr;
PfnVerInstallFileW g_pfnVerInstallFileW = nullptr;
PfnVerLanguageNameA g_pfnVerLanguageNameA = nullptr;
PfnVerLanguageNameW g_pfnVerLanguageNameW = nullptr;
PfnVerQueryValueA g_pfnVerQueryValueA = nullptr;
PfnVerQueryValueW g_pfnVerQueryValueW = nullptr;

void EnsureProxyLoaded() {
    std::call_once(g_proxyOnce, []() {
        wchar_t sysPath[MAX_PATH];
        GetSystemDirectoryW(sysPath, MAX_PATH);
        wcscat_s(sysPath, MAX_PATH, L"\\version.dll");

        g_hRealVersion = LoadLibraryW(sysPath);
        if (g_hRealVersion) {
            g_pfnGetFileVersionInfoA = reinterpret_cast<PfnGetFileVersionInfoA>(GetProcAddress(g_hRealVersion, "GetFileVersionInfoA"));
            g_pfnGetFileVersionInfoByHandle = reinterpret_cast<PfnGetFileVersionInfoByHandle>(GetProcAddress(g_hRealVersion, "GetFileVersionInfoByHandle"));
            g_pfnGetFileVersionInfoExA = reinterpret_cast<PfnGetFileVersionInfoExA>(GetProcAddress(g_hRealVersion, "GetFileVersionInfoExA"));
            g_pfnGetFileVersionInfoExW = reinterpret_cast<PfnGetFileVersionInfoExW>(GetProcAddress(g_hRealVersion, "GetFileVersionInfoExW"));
            g_pfnGetFileVersionInfoSizeA = reinterpret_cast<PfnGetFileVersionInfoSizeA>(GetProcAddress(g_hRealVersion, "GetFileVersionInfoSizeA"));
            g_pfnGetFileVersionInfoSizeExA = reinterpret_cast<PfnGetFileVersionInfoSizeExA>(GetProcAddress(g_hRealVersion, "GetFileVersionInfoSizeExA"));
            g_pfnGetFileVersionInfoSizeExW = reinterpret_cast<PfnGetFileVersionInfoSizeExW>(GetProcAddress(g_hRealVersion, "GetFileVersionInfoSizeExW"));
            g_pfnGetFileVersionInfoSizeW = reinterpret_cast<PfnGetFileVersionInfoSizeW>(GetProcAddress(g_hRealVersion, "GetFileVersionInfoSizeW"));
            g_pfnGetFileVersionInfoW = reinterpret_cast<PfnGetFileVersionInfoW>(GetProcAddress(g_hRealVersion, "GetFileVersionInfoW"));
            g_pfnVerFindFileA = reinterpret_cast<PfnVerFindFileA>(GetProcAddress(g_hRealVersion, "VerFindFileA"));
            g_pfnVerFindFileW = reinterpret_cast<PfnVerFindFileW>(GetProcAddress(g_hRealVersion, "VerFindFileW"));
            g_pfnVerInstallFileA = reinterpret_cast<PfnVerInstallFileA>(GetProcAddress(g_hRealVersion, "VerInstallFileA"));
            g_pfnVerInstallFileW = reinterpret_cast<PfnVerInstallFileW>(GetProcAddress(g_hRealVersion, "VerInstallFileW"));
            g_pfnVerLanguageNameA = reinterpret_cast<PfnVerLanguageNameA>(GetProcAddress(g_hRealVersion, "VerLanguageNameA"));
            g_pfnVerLanguageNameW = reinterpret_cast<PfnVerLanguageNameW>(GetProcAddress(g_hRealVersion, "VerLanguageNameW"));
            g_pfnVerQueryValueA = reinterpret_cast<PfnVerQueryValueA>(GetProcAddress(g_hRealVersion, "VerQueryValueA"));
            g_pfnVerQueryValueW = reinterpret_cast<PfnVerQueryValueW>(GetProcAddress(g_hRealVersion, "VerQueryValueW"));
        }
    });
}

}  // namespace

namespace got {

bool InitProxy() {
    EnsureProxyLoaded();
    return g_hRealVersion != nullptr;
}

void ShutdownProxy() {
    if (g_hRealVersion) {
        FreeLibrary(g_hRealVersion);
        g_hRealVersion = nullptr;
    }
}

}  // namespace got

extern "C" __declspec(dllexport) BOOL WINAPI GetFileVersionInfoA(LPCSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData) {
    EnsureProxyLoaded();
    if (!g_pfnGetFileVersionInfoA) return FALSE;
    return g_pfnGetFileVersionInfoA(lptstrFilename, dwHandle, dwLen, lpData);
}

extern "C" __declspec(dllexport) BOOL WINAPI GetFileVersionInfoByHandle(DWORD dwFlags, LPCWSTR lpFileName) {
    EnsureProxyLoaded();
    if (!g_pfnGetFileVersionInfoByHandle) return FALSE;
    return g_pfnGetFileVersionInfoByHandle(dwFlags, lpFileName);
}

extern "C" __declspec(dllexport) BOOL WINAPI GetFileVersionInfoExA(DWORD dwFlags, LPCSTR lpwstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData) {
    EnsureProxyLoaded();
    if (!g_pfnGetFileVersionInfoExA) return FALSE;
    return g_pfnGetFileVersionInfoExA(dwFlags, lpwstrFilename, dwHandle, dwLen, lpData);
}

extern "C" __declspec(dllexport) BOOL WINAPI GetFileVersionInfoExW(DWORD dwFlags, LPCWSTR lpwstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData) {
    EnsureProxyLoaded();
    if (!g_pfnGetFileVersionInfoExW) return FALSE;
    return g_pfnGetFileVersionInfoExW(dwFlags, lpwstrFilename, dwHandle, dwLen, lpData);
}

extern "C" __declspec(dllexport) DWORD WINAPI GetFileVersionInfoSizeA(LPCSTR lptstrFilename, LPDWORD lpdwHandle) {
    EnsureProxyLoaded();
    if (!g_pfnGetFileVersionInfoSizeA) return 0;
    return g_pfnGetFileVersionInfoSizeA(lptstrFilename, lpdwHandle);
}

extern "C" __declspec(dllexport) DWORD WINAPI GetFileVersionInfoSizeExA(DWORD dwFlags, LPCSTR lpwstrFilename, LPDWORD lpdwHandle) {
    EnsureProxyLoaded();
    if (!g_pfnGetFileVersionInfoSizeExA) return 0;
    return g_pfnGetFileVersionInfoSizeExA(dwFlags, lpwstrFilename, lpdwHandle);
}

extern "C" __declspec(dllexport) DWORD WINAPI GetFileVersionInfoSizeExW(DWORD dwFlags, LPCWSTR lpwstrFilename, LPDWORD lpdwHandle) {
    EnsureProxyLoaded();
    if (!g_pfnGetFileVersionInfoSizeExW) return 0;
    return g_pfnGetFileVersionInfoSizeExW(dwFlags, lpwstrFilename, lpdwHandle);
}

extern "C" __declspec(dllexport) DWORD WINAPI GetFileVersionInfoSizeW(LPCWSTR lptstrFilename, LPDWORD lpdwHandle) {
    EnsureProxyLoaded();
    if (!g_pfnGetFileVersionInfoSizeW) return 0;
    return g_pfnGetFileVersionInfoSizeW(lptstrFilename, lpdwHandle);
}

extern "C" __declspec(dllexport) BOOL WINAPI GetFileVersionInfoW(LPCWSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData) {
    EnsureProxyLoaded();
    if (!g_pfnGetFileVersionInfoW) return FALSE;
    return g_pfnGetFileVersionInfoW(lptstrFilename, dwHandle, dwLen, lpData);
}

extern "C" __declspec(dllexport) DWORD WINAPI VerFindFileA(DWORD uFlags, LPSTR szFileName, LPSTR szWinDir, LPSTR szAppDir, LPSTR szCurDir, PUINT puCurDirLen, LPSTR szDestDir, PUINT puDestDirLen) {
    EnsureProxyLoaded();
    if (!g_pfnVerFindFileA) return 0;
    return g_pfnVerFindFileA(uFlags, szFileName, szWinDir, szAppDir, szCurDir, puCurDirLen, szDestDir, puDestDirLen);
}

extern "C" __declspec(dllexport) DWORD WINAPI VerFindFileW(DWORD uFlags, LPWSTR szFileName, LPWSTR szWinDir, LPWSTR szAppDir, LPWSTR szCurDir, PUINT puCurDirLen, LPWSTR szDestDir, PUINT puDestDirLen) {
    EnsureProxyLoaded();
    if (!g_pfnVerFindFileW) return 0;
    return g_pfnVerFindFileW(uFlags, szFileName, szWinDir, szAppDir, szCurDir, puCurDirLen, szDestDir, puDestDirLen);
}

extern "C" __declspec(dllexport) DWORD WINAPI VerInstallFileA(DWORD uFlags, LPSTR szSrcFileName, LPSTR szDestFileName, LPSTR szSrcDir, LPSTR szDestDir, LPSTR szCurDir, LPSTR szTmpFile, PUINT puTmpFileLen) {
    EnsureProxyLoaded();
    if (!g_pfnVerInstallFileA) return 0;
    return g_pfnVerInstallFileA(uFlags, szSrcFileName, szDestFileName, szSrcDir, szDestDir, szCurDir, szTmpFile, puTmpFileLen);
}

extern "C" __declspec(dllexport) DWORD WINAPI VerInstallFileW(DWORD uFlags, LPWSTR szSrcFileName, LPWSTR szDestFileName, LPWSTR szSrcDir, LPWSTR szDestDir, LPWSTR szCurDir, LPWSTR szTmpFile, PUINT puTmpFileLen) {
    EnsureProxyLoaded();
    if (!g_pfnVerInstallFileW) return 0;
    return g_pfnVerInstallFileW(uFlags, szSrcFileName, szDestFileName, szSrcDir, szDestDir, szCurDir, szTmpFile, puTmpFileLen);
}

extern "C" __declspec(dllexport) DWORD WINAPI VerLanguageNameA(DWORD wLang, LPSTR szLang, DWORD cchLang) {
    EnsureProxyLoaded();
    if (!g_pfnVerLanguageNameA) return 0;
    return g_pfnVerLanguageNameA(wLang, szLang, cchLang);
}

extern "C" __declspec(dllexport) DWORD WINAPI VerLanguageNameW(DWORD wLang, LPWSTR szLang, DWORD cchLang) {
    EnsureProxyLoaded();
    if (!g_pfnVerLanguageNameW) return 0;
    return g_pfnVerLanguageNameW(wLang, szLang, cchLang);
}

extern "C" __declspec(dllexport) BOOL WINAPI VerQueryValueA(LPCVOID pBlock, LPCSTR lpSubBlock, LPVOID* lplpBuffer, PUINT puLen) {
    EnsureProxyLoaded();
    if (!g_pfnVerQueryValueA) return FALSE;
    return g_pfnVerQueryValueA(pBlock, lpSubBlock, lplpBuffer, puLen);
}

extern "C" __declspec(dllexport) BOOL WINAPI VerQueryValueW(LPCVOID pBlock, LPCWSTR lpSubBlock, LPVOID* lplpBuffer, PUINT puLen) {
    EnsureProxyLoaded();
    if (!g_pfnVerQueryValueW) return FALSE;
    return g_pfnVerQueryValueW(pBlock, lpSubBlock, lplpBuffer, puLen);
}
