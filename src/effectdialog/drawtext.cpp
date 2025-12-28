#include "headers/drawtext.h"
#include "../../res/resource.h"
#include <Uxtheme.h>
#include <iostream>
#include <string>
#include "../headers/rendering.hpp"
//#include <dwmapi.h>
bool dontdo = false;
#include <windowsx.h>

static GlobalParams* m;


void UpdateImage(GlobalParams* m);

static LRESULT CALLBACK TrackbarJumpSubclass(
	HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
	UINT_PTR id, DWORD_PTR ref)
{
	static BOOL dragging;
	RECT rc;
	int min, max, pos;
	BOOL vert;

	switch (msg)
	{
	case WM_LBUTTONDOWN:
		dragging = TRUE;
		SetCapture(hwnd);

	case WM_MOUSEMOVE:
		if (!dragging || !(wParam & MK_LBUTTON))
			break;

		SendMessage(hwnd, TBM_GETCHANNELRECT, 0, (LPARAM)&rc);
		min = (int)SendMessage(hwnd, TBM_GETRANGEMIN, 0, 0);
		max = (int)SendMessage(hwnd, TBM_GETRANGEMAX, 0, 0);
		vert = (GetWindowLong(hwnd, GWL_STYLE) & TBS_VERT) != 0;

		if (vert)
		{
			int y = GET_Y_LPARAM(lParam);
			if (y < rc.top) y = rc.top;
			if (y > rc.bottom) y = rc.bottom;
			pos = max - (max - min) * (y - rc.top) / (rc.bottom - rc.top);

		}
		else
		{
			int x = GET_X_LPARAM(lParam);
			if (x < rc.left) x = rc.left;
			if (x > rc.right) x = rc.right;
			pos = min + (max - min) * (x - rc.left) / (rc.right - rc.left);
		}

		SendMessage(hwnd, TBM_SETPOS, TRUE, pos);
		UpdateImage(m);
		return 0;

	case WM_LBUTTONUP:
		dragging = FALSE;
		ReleaseCapture();
		return 0;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

COLORREF boxColor = RGB(255, 0, 0); // Initial color red
HBRUSH hBrush = NULL;

void PerformDrawTextRealignment() {
	POINT pos;
	GetCursorPos(&pos);
	int mpx = pos.x;
	int mpy = pos.y;
	ScreenToClient(m->hwnd, &pos);

	int k1 = (int)((float)(pos.x - m->CoordLeft) * (1.0f / m->mscaler));
	int v1 = (int)((float)(pos.y - m->CoordTop) * (1.0f / m->mscaler));


    m->locationXtextvar = k1 >0 ? k1 : 0;
    m->locationYtextvar = v1 >0 ? v1 : 0;

    HWND adtdb = GetDlgItem(m->drawtext_access_dialog_hwnd, ActualDrawTextDialogBox);
    SendMessage(m->drawtext_access_dialog_hwnd, WM_COMMAND, 0, 0);
	SetFocus(m->drawtext_access_dialog_hwnd);
	int ts = m->sizetextvar;

    POINT location4 = {mpx+5, mpy-80};
	SetWindowPos(m->drawtext_access_dialog_hwnd, 0, location4.x, location4.y, 0, 0, SWP_NOSIZE);
    ScreenToClient(m->hwnd, &location4);
    m->drawtext_guiLocX = location4.x;
    m->drawtext_guiLocY = location4.y;
	SetForegroundWindow(m->drawtext_access_dialog_hwnd);

}

std::string text = "Cosine64";
std::string fontstring = "segoeui.ttf";

uint32_t textColor = 0xFFFF80FF; // inverted
uint32_t outlineColor = 0xFF000000; // global so we can save it

// Function prototypes
static LRESULT CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

bool didinit = false;
int ShowDrawTextDialog(GlobalParams* m0) {
    didinit = false;
    dontdo = false;
    m = m0;
    if (m->drawmode) {
        m->drawmode = false;
    }

    // Create the main dialog

    DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(DrawTextDialog), 0, (DLGPROC)DialogProc);
    return 0;
}

FT_Face f = nullptr;
static void ApplyEffectToBuffer(void* fromBuffer, void* toBuffer) {

    SwitchFont(f);

    memcpy(toBuffer, fromBuffer, m->imgwidth * m->imgheight * 4);
    PlaceString(m, m->sizetextvar, text.c_str(), m->locationXtextvar, m->locationYtextvar, InvertCC(textColor, true), toBuffer, m->imgwidth, m->imgheight, fromBuffer);
    RedrawSurfaceTextDialog(m);
}

static void ConfirmEffect() {
    createUndoStep(m, false);
   // memcpy(m->imgdata, m->imagepreview, m->imgwidth * m->imgheight * 4);
    
    ApplyEffectToBuffer(m->imgdata, m->imgdata);

    m->shouldSaveShutdown = true;
}


HWND adtdb;
HWND tss;
HWND fnameid;

void UpdateLoadFont() {
    if (f) {
        FT_Done_Face(f);
    }
    f = LoadFont(m, fontstring);

    SetWindowText(fnameid, fontstring.c_str());

}

void InitDialogControls(HWND hwnd) {
    
    
    adtdb = GetDlgItem(hwnd, ActualDrawTextDialogBox);
    tss = GetDlgItem(hwnd, TextSizeSlider);
    fnameid = GetDlgItem(hwnd, FontNameID);

	SetWindowSubclass(tss, TrackbarJumpSubclass, 1, 0);

    SetWindowText(adtdb, text.c_str());

    SendMessage(tss, TBM_SETRANGE, TRUE, MAKELPARAM(1, 512));
    SendMessage(tss, TBM_SETPOS, TRUE, m->sizetextvar);

    SetFocus(adtdb);
    SendMessage(adtdb, EM_SETSEL, 0, -1);

    UpdateLoadFont();
}

void UpdateImage(GlobalParams* m) {
    if (dontdo) {
        return;
    }
    if (!didinit) {
        return;
    }

    char str[256];
    GetWindowText(adtdb, str, 256);

    float size = (float)SendMessage(tss, TBM_GETPOS, 0, 0);

    m->sizetextvar = size;
    text = std::string(str);

    ApplyEffectToBuffer(m->imgdata, m->imagepreview);
}

void freeness() {
     m->drawtext_access_dialog_hwnd = 0;
    m->isImagePreview = false;
    if (m->imagepreview) {
        FreeData(m->imagepreview);
    }
    
}

bool lastghostmode = true;
static LRESULT CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if(m->drawtext_ghostmode != lastghostmode) {
        if(m->drawtext_ghostmode) {
            SetWindowLongPtr(hwnd, GWL_EXSTYLE,
                GetWindowLongPtr(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
            SetLayeredWindowAttributes(hwnd, 0, 50, LWA_ALPHA);
            SetWindowLongPtr(hwnd, GWL_EXSTYLE,
            GetWindowLongPtr(hwnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT);
            
        } else {
            SetWindowLongPtr(hwnd, GWL_EXSTYLE,
                GetWindowLongPtr(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
            SetLayeredWindowAttributes(hwnd, 0, 230, LWA_ALPHA);
            SetWindowLongPtr(hwnd, GWL_EXSTYLE,
            GetWindowLongPtr(hwnd, GWL_EXSTYLE) & ~WS_EX_TRANSPARENT);
        }
    }
    lastghostmode = m->drawtext_ghostmode;

    switch (msg) {
        case WM_INITDIALOG: {

            LONG style = GetWindowLong(hwnd, GWL_STYLE);
            style &= ~(WS_CAPTION | WS_DLGFRAME | WS_BORDER);
            SetWindowLong(hwnd, GWL_STYLE, style);
            SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

            m->drawtext_access_dialog_hwnd = hwnd;
            RECT r;
            GetWindowRect(m->hwnd, &r);

            m->imagepreview = malloc(m->imgwidth * m->imgheight * 4);
            memcpy(m->imagepreview, m->imgdata, m->imgwidth * m->imgheight * 4);
            m->isImagePreview = true;

            InitDialogControls(hwnd);

            RedrawSurfaceTextDialog(m);
            didinit = true;
            UpdateImage(m);
            PerformDrawTextRealignment();


            return FALSE;
        }
        case WM_COMMAND: {
            UpdateImage(m);
            switch (LOWORD(wparam)) {
            case SelectFontButton: {
                std::string font = ShowFontDialog(m, hwnd);

                if (font != "Error") {
                    fontstring = font;
                    UpdateLoadFont();
                    UpdateImage(m);
                }
                
                SetFocus(adtdb);
                break;
            }
            case ColorTextButton: {

                bool success = true;// alpha to 0 weird windows bug
                uint32_t c = change_alpha(PickColorFromDialog(m, change_alpha(textColor, 0), &success), 255);
                if (success) {
                    textColor = c;
                }
                SendMessage(hwnd, WM_CTLCOLORSTATIC, (WPARAM)GetDC(hwnd), (LPARAM)hwnd);
                
                HWND tb = GetDlgItem(hwnd, ColorTextBlock);

                RECT rect;
                GetClientRect(tb, &rect);
                InvalidateRect(tb, &rect, TRUE);
                MapWindowPoints(tb, hwnd, (POINT*)&rect, 2);
                RedrawWindow(hwnd, &rect, NULL, RDW_ERASE | RDW_INVALIDATE);
                // set active here
                SetFocus(hwnd);
                SetFocus(adtdb);
                UpdateImage(m);

                break;
            }
            case IDOK: {

                ConfirmEffect();

                dontdo = true;
                freeness();
                EndDialog(hwnd, IDCANCEL);
                break;
            }
            case IDCANCEL: {
                dontdo = true;
                freeness();
                EndDialog(hwnd, IDCANCEL);
            }
            }
            break;
        }
        case WM_HSCROLL: {
            UpdateImage(m);
            return TRUE;
        }
    case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = (HDC)wparam;
            HWND hwndStatic = (HWND)lparam;

            if (GetDlgCtrlID(hwndStatic) == ColorTextBlock) {
                if (hBrush) {
                    DeleteObject(hBrush);
                }
                hBrush = CreateSolidBrush(change_alpha(textColor,0));
                SetBkColor(hdcStatic, change_alpha(textColor, 0));
                return (INT_PTR)hBrush;
            }
            else {
                return DefWindowProc(hwnd, msg, wparam, lparam);
            }
            break;
    }
    case WM_CLOSE: {
        
        dontdo = true;
        freeness();
        
        EndDialog(hwnd, IDCANCEL);
        }
    default: {
        return FALSE;
    }
    }
    return TRUE;
}