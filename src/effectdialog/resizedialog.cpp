#include "headers/resizedialog.hpp"
#include "../../res/resource.h"
#include <Uxtheme.h>
//#include <dwmapi.h>

GlobalParams* m;

// resize dialog
HWND hWidthEdit, hHeightEdit;
HWND Confirm, No, LW, LH;

// Function prototypes
LRESULT CALLBACK ResizeDialogProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

int ShowResizeDialog(GlobalParams* m0){
    m = m0;
    // Create the main dialog
    DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(Resize), m->hwnd, (DLGPROC)ResizeDialogProc);

    return 0;
}

// this should really be in ops.cpp but we aren't going to talk about it
bool IsNumeric(LPCTSTR str) { 
    for (int i = 0; str[i] != '\0'; ++i) {
        if (!iswdigit(str[i])) {
            return false;
        }
    }
    return true;
}

HBRUSH b;
HBRUSH be;
HBRUSH bz;
HBRUSH bu;

HANDLE lockic = LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(lockicon), IMAGE_ICON, 16, 16, LR_DEFAULTSIZE);
HANDLE unlockic = LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(unlockicon1), IMAGE_ICON, 16, 16, LR_DEFAULTSIZE);

bool iswlock = false;
bool ishlock = false;

int ogwidth;
int ogheight;

int firstw;
int firsth;



LRESULT CALLBACK TXTProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    switch (msg)
    {
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wparam;
            SetTextColor(hdc, RGB(220, 220, 220));
            SetBkColor(hdc, RGB(40, 40, 40));
            static HBRUSH hEditBg = CreateSolidBrush(RGB(40, 40, 40));
            return (LRESULT)hEditBg;
        }
        case WM_PAINT: {

            HDC hdc = GetWindowDC(hwnd);
            RECT rect;
            GetWindowRect(hwnd, &rect);
            OffsetRect(&rect, -rect.left, -rect.top);
            
            LRESULT result = DefSubclassProc(hwnd, msg, wparam, lparam);

            HPEN brush = CreatePen(PS_SOLID, 1, RGB(128,128,128));
            HGDIOBJ oldPen = SelectObject(hdc, brush);
            SelectObject(hdc, GetStockObject(NULL_BRUSH));
            RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, 4,4);
            
            // Cleanup
            SelectObject(hdc, oldPen);
            DeleteObject(brush);
            ReleaseDC(hwnd, hdc);

            return result;
        }
        case WM_NCPAINT:
            return 0; // Prevent Windows from drawing the old-style 3D border
        case WM_CHAR: {
            if (wparam < '0' || wparam > '9')  {
                if (wparam != '\b' && wparam != '\r' && wparam != '\t' && wparam != '\x1A')
                    return 0;
            }
            return DefSubclassProc(hwnd, msg, wparam, lparam);
        }
        default:
            return DefSubclassProc(hwnd, msg, wparam, lparam);
        }

        return 0;
}

LRESULT CALLBACK ButtonPaint(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    static HFONT hVerdana = CreateFont(15, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, 
                                       ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
                                       CLEARTYPE_QUALITY, VARIABLE_PITCH | FF_SWISS, "SegoeUI");

    switch (msg) {
        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rect;
            GetClientRect(hwnd, &rect);

            // 1. Setup Quality Rendering
            HFONT oldFont = (HFONT)SelectObject(hdc, hVerdana);
            bool isPressed = (SendMessage(hwnd, BM_GETSTATE, 0, 0) & BST_PUSHED);

            HBRUSH backgroundb = CreateSolidBrush(isPressed ? RGB(60, 60, 60) : RGB(40, 40, 40));
            HGDIOBJ oldBrush = SelectObject(hdc, backgroundb);
            
            HPEN hNullPen = CreatePen(PS_NULL, 0, 0);
            HGDIOBJ oldNullPen = SelectObject(hdc, hNullPen);
            RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, 8, 8);
            
            SelectObject(hdc, GetStockObject(NULL_BRUSH));
            
            HPEN black = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
            SelectObject(hdc, black);
            RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, 8, 8);
            
            HPEN gray = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
            SelectObject(hdc, gray);
            RoundRect(hdc, rect.left + 1, rect.top + 1, rect.right - 1, rect.bottom - 1, 6, 6);

            char text[64];
            GetWindowText(hwnd, text, sizeof(text));
            SetTextColor(hdc, RGB(220, 220, 220));
            SetBkMode(hdc, TRANSPARENT);
            DrawText(hdc, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            
            HICON hIcon = (HICON)SendMessage(hwnd, BM_GETIMAGE, IMAGE_ICON, 0);
            if (hIcon) {
                int iconSize = 16;
                int x = rect.left + (rect.right - rect.left - iconSize) / 2;
                int y = rect.top + (rect.bottom - rect.top - iconSize) / 2;

                DrawIconEx(hdc, x, y, hIcon, iconSize, iconSize, 0, NULL, DI_NORMAL);
            }

            SelectObject(hdc, oldFont);
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldNullPen);
            DeleteObject(backgroundb);
            DeleteObject(black);
            DeleteObject(gray);
            DeleteObject(hNullPen);

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_NCDESTROY: {
            return DefSubclassProc(hwnd, msg, wparam, lparam);
        }

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MOUSEMOVE:
        case WM_MOUSELEAVE: {
            InvalidateRect(hwnd, NULL, FALSE);
            return DefSubclassProc(hwnd, msg, wparam, lparam);
        }

        default:
            return DefSubclassProc(hwnd, msg, wparam, lparam);
    }
}
LRESULT CALLBACK ResizeDialogProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {

    switch (msg) {

    case WM_INITDIALOG: {
        iswlock = false;
        ishlock = false;
        if (m->imgwidth < 1) {

            MessageBox(hwnd, "You need an image first", "Oops", MB_OK | MB_ICONERROR);

            INT Result = 1;
            EndDialog(hwnd, Result);
        }
        if (!m) {
            MessageBox(hwnd, "Once upon a time there was a little pointer called m. it travels all across our code, until one day, it got lost. we can't find m, so we don't know what to do here, therefore throwing an error together!", "Fatal Memory Transfer Error", MB_OK | MB_ICONERROR);
            return FALSE;
        }

        // Initialize dialog controls and set default values
        hWidthEdit = GetDlgItem(hwnd, WidthPBox);
        hHeightEdit = GetDlgItem(hwnd, HeightPBox);
        Confirm = GetDlgItem(hwnd, ConfirmBBox);
        No = GetDlgItem(hwnd, CancelBBox);
        LW = GetDlgItem(hwnd, lockw);
        LH = GetDlgItem(hwnd, lockh);


        SetWindowSubclass(hWidthEdit, TXTProc, 0, 0);
        SetWindowSubclass(hHeightEdit, TXTProc, 0, 0);

        SetWindowSubclass(Confirm, ButtonPaint, 0, 0);
        SetWindowSubclass(No, ButtonPaint, 0, 0);
        SetWindowSubclass(LW, ButtonPaint, 0, 0);
        SetWindowSubclass(LH, ButtonPaint, 0, 0);

        SendMessage(LW, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)unlockic);
        SendMessage(LH, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)unlockic);

        int w1 = m->imgwidth;
        int h1 = m->imgheight;

        firstw = w1;
        firsth = h1;

        char strw[8];
        char strh[8];

        sprintf(strw, "%d", w1);
        sprintf(strh, "%d", h1);

        SetWindowText(hWidthEdit, strw);
        SetWindowText(hHeightEdit, strh);

        SetFocus(hWidthEdit);

        SendMessage(hWidthEdit, EM_SETSEL, 0, -1);

        DwmDarken(hwnd);

        return FALSE;
    }
	case WM_PAINT: {

        HDC hdc = GetWindowDC(hwnd);

        RECT rect = {10, 35, 214, 169};

        HPEN brush = CreatePen(PS_SOLID, 1, RGB(128,128,128));
        HGDIOBJ oldPen = SelectObject(hdc, brush);
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, 4,4);
        
        // Cleanup
        SelectObject(hdc, oldPen);
        DeleteObject(brush);
        ReleaseDC(hwnd, hdc);

		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
    case WM_CTLCOLORDLG: {

        if (bz) DeleteObject(bz);
        bz = CreateSolidBrush(RGB(64,64,64));
        return (LRESULT)bz;
    }
    case WM_CTLCOLORBTN: {
        if (bu) DeleteObject(bu);
        bu = CreateSolidBrush(RGB(64,64,64));
        return (LRESULT)bu;
    }
    case WM_CTLCOLORSTATIC: {

        DWORD CtrlID = GetDlgCtrlID((HWND)lparam);
        HDC hdcStatic = (HDC)wparam;

        if (b) DeleteObject(b);
        b = CreateSolidBrush(RGB(64,64,64));

        SetBkMode(hdcStatic, TRANSPARENT);

        // Set default text color
        SetTextColor(hdcStatic, RGB(200,200,200));


        return (LRESULT)b;
    }

    case WM_CTLCOLOREDIT: {
        DWORD CtrlID = GetDlgCtrlID((HWND)lparam);
        HDC hdcStatic = (HDC)wparam;

        if (be) DeleteObject(be);
        b = CreateSolidBrush(RGB(40,40,40));

        SetBkMode(hdcStatic, TRANSPARENT);
        SetTextColor(hdcStatic, RGB(128,128,128));
        SetBkColor(hdcStatic, RGB(0, 255, 255));


       return (LRESULT)b;
    }
    case WM_COMMAND: {
        if ((LOWORD(wparam) == WidthPBox && HIWORD(wparam) == EN_CHANGE)&&ishlock) {
            const int nMaxCount = 64;

            char widthTxt[nMaxCount];
            GetWindowText(hWidthEdit, widthTxt, nMaxCount);

            int width = std::atoi(widthTxt);

            float asp = (float)ogwidth / (float)ogheight;

            int newheight = (float)width / asp;

            std::string newtxt = std::to_string(newheight);

            SetWindowText(hHeightEdit, newtxt.c_str());
        }

        if ((LOWORD(wparam) == HeightPBox && HIWORD(wparam) == EN_CHANGE) && iswlock) {
            const int nMaxCount = 64;

            char heightTxt[nMaxCount];
            GetWindowText(hHeightEdit, heightTxt, nMaxCount);

            int height = std::atoi(heightTxt);

            float asp = (float)ogwidth / (float)ogheight;

            int newwidth = (float)height * asp;

            std::string newtxt = std::to_string(newwidth);

            SetWindowText(hWidthEdit, newtxt.c_str());
        }

        switch (LOWORD(wparam)) {
        case lockw: {
            if (iswlock) {
                // undo
                iswlock = false;
                SendMessage(LW, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)unlockic);
                SendMessage(LH, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)unlockic);
                EnableWindow(hWidthEdit, TRUE);
                EnableWindow(hHeightEdit, TRUE);
            }
            else {
                iswlock = true;
                ishlock = false;
                SendMessage(LW, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)lockic);
                SendMessage(LH, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)unlockic);
                EnableWindow(hWidthEdit, FALSE);
                EnableWindow(hHeightEdit, TRUE);

                char heightTxt[64];
                GetWindowText(hHeightEdit, heightTxt, 64);
                ogheight = std::atoi(heightTxt);
                char widthTxt[64];
                GetWindowText(hWidthEdit, widthTxt, 64);
                ogwidth = std::atoi(widthTxt);
            }
            break;
        }
        case lockh: {
            if (ishlock) {
                // undo
                ishlock = false;
                SendMessage(LW, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)unlockic);
                SendMessage(LH, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)unlockic);
                EnableWindow(hWidthEdit, TRUE);
                EnableWindow(hHeightEdit, TRUE);
            }
            else {
                iswlock = false;
                ishlock = true;
                SendMessage(LW, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)unlockic);
                SendMessage(LH, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)lockic);
                EnableWindow(hWidthEdit, TRUE);
                EnableWindow(hHeightEdit, FALSE);

                char heightTxt[64];
                GetWindowText(hHeightEdit, heightTxt, 64);
                ogheight = std::atoi(heightTxt);
                char widthTxt[64];
                GetWindowText(hWidthEdit, widthTxt, 64);
                ogwidth = std::atoi(widthTxt);
            }
            break;
        }
        case ConfirmBBox: {
            // Retrieve values from text boxes
            
            char widthText[16], heightText[16];
            GetWindowText(hWidthEdit, widthText, 16);
            GetWindowText(hHeightEdit, heightText, 16);

            if (!IsNumeric(widthText) || !IsNumeric(heightText)) {
                MessageBox(hwnd, "Width and height must be numeric!", "Error", MB_OK | MB_ICONERROR);
                SetFocus(hWidthEdit);
                SendMessage(hWidthEdit, EM_SETSEL, 0, -1);
                return TRUE; // Do not proceed with resizing
            }

            // Convert text to integers
            int width = atoi(widthText);
            int height = atoi(heightText);

            if (width > 16000 || height > 16000) {
                MessageBox(hwnd, "Too many pixels: Too high resolution", "Error", MB_OK | MB_ICONERROR);
                return TRUE; // Do not proceed with resizing
            }
            if (width < 1 || height < 1) {
                MessageBox(hwnd, "You need at least 1 pixel in each dimension", "Error", MB_OK | MB_ICONERROR);
                return TRUE; // Do not proceed with resizing
            }

            if(width == firstw && height == firsth) {
                // do nothing for now
                
            } else {
                // Perform resizing logic with width and height
                ResizeImageToSize(m, width, height);
            }

            // Close the dialog
            EndDialog(hwnd, IDOK);
            break;
        }
        case CancelBBox: {
            // Close the dialog without performing any action
            EndDialog(hwnd, IDCANCEL);
            break;
        }
        }
        break;
    }
    case WM_CLOSE: {
        EndDialog(hwnd, IDCANCEL);
    }
    default: {
        return FALSE;
    }
    }

    return TRUE;
}