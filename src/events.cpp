#include "headers/events.hpp" // e
#include "../res/resource.h"
#include <Shlwapi.h>
#include <vector>
#include <fstream>
#include <thread>
#include <limits>
#include <future>
#include <string>
#include <iostream>
#include <sstream>
#include <cctype>
#include <future>
#include <mutex>
#include <condition_variable>
#include <random>
void ToggleFullscreen(GlobalParams* m);

std::mutex mtx;
std::condition_variable cv;
bool resizeCompleted = false;

void ResizeBuffers(GlobalParams* m) {
	RECT ws = { 0 };
	GetClientRect(m->hwnd, &ws);
	int newWidth = ws.right - ws.left;
	int newHeight = ws.bottom - ws.top;
	m->width = newWidth/m->uiscale;
	m->height = newHeight/m->uiscale;
	m->rlwidth = newWidth;
	m->rlheight = newHeight;
}


void TurnOnLoad(GlobalParams* m) {
	if(m->loading) {
		return;
	}
	m->loading = true;
	RedrawForce(m);
}


void TurnOffLoad(GlobalParams* m) {
	if(!m->loading) {
		return;
	}
	m->loading = false;
	RedrawForce(m, true);
}

void ResetCoordinates(GlobalParams* m) {

	if (m->imgwidth > 0) {
		m->CoordLeft = ((float)m->rlwidth - m->imgwidth * m->mscaler) / 2.0f + m->iLocX;
		m->CoordTop = ((float)m->rlheight - m->imgheight * m->mscaler) / 2.0f + m->iLocY;

		m->CoordRight = ((float)m->rlwidth + m->imgwidth * m->mscaler)/2.0f + m->iLocX;
		m->CoordBottom = ((float)m->rlheight + m->imgheight * m->mscaler)/2.0f + m->iLocY;
	}
	else {
		m->CoordLeft = 0;
		m->CoordTop = 0;
		m->CoordRight = 0;
		m->CoordBottom = 0;
	}
}


void Size(GlobalParams* m) {
	// go back to this
	ResizeBuffers(m);
	autozoom(m);
	RedrawSurface(m);
}

HANDLE hMutex;

void UpdateMousePos(GlobalParams* m) {
	GetCursorPos(&m->mpos);
	GetCursorPos(&m->mrawpos);
	GetCursorPos(&m->gmpos);
	ScreenToClient(m->hwnd, &m->mpos);
	ScreenToClient(m->hwnd, &m->mrawpos);
	m->mpos.x /= m->uiscale;
	m->mpos.y /= m->uiscale;
}

// initiz
// initz
bool Initialization(GlobalParams* m, int argc, LPWSTR* argv) {
	
	char cd[256];
	GetCurrentDirectory(256, cd);
	m->current_directory = std::string(cd);
	
	m->toolbartable = {
	   {0,   "Open Image (F)" , false},
	   {31,  "Save Image (CTRL+S)", true},
	   {62,  "Zoom In", false},
	   {93,  "Zoom Out", false},
	   {124, "Zoom Auto", false},
	   {155, "Zoom 1:1 (100%)", true},
	   {186, "Rotate", false},
	   {217, "Annotate (G)", false},
	   {248, "Image Operations", true},
	   {279, "DELETE image", false},
	   {310, "Print", false},
	   {341, "Copy Image", true},
	   {372, "Information", false},
	};

	char buffer[MAX_PATH];
	DWORD length = GetModuleFileName(nullptr, buffer, MAX_PATH);
	PathRemoveFileSpec(buffer);

	m->cd = std::string(buffer);

	// load fonts here

	m->Verdana = LoadFont(m, "Verdana.ttf");
	m->SegoeUI = LoadFont(m, "SegoeUI.ttf");
	m->OCRAExt = LoadFont(m, "OCRAEXT.ttf");

	m->toolbarData = LoadOpenGLImageFromResource(TOOLBAR_RES);

	// sliders
	m->brush_size_slider = {113, 14, 214, 31, &m->drawMenuOffsetX, &m->drawMenuOffsetY, false};
	m->brush_opacity_slider = {318, 14, 405, 31, &m->drawMenuOffsetX, &m->drawMenuOffsetY, false};

	// icons
	m->menu_icon_atlas = LoadOpenGLImageFromResource(ICON_MAP);
	m->fullscreenIconData = LoadOpenGLImageFromResource(FS_ICON);
	m->dmguideIconData = LoadOpenGLImageFromResource(DMGUIDEICON);
	m->cropImageData = LoadOpenGLImageFromResource(CROPICON);

	// TEMPORARY FILES STUFF

	DWORD pathLength = MAX_PATH;
	TCHAR tempPath[MAX_PATH];
	GetTempPath(pathLength, tempPath);


	std::string firstfolder = std::string(tempPath) + m->name_full;
	CreateDirectory(firstfolder.c_str(), NULL);
	
	hMutex = CreateMutex(NULL, TRUE,  (m->name_primary+"MainProcessUndoCatalogPackagesTemporaryFilesMutex").c_str());
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		// Great!
	} 
	else {
		bool isempty = PathIsDirectoryEmpty(firstfolder.c_str());
		if (!isempty) {
			m->deletingtemporaryfiles = true;

			// nonreplace image
			RedrawSurface(m);

			// for notice
			m->deletingtemporaryfiles = false;

			// nonreplace image
			RedrawSurface(m);
			DeleteTempFiles(m, firstfolder);

			CreateDirectory(firstfolder.c_str(), NULL);
		}
	}

	int PID = GetCurrentProcessId();

	m->undofolder = std::string(tempPath) + m->name_full + "\\VIEW_IMAGE_TEMPORARY_DATA_" + std::to_string(PID) + "\\";
	if (!CreateDirectory(m->undofolder.c_str(), NULL)) {
		MessageBox(m->hwnd, "Unable to create process-specific temporary files folder", "Error", MB_OK | MB_ICONERROR);
		exit(0);
		return false;
	}
	
	auto wideToUtf8 = [](const wchar_t* w) -> std::string {
		if (!w) return {};
		int len = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
		std::string s(len - 1, '\0');
		WideCharToMultiByte(CP_UTF8, 0, w, -1, s.data(), len, nullptr, nullptr);
		return s;
	};

	std::string firstArg;
	std::string secondArg;
	std::string thirdArg;

	if (argc > 1) firstArg = wideToUtf8(argv[1]);
	if (argc > 2) secondArg = wideToUtf8(argv[2]);
	if (argc > 3) thirdArg = wideToUtf8(argv[3]);

	std::cout << "F: " << firstArg << "\n";
	std::cout << "S: " << secondArg << "\n";
	std::cout << "T: " << thirdArg << "\n";

	if (firstArg != "") {
		if (!OpenImageFromPath(m, firstArg, false)) {
			MessageBox(m->hwnd, "Unable to open image", "Error", MB_OK | MB_ICONERROR);
			exit(0);
			return false;
		}
	}
		
	Size(m);

	char work[256];
	GetCurrentDirectory(256, work);

	if (secondArg != "") {
		if (secondArg == "--full") {
			ToggleFullscreen(m);
		}
	}

	// timers
	QueryPerformanceFrequency(&m->frequency);
	QueryPerformanceCounter(&m->previousTime);
	
	// drag and drop
	DragAcceptFiles(m->hwnd, TRUE);

	return true;
}

uint32_t PickColorFromDialog(GlobalParams* m, uint32_t def, bool* success) {
	CHOOSECOLOR cc;
	COLORREF acrCustClr[16];
	ZeroMemory(&cc, sizeof(cc));
	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = m->hwnd;
	cc.lpCustColors = (LPDWORD)acrCustClr;
	cc.rgbResult = def; // Default color
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;

	if (ChooseColor(&cc) == TRUE) {
		return (uint32_t)cc.rgbResult;
	}
	else {
		*success = false;
		return 0;
	}
}


void PerformWASDMagic(GlobalParams* m) {

	bool isW = GetKeyState('W') & 0x8000;
	bool isA = GetKeyState('A') & 0x8000;
	bool isS = GetKeyState('S') & 0x8000;
	bool isD = GetKeyState('D') & 0x8000;

	if(!isW && !isA && !isS && !isD && (fabsf(m->wasdX) < 0.0001f && fabsf(m->wasdY) < 0.0001f)) {
		return;
	}

	LARGE_INTEGER currentTime;
	QueryPerformanceCounter(&currentTime);

    double dt0 = double(currentTime.QuadPart - m->previousTime.QuadPart) / double(m->frequency.QuadPart);
	m->previousTime = currentTime;

	if (dt0 <= 0.0) {
		return;
	};

	const float max_deltatime = 1.0f / 15.0f;

	float dt = (float)dt0;
	if (dt > max_deltatime) dt = max_deltatime;

	HWND temp = GetActiveWindow();
	if (temp != m->hwnd) {
		return;
	}

	float acceleration = 10820.0f;
    const float damping = 18.0f;   // higher = more friction
	
    // Acceleration from input
    float ax = 0.0f;
    float ay = 0.0f;

	bool isShiftOrControl = (GetKeyState(VK_SHIFT) & 0x8000) || (GetKeyState(VK_CONTROL) & 0x8000);

	if(!isShiftOrControl) {
		if (isW) ay -= acceleration;
		if (isS) ay += acceleration;
		if (isA) ax -= acceleration;
		if (isD) ax += acceleration;	
	}
    
    m->wasdX += ax * dt;
    m->wasdY += ay * dt;

    float friction = expf(-damping * dt);
	m->wasdX *= friction;
	m->wasdY *= friction;

    float moveX = m->wasdX * dt;
    float moveY = m->wasdY * dt;

    m->iLocX -= moveX;
    m->iLocY -= moveY;
	std::string vv = std::to_string(m->wasdX);

	MouseMove(m, false);

	// nonreplace image
	RedrawSurface(m);
}


void UndoBus(GlobalParams*m );


void OpenImageEffectsMenu(GlobalParams* m) {
	m->menuVector = {

		{"Automatic Adjust",
			[m]() -> bool {
				bool did = AutoAdjustLevels(m, (uint32_t*)m->imgdata, 7.0);
				return true;
			},79,14, &m->item_enabled, true
		},

		{"Brightness/Contrast{s}",
			[m]() -> bool {
				m->isMenuState = false;
				// nonreplace image
				RedrawSurface(m);
				ShowBrightnessContrastDialog(m);
				return true;
			},66,1, &m->item_enabled, true
		},

		{"Invert Colors",
			[m]() -> bool {
				m->isMenuState = false;
				// modify
				m->shouldSaveShutdown = true;
				createUndoStep(m, true);
				TurnOnLoad(m);
				for (int y = 0; y < m->imgheight; y++) {
					for (int x = 0; x < m->imgwidth; x++) {
						uint32_t* loc = GetMemoryLocation(m->imgdata, x, y, m->imgwidth, m->imgheight);
						// seperate RGB
						uint32_t color = *loc;

						uint8_t a = (color >> 24) & 0xFF;
						uint8_t r = (color >> 16) & 0xFF;
						uint8_t g = (color >> 8) & 0xFF;
						uint8_t b = color & 0xFF;

						r = 255 - r; g = 255 - g; b = 255 - b;
						*loc = (a << 24) | (r << 16) | (g << 8) | b;
					}
				}
				TurnOffLoad(m);
				return true;
			},66,14, &m->item_enabled, true
		},


		{"Gaussian Blur{s}",
			[m]() -> bool {
				m->isMenuState = false;
				// nonreplace image
				RedrawSurface(m);
				ShowGaussianDialog(m);
				return true;
			},92,1, &m->item_enabled, true
		},

		{"Draw Text",
			[m]() -> bool {

				m->isMenuState = false;
				// nonreplace image
				RedrawSurface(m);
				ShowDrawTextDialog(m);
				return true;
			},1,14, &m->item_enabled, true
		},

		{"Crop Image{s}",
			[m]() -> bool {
				autozoom(m);
				m->mscaler = m->mscaler * 0.8f;
				TurnOffDraw(m);
				m->drawmode = false;
				m->isInCropMode = true;
				// nonreplace image
				RedrawSurface(m);
				return true;
			},53,14, &m->item_enabled, true
		},

		{"Erase annotations",
			[m]() -> bool {
				createUndoStep(m,true);
				TurnOnLoad(m);
				memcpy(m->imgdata, m->imgoriginaldata, m->imgwidth * m->imgheight * 4);
				m->shouldSaveShutdown = true;
				TurnOffLoad(m);
				return true;
			},79,1, &m->isimage_menucondition, true
		},
	};

	m->menuX = GetLocationFromButton(m, 8); // 8 = effects
	m->menuY = m->toolheight;
	m->isMenuState = true;

	// calls when image effects menu opens on option 8 (effects)
	// nonreplace image
	RedrawSurface(m);
}

void ShowMyInformation(GlobalParams* m) {
	

	std::string txt = "Nothing";
	if (!m->fpath.empty()) {
		txt = m->fpath;
	}

	std::string information = "Currently Loaded:\n" + txt + " " 
	+ std::to_string(m->imgwidth) + "x" +  std::to_string(m->imgheight) 
	+ " \n\nVisit cosine64.com for more quality software\n\nVersion: " + REAL_BIG_VERSION 
	+ "\nBuild Type: " + BUILD_TYPE + "\nBuild Time: " + __DATE__ + " at " + __TIME__ + "\nBuild System: " + BUILD_SYSTEM + "\n";
	
	MessageBox(m->hwnd, information.c_str(), "Information", MB_OK);
}

int PerformCasedBasedOperation(GlobalParams* m, uint32_t id) {
	if (m->imgwidth > 0 || id == 0) {}
	else { return 1; }
	switch (id) {
	case 0:
		// open
		PrepareOpenImage(m);
		return 0;
	case 1:
		// save
		
		if (GetKeyState(VK_SHIFT) & 0x8000) {
			
			int pos = m->fpath.find(".");
			std::string ext = m->fpath.substr(pos+1);
			std::transform(ext.begin(), ext.end(), ext.begin(),    [](unsigned char c){ return std::tolower(c); });

			std::string name = m->current_directory + "\\Image" + std::to_string(rand()) + "." + ext;
			

			bool save = ActuallySaveImage(m, name);
		} else {
			PrepareSaveImage(m);
		}

		return 0;
	case 2:
		// zoom in
		NewZoom(m, 1.25f, 2, true); // ALSO: Use this for the + and - hotkeys for ZOOM
		return 0;
	case 3:
		// zoom out
		NewZoom(m, 0.8f, 2, true); // ALSO HERE TOO 
		return 0;
	case 4:
		// zoom fit
		autozoom(m);
		return 0;
	case 5:

		// zoom original
		m->mscaler = 1.0f;

		// nonreplace image
		RedrawSurface(m);
		return 0;
	case 6:
		// rotate

		rotateImage90Degrees(m);
		return 0;
	case 7: {
		// draw
		m->drawmode = !m->drawmode;
		// nonreplace image
		RedrawSurface(m);
		return 0;
	}
	case 8: {
		// effects
		if(m->isMenuState) {
			m->isMenuState = false;
			// nonreplace image
			RedrawSurface(m);
			return 0;
		}

		OpenImageEffectsMenu(m);
		return 0;
	}
	case 9: {
		// DELETE
		if (m->fpath != "Untitled") {

			// delete
			int result = MessageBox(m->hwnd, "This will delete the image permanently!!!", "Are You Sure?", MB_YESNO | MB_ICONQUESTION);
			if (result == IDYES) {
				bool i = DeleteFile(m->fpath.c_str());
				if (!i) {
					int err = GetLastError();
					MessageBox(m->hwnd, std::string("Can not delete file - Error: " + std::to_string(err)).c_str(), "Error", MB_OK | MB_ICONERROR);
					return 0;
				}
				CHAR szPath[MAX_PATH];
				GetModuleFileName(NULL, szPath, MAX_PATH);

				ShellExecute(NULL, "open", szPath, NULL, NULL, SW_SHOWNORMAL);

				ExitProcess(0);
			}
		}
		else {
			MessageBox(m->hwnd, "The image that is loaded is temporary and can not be deleted", "Untitled", MB_OK);
		}


		return 0;
	}
	case 10: {
		// print
		Print(m);

		return 0;
	}
	case 11: {

		// copy
		if (!CopyImageToClipboard(m, m->imgdata, m->imgwidth, m->imgheight)) {
			MessageBox(m->hwnd, "Error copying the image to the clipboard", "Error", MB_OK | MB_ICONERROR);
		}

		return 0;
	}
	case 12: {


		// information
		// image info
		
		ShowMyInformation(m);
		return 0;
	}
	}

	return 1;
}

bool MouseDownCases(GlobalParams* m){
	UpdateMousePos(m);

	// mpos = mPP
	bool menugateway = false;
	
	// [Mouse Down] Menu inactive gateway
	if (m->isMenuState && !(IfInMenu(m->mpos, m))) {
		m->isMenuState = false;
		// nonreplace image
		RedrawSurface(m);
		menugateway = true;
		// silent: do not stop past input
	}
	if (m->isInCropMode) {
		if (m->imgwidth < 1) {
			PrepareOpenImage(m);
		}

		m->isMovingTL = m->CropHandleSelectTL;
		m->isMovingTR = m->CropHandleSelectTR;
		m->isMovingBL = m->CropHandleSelectBL;
		m->isMovingBR = m->CropHandleSelectBR;

		return 0;
	}

	if (m->eyedroppermode) {
		// eyedropper here
		
		int k1 = (int)((m->mpos.x - m->CoordLeft) / m->mscaler);
		int v1 = (int)((m->mpos.y - m->CoordTop) / m->mscaler);
		if(k1 >= 0 && k1 < m->imgwidth && v1 >= 0 && v1 < m->imgheight) {
			std::cout << k1 << " " << v1 << "\n";
			uint32_t color = *GetMemoryLocation(m->imgdata, k1, v1, m->imgwidth, m->imgheight);
			m->a_drawColor = color;
			m->eyedroppermode = false;
			MouseMove(m);
			m->drawtype = 1;
		}

		// nonreplace image
		RedrawSurface(m);

		
		return 0;
	}

	// [Mouse Down] Menu logic
	if (m->isMenuState && IfInMenu(m->mpos, m)) {
		int selected = (m->mpos.y - (m->actmenuY + 2)) / m->mH;
		if (selected < m->menuVector.size()) {
			auto l = m->menuVector[selected].func;
			if(*(m->menuVector[selected].enable_condition) && l) {
				l();
				if(m->menuVector[selected].close_menu) {
					m->isMenuState = false;
				}
			}
		}
		return 0;
	}

	// [Mouse Down] Drawing toolbar
	if (m->drawmode)
	{
		int colorBeginX = m->drawMenuOffsetX + 51;
		int colorEndX = m->drawMenuOffsetX + 69;
		int colorBeginY = m->drawMenuOffsetY + 14;
		int colorEndY = m->drawMenuOffsetY + 32;

		int softBeginX = m->drawMenuOffsetX + 461;
		int softEndX = m->drawMenuOffsetX + 479;
		int softBeginY = m->drawMenuOffsetY + 14;
		int softEndY = m->drawMenuOffsetY + 32;

		if ((m->mpos.x > colorBeginX && m->mpos.x < colorEndX) && (m->mpos.y > colorBeginY && m->mpos.y < colorEndY)) { //color icon coordinates
			// open color picker
			bool success = true;// alpha to 0 weird windows bug
			uint32_t c = change_alpha(PickColorFromDialog(m, InvertCC(change_alpha(m->a_drawColor,0),true), &success), 255);
			if (success) {
				m->a_drawColor = InvertCC(c, true);
				m->drawtype = 1;
			}
			// nonreplace image
			RedrawSurface(m);
			return 0;
		}
		if ((m->mpos.x > softBeginX && m->mpos.x < softEndX) && (m->mpos.y > softBeginY && m->mpos.y < softEndY)) { // soft had
			// open soft hard
			m->a_softmode = !m->a_softmode;
			// nonreplace image
			RedrawSurface(m);
			return 0;
		}

		if(IsInSlider(m->brush_size_slider)) {
			m->brush_size_slider.md = true;
			return 0;
		}
		if(IsInSlider(m->brush_opacity_slider)) {
			m->brush_opacity_slider.md = true;
			return 0;
		}

		if (IsInImage(m->mrawpos, m)) {
			TurnOnDraw(m);
			createUndoStep(m, true);
			return 0;
		}
	}

	// [Mouse Down] dmguide 
	if (m->drawmode && (m->mpos.x > (m->dmguide_x) && m->mpos.x <= (m->dmguide_x+m->dmguide_sx))) {
		if((m->mpos.y > (m->dmguide_y) && m->mpos.y <= (m->dmguide_y+43))) {
			// pen
			m->drawtype = 1;
			// nonreplace image
			RedrawSurface(m);
			return 0;
		}
		if((m->mpos.y > (m->dmguide_y+43) && m->mpos.y <= (m->dmguide_y+84))) {
			// erase
			m->drawtype = 0;
			// nonreplace image
			RedrawSurface(m);
			return 0;
		}
		if((m->mpos.y > (m->dmguide_y+84) && m->mpos.y <= (m->dmguide_y+125))) {
			// transparent
			m->drawtype = 3;
			// nonreplace image
			RedrawSurface(m);
			return 0;
		}
		if((m->mpos.y > (m->dmguide_y+125) && m->mpos.y <= (m->dmguide_y+168))) {
			// eyedropper
			m->eyedroppermode = true;
			// nonreplace image
			RedrawSurface(m);
			return 0;
		}
	}

	// [Mouse Down] fullscreen icon
	if ((m->mpos.x > m->width - 36 && m->mpos.x < m->width - 13) && (m->mpos.y > 12 && m->mpos.y < 33)) { //fullscreen icon location check coordinates (ALWAYS KEEP)
		if (m->width > 535) { // to check to make sure window isn't too small
			ToggleFullscreen(m); // TODO: please make a seperate icon for the exiting fullscreen
		}

		return 0;
	}
	
	uint32_t id = getXbuttonID(m);
	
	// [Mouse Down] cased based toolbar buttons
	if(!menugateway || id != 8) {
		if(PerformCasedBasedOperation(m, id) == 0) {
			return 0;
		}
	}
	
	// [Mouse Down] toolbar (nothing)
	if (m->mpos.y <= m->toolheight || (!extracases(m->mpos, m))) {
		return 0;
	}
	// [Mouse Down] move mouse down
	m->movemousedown = true;
	return 0;
}

bool MouseDown(GlobalParams* m) {
	if(m->Leftdown) return 1; // both way: mouse move check down or OS check down will come first, first come first serve. do not do twice

	m->Leftdown = true;
	
	m->lastK = -1;
	m->lastV = -1;

	m->lockimgoffx = m->iLocX;
	m->lockimgoffy = m->iLocY;
	GetCursorPos(&m->LockmPos);
	
	MouseDownCases(m);
	
	MouseMove(m, false);
	return 0;
}



void TurnOnDraw(GlobalParams* m) {
	if (!m->drawmousedown) {
		//m->SetLastMouseForWASDInputCaptureProtectionLock = true;
		m->drawmousedown = true;
	}
}

void TurnOffDraw(GlobalParams* m) {
	if (m->drawmousedown) {
		m->drawmousedown = false;
	}
}

POINT* sampleLine(GlobalParams* m, double x1, double y1, double x2, double y2, int* outSamples) {
	float sizex = abs(x2 - x1);
	float sizey = abs(y2 - y1);
	float dist = sqrt(pow(sizex, 2) + pow(sizey, 2));
	int numSamples = dist / (m->drawSize/ m->a_resolution);

	int qualsamples = numSamples;
	if (numSamples > 2) { qualsamples--; };

	POINT* samples = (POINT*)vismalloc(sizeof(POINT) * (qualsamples + 1), "Drawing Line Sampling Algorithm"); // Include starting point

	float dx = (x2 - x1) / numSamples;
	float dy = (y2 - y1) / numSamples;

	samples[0].x = x1;
	samples[0].y = y1;

	for (int i = 1; i <= qualsamples; i++) {
		samples[i].x = x1 + i * dx;
		samples[i].y = y1 + i * dy;
	}

	*outSamples = qualsamples + 1; // Include starting point
	return samples;
}



static clock_t start, end;
void placeDraw(GlobalParams* m, POINT* pos) {
    float actdrawsize = m->drawSize;

    if (m->drawtype == 0 && m->drawSize > 1.5f) {
        actdrawsize *= 2.0f;
    }

    start = clock();

    const float target_dt = 0.016f;
    float dt_diff = target_dt - (m->ms_time+m->dt_time);
    float adjust = powf(2.0f, dt_diff * 10.0f);

    m->a_resolution = std::clamp(m->a_resolution * adjust, 1.0f, 25.0f);

    int k = m->lastK;
    int v = m->lastV;

    int k1 = (int)((pos->x - m->CoordLeft) / m->mscaler);
    int v1 = (int)((pos->y - m->CoordTop) / m->mscaler);

    float dx = (float)(k - k1);
    float dy = (float)(v - v1);
    float dist = sqrtf(dx * dx + dy * dy);

    if (dist <= (actdrawsize / m->a_resolution)) {
        end = clock();
        m->ms_time = (double)(end - start) / CLOCKS_PER_SEC;
        return;
    }

    if (m->lastK < 0 || m->lastV < 0) {
        k = k1;
        v = v1;
    }

    int samples0 = 0;
    POINT* k2 = sampleLine(m, k1, v1, k, v, &samples0);

    const int diameter = (int)actdrawsize;
    const float radius = diameter * 0.5f;
    const float realOpacity = powf(m->a_opacity, 4.0f);

    const bool ctrl  = (GetKeyState(VK_CONTROL) & 0x8000);
    const bool shift = (GetKeyState(VK_SHIFT)   & 0x8000);

    for (int i = 0; i < samples0; i++) {

        int baseX = k2[i].x;
        int baseY = k2[i].y;

        for (int y = 0; y < diameter; y++) {
            float vy = y + 0.5f;
            float dyc = vy - radius;

            for (int x = 0; x < diameter; x++) {
                float vx = x + 0.5f;
                float dxc = vx - radius;

                float distc = sqrtf(dxc * dxc + dyc * dyc);
                if (distc > radius) continue;

                uint32_t xloc = (uint32_t)(dxc + baseX);
                uint32_t yloc = (uint32_t)(dyc + baseY);

                if (xloc >= m->imgwidth || yloc >= m->imgheight) continue;

                uint32_t* memoryPath =  GetMemoryLocation(m->imgdata, xloc, yloc, m->imgwidth, m->imgheight);

                uint32_t* memoryPathOriginal = GetMemoryLocation(m->imgoriginaldata, xloc, yloc, m->imgwidth, m->imgheight);

                uint32_t actualDrawColor = m->a_drawColor;

				// color selection
                if (ctrl || m->Rightdown || m->drawtype == 0) {
                    actualDrawColor = *memoryPathOriginal;
                }

                if ((ctrl && shift) || m->drawtype == 3) {
                    actualDrawColor = 0x00000000;
                }

				// softness // falloff // anti-aliasing
                float transparency = 0.0f;

                if (m->a_softmode) {
                    float t = distc / radius;
                    transparency = t * t;
                } else if (actdrawsize > 1.01f) {
                    float edge = distc - radius + 1.0f;

                    if (edge < 0.001f) edge = 0.0f;
                    if (edge > 9.99f)  edge = 1.0f;

                    transparency = edge;
                }

                float radiusAlpha = (1.0f - transparency) * realOpacity;

				// write pixel
                if (actdrawsize > 1.01f) {

                    if (m->a_opacity > 0.99f && radiusAlpha > 0.995f) {
                        *memoryPath = actualDrawColor;
                    } else {
                        if (m->a_resolution > 20) {
                            *memoryPath = lerp_gc(*memoryPath, actualDrawColor, radiusAlpha);
                        } else {
                            uint8_t a = (uint8_t)(radiusAlpha * 255.0f);
                            *memoryPath = lerp_u32(*memoryPath, actualDrawColor, a);
                        }
                    }

                } else {
                    if (m->a_opacity > 0.99f) {
                        *memoryPath = actualDrawColor;
                    } else {
                        *memoryPath = lerp_gc(*memoryPath, actualDrawColor, realOpacity);
                    }
                }
            }
        }
    }

    FreeData(k2);

    m->lastK = k1;
    m->lastV = v1;
    m->shouldSaveShutdown = true;

	RedrawSurface(m, true);

    end = clock();
    m->ms_time = (double)(end - start) / CLOCKS_PER_SEC;
}


bool firsttime = true;
bool fullscreenhover = false;
void MouseMoveCases(LPSTR* cursor, HINSTANCE* cursorinstance, GlobalParams* m) {
	// i would probably min this
	if (m->isInCropMode) {
		*cursor = IDC_ARROW;
		uint32_t distLeft, distRight, distTop, distBottom;
		GetCropCoordinates(m, &distLeft, &distRight, &distTop, &distBottom);

		uint32_t range = 20;

		int mpx = m->mrawpos.x;
		int mpy = m->mrawpos.y;


		if (mpx < (distLeft + range) && mpx >(distLeft - range) && mpy < (distTop + range) && mpy >(distTop - range)) {
			*cursor = IDC_SIZENWSE;
			m->CropHandleSelectTL = true; m->CropHandleSelectTR = false; m->CropHandleSelectBL = false; m->CropHandleSelectBR = false;
		} else if (mpx < (distRight + range) && mpx >(distRight - range) && mpy < (distTop + range) && mpy >(distTop - range)) {
			*cursor = IDC_SIZENESW;
			m->CropHandleSelectTL = false; m->CropHandleSelectTR = true; m->CropHandleSelectBL = false; m->CropHandleSelectBR = false;
		} else if (mpx < (distLeft + range) && mpx >(distLeft - range) && mpy < (distBottom + range) && mpy >(distBottom - range)) {
			*cursor = IDC_SIZENESW;
			m->CropHandleSelectTL = false; m->CropHandleSelectTR = false; m->CropHandleSelectBL = true; m->CropHandleSelectBR = false;
		} else if (mpx < (distRight + range) && mpx >(distRight - range) && mpy < (distBottom + range) && mpy >(distBottom - range)) {
			*cursor = IDC_SIZENWSE;
			m->CropHandleSelectTL = false; m->CropHandleSelectTR = false; m->CropHandleSelectBL = false; m->CropHandleSelectBR = true;
		}
		else {
			m->CropHandleSelectTL = false; m->CropHandleSelectTR = false; m->CropHandleSelectBL = false; m->CropHandleSelectBR = false;
		}

		if (m->isMovingTL) {
			float perX, perY;
			GetCropPercentagesFromCursor(m, mpx, mpy, &perX, &perY);
			m->leftP = perX;
			m->topP = perY;
			if (m->leftP > m->rightP) { m->leftP = m->rightP; }
			if (m->topP > m->bottomP) { m->topP = m->bottomP; }
		}

		if (m->isMovingTR) {
			float perX, perY;
			GetCropPercentagesFromCursor(m, mpx, mpy, &perX, &perY);
			m->rightP = perX;
			m->topP = perY;
			if (m->rightP < m->leftP) { m->rightP = m->leftP; }
			if (m->topP > m->bottomP) { m->topP = m->bottomP; }
		}

		if (m->isMovingBL) {
			float perX, perY;
			GetCropPercentagesFromCursor(m, mpx, mpy, &perX, &perY);
			m->leftP = perX;
			m->bottomP = perY;
			if (m->leftP > m->rightP) { m->leftP = m->rightP; }
			if (m->bottomP < m->topP) { m->bottomP = m->topP; }
		}

		if (m->isMovingBR) {
			float perX, perY;
			GetCropPercentagesFromCursor(m, mpx, mpy, &perX, &perY);
			m->rightP = perX;
			m->bottomP = perY;
			if (m->rightP < m->leftP) { m->rightP = m->leftP; }
			if (m->bottomP < m->topP) { m->bottomP = m->topP; }
		}
		// nonreplace image
		RedrawSurface(m);

	} else if (m->eyedroppermode) {
		*cursorinstance = GetModuleHandle(NULL);
		*cursor = MAKEINTRESOURCE(IDC_CURSOR2);
	}
	else if (m->movemousedown) {
		*cursor = IDC_SIZEALL;
		// Moving around the image using left mouse button
		
		m->iLocX = m->lockimgoffx - (m->LockmPos.x - m->gmpos.x);
		m->iLocY = m->lockimgoffy - (m->LockmPos.y - m->gmpos.y);
		
		// nonreplace image
		RedrawSurface(m);

	}
	else if (m->brush_size_slider.md) {
		*cursor = IDC_SIZEWE;

		float findMid = (float)(m->mpos.x - (m->brush_size_slider.x+(*m->brush_size_slider.parentX))) / (float)((m->brush_size_slider.endX)-(m->brush_size_slider.x));

		if (findMid > 0.0f) {
			float eff = sqrt(m->imgheight-1);
			m->drawSize = pow((findMid*eff),2)+1;
			if (m->drawSize < 1.0f) { m->drawSize = 1.0f; }
		}
		else {
			m->drawSize = 1.0f;
		}
		
		// nonreplace image
		RedrawSurface(m);
	}
	else if (m->brush_opacity_slider.md) {
		*cursor = IDC_SIZEWE;
		float findMid = (float)(m->mpos.x - (m->brush_opacity_slider.x+(*m->brush_opacity_slider.parentX))) / (float)((m->brush_opacity_slider.endX)-(m->brush_opacity_slider.x));

		if (findMid >= 0.0f && findMid <= 1.0f) {
			m->a_opacity = findMid;
		}
		else if (findMid < 0) {
			m->a_opacity = 0;
		}
		else {
			m->a_opacity = 1.0f;
		}

		// nonreplace image
		RedrawSurface(m);
		
	} else if (m->isMenuState && IfInMenu(m->mpos, m)) {
		
		int last = m->menuselected;
		m->menuselected = (m->mpos.y-(m->actmenuY+2))/m->mH;
		if (m->menuselected >= 0 && m->menuselected < m->menuVector.size() && *(m->menuVector[m->menuselected].enable_condition) ) {
			*cursor = IDC_ARROW;
		}

		// nonreplace image
		if(m->menuselected != last) {
			RedrawSurface(m);
		}

	}
	else if (m->drawmousedown) {
		placeDraw(m, &m->mrawpos);

	}
	else if ((m->mpos.x > m->width - 36 && m->mpos.x < m->width - 13) && (m->mpos.y > 12 && m->mpos.y < 33)) {
		// fullscreen icon
		if(!fullscreenhover) {
			RedrawSurface(m);
			fullscreenhover = true;
		}
	} else if ((m->mpos.x > (m->dmguide_x) && m->mpos.x <= (m->dmguide_x+m->dmguide_sx)) && (m->mpos.y > (m->dmguide_y    ) && m->mpos.y <= (m->dmguide_y+168))) {
		// dm guide
		int last = m->dmguidebutton;
		if((m->mpos.y > (m->dmguide_y    ) && m->mpos.y <= (m->dmguide_y+43 ))) m->dmguidebutton = 0; // pen
		if((m->mpos.y > (m->dmguide_y+43 ) && m->mpos.y <= (m->dmguide_y+84 ))) m->dmguidebutton = 1; // erase
		if((m->mpos.y > (m->dmguide_y+84 ) && m->mpos.y <= (m->dmguide_y+125))) m->dmguidebutton = 2; // transparent
		if((m->mpos.y > (m->dmguide_y+125) && m->mpos.y <= (m->dmguide_y+168))) m->dmguidebutton = 3; // eyedropper

		if(last != m->dmguidebutton) {
			RedrawSurface(m);
		}
	}
	else if (m->mpos.y <= m->toolheight) {
		int last = m->selectedbutton;
		m->selectedbutton = getXbuttonID(m);
		
		if (m->selectedbutton >= 0 && m->selectedbutton < m->toolbartable.size()) {
			*cursor = IDC_ARROW;
		}

		// nonreplace image
		if(m->selectedbutton != last || m->mpos.y<3) { // pos.y < 3 for the hover when fullscreening
			RedrawSurface(m);
		}
	} else {

	}

	if(!(m->mpos.y <= m->toolheight)){
		if(m->selectedbutton >= 0) {
			m->selectedbutton = -1;
			// nonreplace image
			RedrawSurface(m);
		}
	}

	if (!(m->isMenuState && IfInMenu(m->mpos, m))) {
		if(m->menuselected >= 0) {
			// off menu, redraw
			m->menuselected = -1;
			RedrawSurface(m);
		}
	}

	if (!((m->mpos.x > m->width - 36 && m->mpos.x < m->width - 13) && (m->mpos.y > 12 && m->mpos.y < 33))) {
		if(fullscreenhover) {fullscreenhover = false; RedrawSurface(m);}
	}

	if (!((m->mpos.x > (m->dmguide_x) && m->mpos.x <= (m->dmguide_x+m->dmguide_sx)) && (m->mpos.y > (m->dmguide_y    ) && m->mpos.y <= (m->dmguide_y+168)))) {
		if(m->dmguidebutton >= 0) {
			m->dmguidebutton = -1;
			RedrawSurface(m);
		}
	}

}


LPSTR lastcursor=0;
HCURSOR realcursor;
void MouseMove(GlobalParams* m, bool isCalledWhenMouseAcuallyMoved){
	LPSTR cursor = IDC_ARROW;
	HINSTANCE cursorinstance = NULL;

	if (isCalledWhenMouseAcuallyMoved) {
		if (abs(m->wasdX) > 0.01f || abs(m->wasdY) > 0.01f) {
			return;
		}
	}

	// Fix unhandled mouse events
	if((GetAsyncKeyState(VK_LBUTTON) & 0x8000) && (!m->Leftdown))
		MouseDown(m);
	if((GetAsyncKeyState(VK_RBUTTON) & 0x8000) && (!m->Rightdown))
		RightDown(m);
	if((GetAsyncKeyState(VK_MBUTTON) & 0x8000) && (!m->Middledown))
		MiddleDown(m);
	if((!(GetAsyncKeyState(VK_LBUTTON) & 0x8000)) && m->Leftdown)
		MouseUp(m);
	if((!(GetAsyncKeyState(VK_RBUTTON) & 0x8000)) && m->Rightdown)
		RightUp(m);
	if((!(GetAsyncKeyState(VK_MBUTTON) & 0x8000)) && m->Middledown)
		MiddleUp(m);

	// mpos : pos
	UpdateMousePos(m);

	// gmpos : globalpos
	
	bool isInMenu = IfInMenu(m->mpos, m) && m->isMenuState;
	bool isInImage = IsInImage(m->mrawpos, m);

	if (m->drawmode && isInImage && !isInMenu && !m->Middledown) {
		cursorinstance = GetModuleHandle(NULL);
		cursor = MAKEINTRESOURCE(IDC_CURSOR1);
		// annotation circle
		if (!m->drawmousedown) RedrawSurface(m);
	}

	MouseMoveCases(&cursor, &cursorinstance, m);

	if(lastcursor != cursor) {
    	realcursor = LoadCursor(cursorinstance, cursor);
    	lastcursor = cursor;
	}

	if (realcursor != NULL) {
		SetCursor(realcursor);
	}
	ShowCursor(TRUE);
	
}
GlobalParams* m_proc;
INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT umsg, WPARAM wparam, LPARAM lparam) {
	switch (umsg) {
	case WM_INITDIALOG: {
		SetTimer(hwndDlg, 1, 100, NULL);
		HWND i = GetDlgItem(hwndDlg, IDC_PROGRESS1);
		SendMessage(i, (WM_USER + 10), (WPARAM)TRUE, (LPARAM)10);
		return TRUE;
	}
	case WM_TIMER: {
		if (m_proc->ProcessOfMakingUndoStep == 0) {
			EndDialog(hwndDlg, 0);
		}
		return TRUE;
	}
	case WM_CLOSE:
		EndDialog(hwndDlg, 0);
		return TRUE;
	}
	return FALSE;
}

void showMessageWhileProcessing(GlobalParams* m) {
	m_proc = m;
	m->tint = true;
	// nonreplace image
	RedrawSurface(m);
	DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(LoadingHalt), m->hwnd, DialogProc);
	m->tint = false;
	// nonreplace image
	RedrawSurface(m);
}

uint32_t genrand() {
	std::uint32_t min = 0;
	std::uint32_t max = 4294967295;

	// Initialize random number generation
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<std::uint32_t> dis(min, max);

	// Generate a random number
	std::uint32_t random_number = dis(gen);
	return random_number;
}

bool goodbye(GlobalParams* m, uint32_t id) {
	std::string path = m->undofolder + std::to_string(id) + "-" + m->name_primary+".bmp";
	if (!DeleteFile(path.c_str())) {
		MessageBox(m->hwnd, "Error cleaning undo data packages", "ERROR", MB_OK | MB_ICONERROR);
		return 0;
	}
	return 1;
}

uint32_t* hello(GlobalParams* m, uint32_t id) {
	std::string path = m->undofolder + std::to_string(id) + "-" + m->name_primary+".bmp";
	int x, y, c;
	uint32_t* data = (uint32_t*)stbi_load(path.c_str(), &x, &y, &c, 4);
	return data;
}

bool classUndo = true;
void PushUndo(GlobalParams* m, uint32_t* thisImage, uint32_t* thisOImage) {

	m->ProcessOfMakingUndoStep++;
	
	for (int y = m->undoData.size(); y > m->undoStep; y--) {
		auto back = m->undoData.back();
		int last = back.imageID;
		int lasto = back.imageIDoriginal;
		if (!goodbye(m, last)) { m->ProcessOfMakingUndoStep = 0; return; }
		if (!goodbye(m, lasto)) { m->ProcessOfMakingUndoStep = 0; return; }
		m->undoData.pop_back();
	}

	uint32_t imageID = genrand();
	uint32_t imageIDo = genrand();

	std::string path1 = m->undofolder + std::to_string(imageID) + "-" + m->name_primary+".bmp";
	bool is = stbi_write_bmp(path1.c_str(), m->imgwidth, m->imgheight, 4, thisImage);
	
	std::string path2 = m->undofolder + std::to_string(imageIDo) + "-" + m->name_primary+".bmp";
	bool is2 = stbi_write_bmp(path2.c_str(), m->imgwidth, m->imgheight, 4, thisOImage);

	if (!is) {
		MessageBox(m->hwnd, "Failed to create undo data package", "Oops", MB_OK | MB_ICONERROR);
		return;
	}

	if (!is2) {
		MessageBox(m->hwnd, "Failed to create undo data package (Original Image)", "Oops", MB_OK | MB_ICONERROR);
		return;
	}

	m->undoStep++;
	UndoDataStruct k;
	k.imageID = imageID;
	k.imageIDoriginal = imageIDo;
	k.height = m->imgheight;
	k.width = m->imgwidth;
	m->undoData.push_back(k);

	FreeData(thisImage);
	FreeData(thisOImage);

	classUndo = true;
	m->ProcessOfMakingUndoStep--;
}

void createUndoStep(GlobalParams* m, bool async) {
	if (!async) {
		if (m->ProcessOfMakingUndoStep > 0) {
			TurnOnLoad(m);
			showMessageWhileProcessing(m);
		}
	}

	uint32_t* thisImage = (uint32_t*)vismalloc(m->imgwidth * m->imgheight * 4, "Create Undo Step Image 1");
	uint32_t* thisOImage = (uint32_t*)vismalloc(m->imgwidth * m->imgheight * 4, "Create Undo Step Image 2");

	memcpy(thisImage, (uint32_t*)m->imgdata, m->imgwidth * m->imgheight * 4);
	memcpy(thisOImage, (uint32_t*)m->imgoriginaldata, m->imgwidth * m->imgheight * 4);

	if (async) {
		std::thread t{ PushUndo, m, thisImage, thisOImage };
		t.detach();
	}
	else {
		PushUndo(m, thisImage, thisOImage);
	}

	TurnOffLoad(m);
}



void UndoBus(GlobalParams* m) {
	if (m->drawmousedown) {
		return;
	}

	if (classUndo) { createUndoStep(m, false); m->undoStep--; }
	
	if (m->ProcessOfMakingUndoStep > 0) {
		TurnOnLoad(m);
		showMessageWhileProcessing(m);
		TurnOffLoad(m);
	}
	
    if (m->undoStep > 0) {
		TurnOnLoad(m);
        m->undoStep--;
        UndoDataStruct selection = m->undoData[m->undoStep];
		FreeData(m->imgdata);
		FreeData(m->imgoriginaldata);

		m->imgdata = vismalloc(selection.width * selection.height * 4, "Replace Image for Undo");
		m->imgoriginaldata = vismalloc(selection.width * selection.height * 4, "Replace Original Image for Undo");

		m->imgwidth = selection.width;
		m->imgheight = selection.height;
		uint32_t* d1 = hello(m, selection.imageID);
		uint32_t* d2 = hello(m, selection.imageIDoriginal);
		if (!d1) {
			MessageBox(m->hwnd, "Annotated undo data package failed to load", "Oops", MB_OK | MB_ICONERROR);
			TurnOffLoad(m);
			return;
		}
		if (!d2) {
			MessageBox(m->hwnd, "RAW undo data package failed to load", "Oops", MB_OK | MB_ICONERROR);
			TurnOffLoad(m);
			return;
		}

        memcpy(m->imgdata, d1, selection.width * selection.height * 4);
		memcpy(m->imgoriginaldata, d2, selection.width * selection.height * 4);
		FreeData(d1);
		FreeData(d2);
		TurnOffLoad(m);
    }
	classUndo = false;
}

void RedoBus(GlobalParams* m) {
	if (m->drawmousedown) {
		return;
	}
	if (m->ProcessOfMakingUndoStep > 0) {
		TurnOnLoad(m);
		showMessageWhileProcessing(m);
		TurnOffLoad(m);
	}

	int s = m->undoStep;
	int step = m->undoData.size() - 1;
	if (s < step) {
		TurnOnLoad(m);
		m->undoStep++;
		UndoDataStruct selection = m->undoData[m->undoStep];
		FreeData(m->imgdata);
		FreeData(m->imgoriginaldata);
		m->imgdata = vismalloc(selection.width * selection.height * 4, "Replace Image for Redo");
		m->imgoriginaldata = vismalloc(selection.width * selection.height * 4, "Replace Original Image for Redo");
		m->imgwidth = selection.width;
		m->imgheight = selection.height;
		uint32_t* d1 = hello(m, selection.imageID);
		uint32_t* d2 = hello(m, selection.imageIDoriginal);
		if (!d1) {
			MessageBox(m->hwnd, "Annotated undo data package failed to load", "Oops", MB_OK | MB_ICONERROR);
			TurnOffLoad(m);
			return;
		}
		if (!d2) {
			MessageBox(m->hwnd, "RAW undo data package failed to load", "Oops", MB_OK | MB_ICONERROR);
			TurnOffLoad(m);
			return;
		}
		memcpy(m->imgdata, d1, selection.width * selection.height * 4);
		memcpy(m->imgoriginaldata, d2, selection.width * selection.height * 4);
		FreeData(d1);
		FreeData(d2);
		TurnOffLoad(m);
	}
	classUndo = false;
}

void ToggleFullscreen(GlobalParams* m) { 
	if (m->fullscreen) {
		// disable fullscreen
		SetWindowLong(m->hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
		m->fullscreen = false; // to fix fullscreen "testing" issue with toolheight
		SetWindowPlacement(m->hwnd, &m->wpPrev);
		SetWindowPos(m->hwnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
		// nonreplace image
		RedrawSurface(m);
	}
	else {
		m->fullscreen = true;
		int screenX = GetSystemMetrics(SM_CXSCREEN);
		int screenY = GetSystemMetrics(SM_CYSCREEN);
		GetWindowPlacement(m->hwnd, &m->wpPrev);
		SetWindowLong(m->hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
		SetWindowPos(m->hwnd, 0, 0, 0, screenX, screenY, 0);
		// nonreplace image
		RedrawSurface(m);
	}
}

void zoomcycle(GlobalParams* m, float factor) {
	for (int i = 0; i < 10; i++) { // Use the "for" loop to recursively make more perfect to reduce rounding errors
		float factor_s = factor / m->mscaler;
		NewZoom(m, factor_s, true, false);
	}
}


void KeyDown(GlobalParams* m, WPARAM wparam, LPARAM lparam) {


	if (m->halt) { return; }

	HWND temp = GetActiveWindow();
	if (temp != m->hwnd) {
		return;
	}

	if (m->fullscreen) {
		ShowCursor(false);
	}

	if ((GetKeyState(VK_ESCAPE) & 0x8000)) {
		m->eyedroppermode = false;
		m->isInCropMode = false;
		// nonreplace image
		RedrawSurface(m);
	}

	if (((GetKeyState(VK_SHIFT) & 0x8000) && wparam == 'Z')&& m->drawmode) { // Eyedropper
		m->eyedroppermode = true;
		// nonreplace image
		RedrawSurface(m);
	}

	if (((GetKeyState(VK_CONTROL) & 0x8000) && (GetKeyState(VK_SHIFT) & 0x8000)) && wparam == 'D') {
		m->debugmode = !m->debugmode;
		return;
	}


	// FOR KEYBOARD NAVIGATION
	if (((GetKeyState(VK_CONTROL) & 0x8000) && (GetKeyState(VK_MENU) & 0x8000)) && (m->imgwidth >= 1)) { // ALSO FOUND IN REDRAW SURFACE TO DRAW TXT
		// nonreplace image
		RedrawSurface(m);

		if (wparam >= '1' && wparam <= '9'){
			PerformCasedBasedOperation(m, wparam - 0x31);
		}
		if (wparam == '0') {
			PerformCasedBasedOperation(m, 9);
		}
		if (wparam == VK_OEM_MINUS) {
			PerformCasedBasedOperation(m, 10);
		}
		return;
	}



	if (wparam == VK_F11) {
		ToggleFullscreen(m);
	}
	if (wparam == VK_ESCAPE) {
		if(m->isMenuState) {
			m->isMenuState = false;
		}
		if (m->fullscreen) {
			// disable fullscreen
			SetWindowLong(m->hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);

			SetWindowPlacement(m->hwnd, &m->wpPrev);
			SetWindowPos(m->hwnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
			m->fullscreen = false;
			// nonreplace image
			RedrawSurface(m);
		}
	}


	if (wparam == 'F' && !m->isInCropMode) {
		PrepareOpenImage(m);
	}

	if (wparam == 'V' && GetKeyState(VK_CONTROL) & 0x8000) {
		
		if (!PasteImageFromClipboard(m)) {
			MessageBox(m->hwnd, "Error pasting the image from the clipboard. You used control+v.", "Bug Detected!", MB_OK | MB_ICONERROR);
		}
		
	}
	

	if (wparam == VK_LEFT) {
		GoLeft(m);
	}
	if (wparam == VK_RIGHT) {
		GoRight(m);
	}


	if (m->imgwidth < 1) {
		return;
	}


	if (wparam == VK_OEM_PLUS) { // WHY? (the plus sign or equals sign)
		NewZoom(m, 1.25f, 2, true);
	}
	if (wparam == VK_OEM_MINUS) { // WHY? (the minus sign or dash sign)
		NewZoom(m, 0.8f, 2, true);

	}
	if (wparam == '1') {
		zoomcycle(m, 1.0f);
	}

	if (wparam == '2') {
		zoomcycle(m, 2.0f);
	}
	if (wparam == '3') {
		zoomcycle(m, 0.5f);
	}

	if (wparam == '4') {
		zoomcycle(m, 4.0f);
	}

	if (wparam == '5') {
		autozoom(m);
	}

	if (wparam == '6') {
		zoomcycle(m, 0.25f);
	}

	if (wparam == '7') {
		zoomcycle(m, 0.125f);
	}

	if (wparam == '8') {
		//m->mscaler = 8.0f;
		zoomcycle(m, 8.0f);
	}


	if (m->isInCropMode) {
		return;
	}

	if (wparam == 'Z' && GetKeyState(VK_CONTROL) & 0x8000) {
		// undo
		UndoBus(m);
	}

	if(wparam == 'C' && GetKeyState(VK_CONTROL) & 0x8000) {
		// copy
		if (!CopyImageToClipboard(m, m->imgdata, m->imgwidth, m->imgheight)) {
			MessageBox(m->hwnd, "Error copying the image to the clipboard. You used control+c.", "Bug Detected!", MB_OK | MB_ICONERROR);
		}
	}

	

	if (wparam == 'S' && GetKeyState(VK_CONTROL) & 0x8000) {
		// save
		PrepareSaveImage(m);
	}

	if (wparam == 'A' && GetKeyState(VK_SHIFT) & 0x8000) {
		// AutoAdjust
		bool did = AutoAdjustLevels(m, (uint32_t*)m->imgdata, 7.0);
	}

	if (wparam == 'R' && GetKeyState(VK_CONTROL) & 0x8000) {
		// resize
		ShowResizeDialog(m);
	}
	if (wparam == 'Y' && GetKeyState(VK_CONTROL) & 0x8000) {
		// undo
		RedoBus(m);
	}
	
	if (wparam == 'G') {
		m->drawmode = !m->drawmode;
		// nonreplace image
		RedrawSurface(m);
	}

	//CheckKeys(m, wparam);

	if (GetAsyncKeyState('W') & 0x8000 || GetAsyncKeyState('A') & 0x8000 || GetAsyncKeyState('S') & 0x8000 || GetAsyncKeyState('D') & 0x8000) {
		// dont do it
	}
	else {
		MouseMove(m);
	}
}

void MouseUp(GlobalParams* m) {
	m->movemousedown = false;
	m->Leftdown = false;
	m->brush_size_slider.md = false;
	m->brush_opacity_slider.md = false;
	m->isMovingTL = false;
	m->isMovingTR = false;
	m->isMovingBL = false;
	m->isMovingBR = false;

	m->isSize = false;
	TurnOffDraw(m);
	MouseMove(m, false);
}

bool aot = false;

void RightDownCases(GlobalParams* m){
	if (m->isInCropMode) {
		return;
	}
	if (m->eyedroppermode) {
		return;
	}

	m->lastK = -1;
	m->lastV = -1;

	// mpos: mPP
	UpdateMousePos(m);

	if (IfInMenu(m->mpos, m) && m->isMenuState) { // check if the mouse is over a menu
		return;
	}

	if (m->drawmode) {
		if (IsInImage(m->mrawpos, m)) {// NOW, im putting it in the right click menu up thing to not rendeeer menu ACTUALLLY its right down below
			// removed the draw type thing because now it is handled in realtime
			TurnOnDraw(m);
			createUndoStep(m, true);
			return;
		}
	}

	if((m->mpos.y <= m->toolheight || !extracases(m->mpos, m))) {
		return;
	}

	if(!(GetAsyncKeyState(VK_LBUTTON) & 0x8000)){
		m->menuVector = {

			{"Blank Image",
				[m]() -> bool {
					m->isMenuState = false;
					if (AllocateBlankImage(m, 0xFFFFFFFF)) {
						ShowResizeDialog(m);
					}
					return true;
				},1,1, &m->item_enabled, true
			},

			{"Toggle Smoothing{s}",
				[m]() -> bool {
					m->isMenuState = false;
					m->smoothing = !m->smoothing;
					// opengl notice: this should no longer be a m-> variable and should be tied to the opengl renderer
					// nonreplace image
					RedrawSurface(m);
					return true;
				},14,1, &m->isimage_menucondition, true
			},

			{"Undo (CTRL+Z)",
				[m]() -> bool {
					m->isMenuState = false;
					UndoBus(m);
					return true;
				},27,1, &m->undo_menucondition, true
			},

			{"Redo (CTRL+Y){s}",
				[m]() -> bool {
					m->isMenuState = false;
					RedoBus(m);
					return true;
				},40,1, &m->redo_menucondition, true
			},

			

			{"Apply annotations",
			[m]() -> bool {

				createUndoStep(m, true);
				TurnOnLoad(m);
				
				memcpy(m->imgoriginaldata, m->imgdata, m->imgwidth*m->imgheight*4);
				Beep(2000, 50);

				m->shouldSaveShutdown = true;

				TurnOffLoad(m);

				return true;
			},92,14, &m->isimage_menucondition, true
			},

			{"Resize Image [CTRL+R]{s}",
				[m]() -> bool {
					m->isMenuState = false;
					// nonreplace image
					RedrawSurface(m);
					ShowResizeDialog(m);
					//ResizeImageToSize(m);
					return true;
				},53,1, &m->isimage_menucondition, true
			},
			
 
			{"Set Always On Top",
				[m]() -> bool {
					if (aot) {
						SetWindowPos(m->hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
						m->menuVector[6].atlasX = 26;
						aot = false;
					// nonreplace image
						RedrawSurface(m);
						Beep(1000, 100);
					}
					else {
						SetWindowPos(m->hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
						m->menuVector[6].atlasX = 39;
						aot = true;
						// nonreplace image
						RedrawSurface(m);
						Beep(2000, 100);
					}
					return true;
				},27,14, &m->item_enabled, false // double tap bug
			},	

		};
		if(aot) {
			m->menuVector[6].atlasX = 39;

		} else {
			m->menuVector[6].atlasX = 26;
		}
		m->menuX = m->mpos.x;
		m->menuY = m->mpos.y;
		m->isMenuState = true;
		// nonreplace image
		RedrawSurface(m);
	}

}


void MiddleDownCases(GlobalParams* m){
	if (m->isInCropMode) {
		return;
	}
	if (m->eyedroppermode) {
		return;
	}

	m->lastK = -1;
	m->lastV = -1;

	m->lockimgoffx = m->iLocX;
	m->lockimgoffy = m->iLocY;
	GetCursorPos(&m->LockmPos);

	m->isMenuState = false;
	
	// nonreplace image
	RedrawSurface(m);
	
	m->movemousedown = true;
}

void RightDown(GlobalParams* m) {
	if(m->Rightdown) return; // both way: mouse move check down or OS check down will come first, first come first serve. do not do twice

	m->Rightdown = true;
	
	RightDownCases(m);

	MouseMove(m, false);
}

void MiddleDown(GlobalParams* m) {
	if(m->Middledown) return; // both way: mouse move check down or OS check down will come first, first come first serve. do not do twice

	m->Middledown = true;
	
	MiddleDownCases(m);

	MouseMove(m, false);
}

void RightUpCases(GlobalParams* m) {
	UpdateMousePos(m);

	if (m->isInCropMode) {
		ConfirmCrop(m);
		return;
	}
	if (m->eyedroppermode) {
		return;
	}
	
	// maybe
	if(!(GetAsyncKeyState(VK_LBUTTON) & 0x8000)){ // added conditional to prevent right/left mouse button slippage
		TurnOffDraw(m);
	}

	bool isdraw = m->drawmousedown;

	// mpos: pos

	if (isdraw) {
		return;
	}
	if (m->mpos.y < m->toolheight) {
		return;
	}
	if (m->drawmode) {
		if (IsInImage(m->mrawpos, m)) {
			return;
		}
	}
}

void RightUp(GlobalParams* m) {
	m->Rightdown = false;
	
	// nonreplace image
	RedrawSurface(m);
	
	RightUpCases(m);

	MouseMove(m, false);
}

void MiddleUp(GlobalParams* m) {
	m->movemousedown = false;
	m->Middledown = false;

	MouseMove(m, false);
}

void MouseWheel(GlobalParams* m, WPARAM wparam, LPARAM lparam) {
	UpdateMousePos(m);

	if (m->imgwidth < 1) return;
	float zDelta = (float)GET_WHEEL_DELTA_WPARAM(wparam) / WHEEL_DELTA;

	// mpos: mPP

	if (m->drawmode) {

		if (IsInSlider(m->brush_size_slider)) {
			if (zDelta > 0) {
				m->drawSize += 1.0f;
				m->drawSize *= 1.3f;
			}
			else {
				m->drawSize -= 1.0f;
				m->drawSize *= 0.76f;
			}
			if (m->drawSize < 1) { m->drawSize = 1; }
			if (m->drawSize > 100.0f) { m->drawSize = 100.0f; }

			// nonreplace image
			RedrawSurface(m);
			return;
		}

		if (IsInSlider(m->brush_opacity_slider)) {
			if (zDelta > 0) {
				m->a_opacity += 0.1f;
			}
			else {
				m->a_opacity -= 0.1f;
			}
			if (m->a_opacity < 0.01f) { m->a_opacity = 0.01f; }
			if (m->a_opacity > 1.0f) { m->a_opacity = 1.0f; }

			// nonreplace image
			RedrawSurface(m);
			return;
		}
	}
	
	// CONTROL F!



	float v = 1;

	if (zDelta > 0 && m->mscaler < 400.0f) {
		v = 1.25f;
	}
	else if (m->mscaler > 0.01f) {
		v = 0.8f;
	}

	if ((GetKeyState(VK_MENU) & 0x8000)&&m->drawmode) {// why do they call the alt key VK_MENU
		m->drawSize *= v;
		// nonreplace image
		RedrawSurface(m);
	}
	else {
		NewZoom(m, v, true, true);
	}

	// nonreplace image
	RedrawSurface(m);
}