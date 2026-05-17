#include "headers/gaussian.h"
#include "../../res/resource.h"
#include <Uxtheme.h>
#include <windowsx.h>
//#include <dwmapi.h>

static GlobalParams* m;


HWND gslider;

// Function prototypes
static LRESULT CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

int ShowGaussianDialog(GlobalParams* m0) {
    m = m0;
    // Create the main dialog
    DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(Gaussian), m->hwnd, (DLGPROC)DialogProc);

    return 0;
}

static void ApplyEffectToBuffer(float amount) {
    uint32_t* from = (uint32_t*)m->imgdata;
    uint32_t* to = (uint32_t*)m->imagepreview;
    //fast_gaussian_blur(from, to, m->imgwidth, m->imgheight, 4, 4.0f, 10, Border::kKernelCrop);
    //std::swap(m->imgdata, m->imagepreview);
    
    // gaussian B previously
    gaussian_blur_real(from, to, m->imgwidth, m->imgheight, amount, m->imgwidth, m->imgheight, 0, 0);
}

static void ConfirmEffect() {
    createUndoStep(m, true);
    memcpy(m->imgdata, m->imagepreview, m->imgwidth * m->imgheight * 4);
    m->shouldSaveShutdown = true;
}

LRESULT CALLBACK SliderProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

static LRESULT CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {

    switch (msg) {

    case WM_INITDIALOG: {

        m->imagepreview = malloc(m->imgwidth * m->imgheight * 4);
        memcpy(m->imagepreview, m->imgdata, m->imgwidth * m->imgheight * 4);
        m->isImagePreview = true;

        if (!DwmDarken(hwnd)) {
            // windows XP
        }

        gslider = GetDlgItem(hwnd, SLIDERGAUSSIAN);

        SendMessage(gslider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 40));
        SendMessage(gslider, TBM_SETPOS, TRUE, 0);

        SetWindowSubclass(gslider, SliderProc, 0, 0);

        RedrawSurface(m);
        return FALSE;
    }
    case WM_HSCROLL: {
        return TRUE;
    }
    case WM_COMMAND: {
        switch (LOWORD(wparam)) {
        case IDOK: {
            // Confirm
            ConfirmEffect();

            m->isImagePreview = false;
            if (m->imagepreview) {
                FreeData(m->imagepreview);
            }
            EndDialog(hwnd, IDCANCEL);
            break;
        }
        case IDCANCEL: {
            m->isImagePreview = false;
            if (m->imagepreview) {
                FreeData(m->imagepreview);
            }
            EndDialog(hwnd, IDCANCEL);
        }
        }
        break;
    case WM_CLOSE: {
        m->isImagePreview = false;
        if (m->imagepreview) {
            FreeData(m->imagepreview);
        }
        EndDialog(hwnd, IDCANCEL);
    }
    default:
        return FALSE;
    }
    }
    return TRUE;
}

LRESULT CALLBACK SliderProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR ref)
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

	case WM_MOUSEMOVE: {
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

        ApplyEffectToBuffer(pos);
        RedrawSurface(m);
        
		return 0;
    }
	case WM_LBUTTONUP:
		dragging = FALSE;
		ReleaseCapture();
        
		return 0;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

/*

            float posg = (float)SendMessage(hwnd, TBM_GETPOS, 0, 0);
            
            ApplyEffectToBuffer(posg);
*/