#include <windows.h>
#include <stdint.h>
#include <stdio.h>

void* decodeico(const char* filepath, int* imgwidth, int* imgheight) {
    HICON hIcon = (HICON)LoadImage(NULL, filepath, IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
    if (!hIcon) return NULL;

    ICONINFO ii = {0};
    if (!GetIconInfo(hIcon, &ii)) {
        DestroyIcon(hIcon);
        return NULL;
    }
    BITMAP bm;
    GetObject(ii.hbmColor, sizeof(BITMAP), &bm);
    
    DeleteObject(ii.hbmColor);
    DeleteObject(ii.hbmMask);

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = bm.bmWidth;
    bmi.bmiHeader.biHeight = -bm.bmHeight; // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;         // Guaranteed 32-bit
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = NULL;
    HDC hScreenDC = GetDC(NULL);
    HBITMAP hDib = CreateDIBSection(hScreenDC, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    HDC hMemDC = CreateCompatibleDC(hScreenDC);
    ReleaseDC(NULL, hScreenDC);

    if (!hDib || !hMemDC) {
        if (hDib) DeleteObject(hDib);
        if (hMemDC) DeleteDC(hMemDC);
        DestroyIcon(hIcon);
        return NULL;
    }

    HBITMAP hOldBmp = (HBITMAP)SelectObject(hMemDC, hDib);
    
    DrawIconEx(hMemDC, 0, 0, hIcon, bm.bmWidth, bm.bmHeight, 0, NULL, DI_NORMAL);

    int size = bm.bmWidth * bm.bmHeight * 4;
    void* resultBuffer = malloc(size);
    if (resultBuffer) {
        memcpy(resultBuffer, pBits, size);
    }

    SelectObject(hMemDC, hOldBmp);
    DeleteObject(hDib);
    DeleteDC(hMemDC);
    DestroyIcon(hIcon);

    *imgwidth = bm.bmWidth;
    *imgheight = bm.bmHeight;

    return resultBuffer;
}