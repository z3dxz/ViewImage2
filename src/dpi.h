#include <windows.h>
#include <iostream>

typedef HRESULT(WINAPI* GetDpiForMonitorProc)(HMONITOR, int, UINT*, UINT*);
typedef UINT(WINAPI* GetDpiForWindowProc)(HWND);

struct DpiResult {
    UINT dpiX;
    UINT dpiY;
};

inline DpiResult GetSystemOrWindowDpi(HWND hwnd) {
    DpiResult result = { 96, 96 };

    // 1. Windows 10
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32 && hwnd) {
        GetDpiForWindowProc pGetDpiForWindow = (GetDpiForWindowProc)GetProcAddress(hUser32, "GetDpiForWindow");
        if (pGetDpiForWindow) {
            UINT dpi = pGetDpiForWindow(hwnd);
            if (dpi > 0) {
                result.dpiX = dpi;
                result.dpiY = dpi;
                return result;
            }
        }
    }

    // 2. Windows 8.1
    HMODULE hShcore = LoadLibraryW(L"shcore.dll");
    if (hShcore) {
        GetDpiForMonitorProc pGetDpiForMonitor = (GetDpiForMonitorProc)GetProcAddress(hShcore, "GetDpiForMonitor");
        if (pGetDpiForMonitor && hwnd) {
            HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
            UINT dpiX = 0, dpiY = 0;
            if (SUCCEEDED(pGetDpiForMonitor(hMonitor, 0, &dpiX, &dpiY))) {
                result.dpiX = dpiX;
                result.dpiY = dpiY;
                FreeLibrary(hShcore);
                return result;
            }
        }
        FreeLibrary(hShcore);
    }

    HDC hdc = GetDC(hwnd);
    if (hdc) {
        result.dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
        result.dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
        ReleaseDC(hwnd, hdc);
    }

    return result;
}

inline float getuiscale() {
    HWND hwnd = GetDesktopWindow(); 
    
    DpiResult dpi = GetSystemOrWindowDpi(hwnd);
    
    double scaleFactorX = (double)dpi.dpiX / 96.0;
    double scaleFactorY = (double)dpi.dpiY / 96.0;

    std::cout << "My DPI: " << dpi.dpiX << " and " << dpi.dpiY << "\n";

    return scaleFactorY;
}
