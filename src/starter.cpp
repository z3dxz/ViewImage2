
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <windows.h>
#include <iostream>
#include <thread>
#include <string>
#include "../res/resource.h"
#include <cstdint>
#include "headers/ops.hpp"
//#include <dwmapi.h>
#include <uxtheme.h>
#include "headers/globalvar.hpp"
#include "headers/imgload.hpp"
#include "headers/events.hpp"
// draw vars

GlobalParams gp;

GlobalParams* mv = &gp;
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);


/*
 ▄               ▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄         ▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄       ▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄
▐░▌             ▐░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░▌       ▐░▌▐░░░░░░░░░░░▌▐░░▌     ▐░░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌
 ▐░▌           ▐░▌  ▀▀▀▀█░█▀▀▀▀ ▐░█▀▀▀▀▀▀▀▀▀ ▐░▌       ▐░▌ ▀▀▀▀█░█▀▀▀▀ ▐░▌░▌   ▐░▐░▌▐░█▀▀▀▀▀▀▀█░▌▐░█▀▀▀▀▀▀▀▀▀ ▐░█▀▀▀▀▀▀▀▀▀
  ▐░▌         ▐░▌       ▐░▌     ▐░▌          ▐░▌       ▐░▌     ▐░▌     ▐░▌▐░▌ ▐░▌▐░▌▐░▌       ▐░▌▐░▌          ▐░▌
   ▐░▌       ▐░▌        ▐░▌     ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌   ▄   ▐░▌     ▐░▌     ▐░▌ ▐░▐░▌ ▐░▌▐░█▄▄▄▄▄▄▄█░▌▐░▌ ▄▄▄▄▄▄▄▄ ▐░█▄▄▄▄▄▄▄▄▄
	▐░▌     ▐░▌         ▐░▌     ▐░░░░░░░░░░░▌▐░▌  ▐░▌  ▐░▌     ▐░▌     ▐░▌  ▐░▌  ▐░▌▐░░░░░░░░░░░▌▐░▌▐░░░░░░░░▌▐░░░░░░░░░░░▌
	 ▐░▌   ▐░▌          ▐░▌     ▐░█▀▀▀▀▀▀▀▀▀ ▐░▌ ▐░▌░▌ ▐░▌     ▐░▌     ▐░▌   ▀   ▐░▌▐░█▀▀▀▀▀▀▀█░▌▐░▌ ▀▀▀▀▀▀█░▌▐░█▀▀▀▀▀▀▀▀▀
	  ▐░▌ ▐░▌           ▐░▌     ▐░▌          ▐░▌▐░▌ ▐░▌▐░▌     ▐░▌     ▐░▌       ▐░▌▐░▌       ▐░▌▐░▌       ▐░▌▐░▌
	   ▐░▐░▌        ▄▄▄▄█░█▄▄▄▄ ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌░▌   ▐░▐░▌ ▄▄▄▄█░█▄▄▄▄ ▐░▌       ▐░▌▐░▌       ▐░▌▐░█▄▄▄▄▄▄▄█░▌▐░█▄▄▄▄▄▄▄▄▄
		▐░▌        ▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░░▌     ▐░░▌▐░░░░░░░░░░░▌▐░▌       ▐░▌▐░▌       ▐░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌
		 ▀          ▀▀▀▀▀▀▀▀▀▀▀  ▀▀▀▀▀▀▀▀▀▀▀  ▀▀       ▀▀  ▀▀▀▀▀▀▀▀▀▀▀  ▀         ▀  ▀         ▀  ▀▀▀▀▀▀▀▀▀▀▀  ▀▀▀▀▀▀▀▀▀▀▀

 Welcome! The program starts here. If you have any questions about the code, please talk to me at github.com

 Note: Although the code is C++, the code is mostly in C style. Object oriented programming is not apparent and "classes" are just
 structs being passed through arguments


Thank you Sean T. Barrett for making stb_image(_resize/_write) and this entire image viewer possible


*/


int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {

	// get argument info
	int argc;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	std::string CLASS_NAME = gp.name_full + " Primary Class";
	std::string WINDOW_NAME = gp.name_full + " (Loading)";

	WNDCLASSEX wc = { 0 };

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wc.lpszClassName = CLASS_NAME.c_str();
	wc.lpfnWndProc = WndProc;
	//wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.style = CS_HREDRAW | CS_VREDRAW;

	RegisterClassEx(&wc);

	RECT ws = { 0, 0, gp.width, gp.height };
	AdjustWindowRectEx(&ws, WS_OVERLAPPEDWINDOW, FALSE, 0);
	int w_width = ws.right - ws.left;
	int w_height = ws.bottom - ws.top;
	 
	uint32_t w = GetSystemMetrics(SM_CXSCREEN);
	uint32_t h = GetSystemMetrics(SM_CYSCREEN);

	uint32_t px = (w / 2) - (gp.width / 2);
	uint32_t py = (h / 2) - (gp.height / 2);


	// RIGHT HERE: WIDTH = 1024

	gp.hwnd = CreateWindowEx(0, CLASS_NAME.c_str(), WINDOW_NAME.c_str(), WS_OVERLAPPEDWINDOW | WS_VISIBLE, px, py, w_width, w_height, NULL, NULL, NULL, NULL);

	// RIGHT HERE: WIDTH = 0

	gp.hdc = GetDC(gp.hwnd);

	if (!Initialization(&gp, argc, argv)) {
		return 0;
	}

	// WASD Replaced with TIMER! Look under WNDPROC Below
	SetTimer(gp.hwnd, 1, 12, NULL);

	MSG msg{};

	while (GetMessage(&msg, 0, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		
	}
	return 0;
}


LRESULT CALLBACK WndProcNormal(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
LRESULT CALLBACK WndProcDialogDrawText(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);


LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	LRESULT retv = 0;
	if(!gp.drawtext_access_dialog_hwnd) {
		retv = WndProcNormal(hwnd, msg, wparam, lparam);
	} else {
		retv = WndProcDialogDrawText(hwnd, msg, wparam, lparam);
	}
	return retv;
}


#define CheckEssential(hwnd, msg, wparam, lparam) \
\
		case WM_GETMINMAXINFO:\
		{\
			LPMINMAXINFO lpMMI = (LPMINMAXINFO)lparam;\
			lpMMI->ptMinTrackSize.x = 505;\
			lpMMI->ptMinTrackSize.y = 220;\
\
\
			break;\
		}\
		case WM_SIZE: {\
			Size(&gp);\
\
			break;\
		}\
		case WM_CLOSE: {\
			if (doIFSave(&gp)) {\
				gp.loading = true;\
				RedrawSurface(&gp);\
				DeleteTempFiles(&gp, gp.undofolder);\
				DestroyWindow(hwnd);\
			}\
			break;\
		}\
		case WM_DESTROY: {\
			PostQuitMessage(0);\
			return 0;\
		}\
		case WM_PAINT: {\
			UpdateBuffer(&gp);\
			return DefWindowProc(hwnd, msg, wparam, lparam);\
		}\
		case WM_SETFOCUS: {\
			gp.sleepmode = false;\
			if (gp.scrdata && gp.width > 1) {\
				if(gp.drawtext_access_dialog_hwnd) {\
				RedrawSurfaceTextDialog(&gp);\
				}\
				else {\
					RedrawSurface(&gp);\
				}\
			}\
			break;\
		}\
		case WM_KILLFOCUS: {\
\
			gp.sleepmode = true;\
			for (int i = 0; i < 50; i++) {\
				ShowCursor(500);\
			}\
			break;\
		}\

bool md_dt = false;
LRESULT CALLBACK WndProcDialogDrawText(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch(msg) {
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_LBUTTONDOWN: {
			gp.drawtext_ghostmode = true;
			PerformDrawTextRealignment();
			md_dt = true;
			break;
		}
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
		case WM_LBUTTONUP: {
			gp.drawtext_ghostmode = false;
			md_dt = false;
			HWND dtd = gp.drawtext_access_dialog_hwnd;
			if(dtd) {
    			HWND adtdb = GetDlgItem(dtd, ActualDrawTextDialogBox);
				SetFocus(adtdb);
			}

			break;
		}
		case WM_MOUSEMOVE: {
			SetCursor(LoadCursor(NULL, IDC_SIZEALL));
			if(md_dt) {
				PerformDrawTextRealignment();
			}
			ShowCursor(1);
			break;
		}
		CheckEssential(hwnd, msg, wparam, lparam)
		default: {
			return DefWindowProc(hwnd, msg, wparam, lparam);
		}
		
	}
	return 0;
}

LRESULT CALLBACK WndProcNormal(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {


	switch (msg) {
	case WM_CREATE: {


		//BOOL enable = TRUE;
		//DwmSetWindowAttribute(hwnd, 20, &enable, sizeof(enable));

		if (!DwmDarken(hwnd)) {
			// windows XP
		}

		break;
	}
	case WM_TIMER: {

		PerformWASDMagic(&gp);

		break;
	}
	case WM_DROPFILES: {
		// drag and drop, that was easy!

		HDROP hDrop = (HDROP)wparam;
		
		UINT count = DragQueryFileA(hDrop, 0xFFFFFFFF, nullptr, 0);
		if (count > 0)
		{
			char path[MAX_PATH];
			if (DragQueryFileA(hDrop, 0, path, MAX_PATH))
			{
				gp.loading = true;
				RedrawSurface(&gp);

				OpenImageFromPath(&gp, path, false);

				gp.loading = false;
				RedrawSurface(&gp);
				gp.shouldSaveShutdown = false;
			}
		}
		
		DragFinish(hDrop);
		RedrawSurface(&gp, false, false, true);
		return 0;
	}
	case WM_LBUTTONDOWN:
	{
		if (!ToolbarMouseDown(&gp)) {
			return 0;
		}
	}
	case WM_MBUTTONDOWN: {

		MouseDown(&gp);
		break;
	}
	case WM_LBUTTONUP:
	case WM_MBUTTONUP: {
		MouseUp(&gp, true);
		break;
	}
	case WM_RBUTTONUP: {

		RightUp(&gp);

		break;
	}
	case WM_RBUTTONDOWN: {
		if(!gp.drawtext_access_dialog_hwnd) {
			RightDown(&gp);
		}
		break;
	}
	case WM_MOUSEMOVE: {
		MouseMove(&gp);
		break;
	}

	case WM_SYSKEYDOWN: { // fix alt stalling issue that has ALWAYS existed before 2.6, even in 1.0
		return 0;
	}
	case WM_SYSKEYUP: {
		return 0;
	}
	case WM_KEYDOWN: {
		KeyDown(&gp, wparam, lparam);
		break;
	}

	
	case WM_KEYUP: {
		RedrawSurface(&gp);
		break;
	}
	case WM_MOUSEWHEEL: {
		MouseWheel(&gp, wparam, lparam);
		break;
	}
	
	CheckEssential(hwnd, msg, wparam, lparam)
	default: {
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	}


	return 0;
}
