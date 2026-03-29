#include <filesystem>
#include <stdint.h>
#include <stdlib.h>
#include <shlwapi.h>
#include <string.h>
#include "headers/ops.hpp"
#include "headers/imgload.hpp"
#include "../res/resource.h"
#include "vendor/stb_image_resize2.h"
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))



uint8_t ivv = 0;


typedef HRESULT(WINAPI* DwmSetWindowAttribute_t)(HWND, DWORD, LPCVOID, DWORD);
bool DwmDarken(HWND hwnd) {
	BOOL enable = TRUE;

	HMODULE hDwmApi = LoadLibrary(TEXT("dwmapi.dll"));
	if (hDwmApi == nullptr) {
		return false;
	}

	DwmSetWindowAttribute_t pDwmSetWindowAttribute =
		(DwmSetWindowAttribute_t)GetProcAddress(hDwmApi, "DwmSetWindowAttribute");

	if (pDwmSetWindowAttribute == nullptr) {
		FreeLibrary(hDwmApi);
		return false;
	}

	HRESULT hr = pDwmSetWindowAttribute(hwnd, 20, &enable, sizeof(enable));
	if (FAILED(hr)) {
		std::cerr << "DwmSetWindowAttribute failed with HRESULT: " << hr << std::endl;
	}

	FreeLibrary(hDwmApi);
	return true;
}

typedef struct _MARGINS {
    int cxLeftWidth;
    int cxRightWidth;
    int cyTopHeight;
    int cyBottomHeight;
} MARGINS, *PMARGINS;

typedef HRESULT(WINAPI* DwmExtendFrameIntoClientArea_t)(HWND, const MARGINS*);
bool DwmExtend(HWND hwnd, int topHeight) {
    HMODULE hDwmApi = LoadLibrary(TEXT("dwmapi.dll"));
    if (hDwmApi == nullptr) {
        return false;
    }

    DwmExtendFrameIntoClientArea_t pDwmExtendFrameIntoClientArea = (DwmExtendFrameIntoClientArea_t)GetProcAddress(hDwmApi, "DwmExtendFrameIntoClientArea");

    if (pDwmExtendFrameIntoClientArea == nullptr) {
        FreeLibrary(hDwmApi);
        return false;
    }

    MARGINS margins = { 0, 0, topHeight, 0 };

    HRESULT hr = pDwmExtendFrameIntoClientArea(hwnd, &margins);
    
    FreeLibrary(hDwmApi);
    return SUCCEEDED(hr);
}



bool DeleteDirectory(const char* directoryPath) {
	return std::filesystem::remove_all(directoryPath);
}

void DeleteTempFiles(GlobalParams* m, std::string folder) {
	if (!DeleteDirectory(folder.c_str())) {
		DWORD error = GetLastError();
		std::string s = "Failed to remove temporary files: ERR CODE: " + std::to_string(error);
		std::string str = "Failed to remove the temporary files for " + m->name_full + ". You can manually remove them at AppData\\Local\\Temp\\"+m->name_full;
		MessageBox(m->hwnd, str.c_str(), s.c_str(), MB_OK | MB_ICONERROR);
	}
}

bool isFile(const char* str, const char* suffix) {
	size_t len_str = strlen(str);
	size_t len_suffix = strlen(suffix);
	if (len_str < len_suffix) {
		return false;
	}
	for (size_t i = len_suffix; i > 0; i--) {
		if (tolower(str[len_str - i]) != tolower(suffix[len_suffix - i])) {
			return false;
		}
	}
	return true;
}

unsigned char* LoadImageFromResource(int resourceId, int& width, int& height, int& channels)
{
	HMODULE hModule = GetModuleHandle(nullptr);
	if (!hModule)
	{
		std::cerr << "Failed to get module handle" << std::endl;
		return nullptr;
	}

	HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(resourceId), "PNG");
	if (!hResource)
	{
		std::cerr << "Failed to find resource " << resourceId << std::endl;
		return nullptr;
	}

	HGLOBAL hResourceData = LoadResource(hModule, hResource);
	if (!hResourceData)
	{
		std::cerr << "Failed to load resource data" << std::endl;
		return nullptr;
	}

	const void* resourceData = LockResource(hResourceData);
	if (!resourceData)
	{
		std::cerr << "Failed to lock resource data" << std::endl;
		FreeResource(hResourceData);
		return nullptr;
	}

	const size_t resourceSize = SizeofResource(hModule, hResource);

	unsigned char* imageData = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(resourceData), resourceSize, &width, &height, &channels, 4);

	if (!imageData)
	{
		std::cerr << "Failed to load image from resource " << resourceId << ": " << stbi_failure_reason() << std::endl;
	}

	FreeResource(hResourceData);

	return imageData;
}

void PrintImageToPrinter(uint32_t* image, int width, int height, HDC printerDC) {

	uint32_t* temp = (uint32_t*)malloc(width * height * 4);
	for (int i = 0; i < width * height; i++) {
		*(temp+i) = (*(image+i));
	}

	if (!image) {

		return;
	}

	BITMAPINFO bmpInfo = {};
	bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpInfo.bmiHeader.biWidth = width;
	bmpInfo.bmiHeader.biHeight = -height; // Negative height for top-down DIB
	bmpInfo.bmiHeader.biPlanes = 1;
	bmpInfo.bmiHeader.biBitCount = 4 * 8; // Number of bits per pixel
	bmpInfo.bmiHeader.biCompression = BI_RGB;

	DOCINFO docInfo = {};
	docInfo.cbSize = sizeof(DOCINFO);
	docInfo.lpszDocName = TEXT("Image Print");
	StartDoc(printerDC, &docInfo);
	StartPage(printerDC);

	int printerWidth = GetDeviceCaps(printerDC, HORZRES);
	int printerHeight = GetDeviceCaps(printerDC, VERTRES);

	float scaleX = static_cast<float>(printerWidth) / width;
	float scaleY = static_cast<float>(printerHeight) / height;

	int destWidth = static_cast<int>(width * scaleX);
	int destHeight = static_cast<int>(height * scaleY);

	StretchDIBits(printerDC, 0, 0,destWidth, destHeight, 0, 0, width, height, temp, &bmpInfo, DIB_RGB_COLORS, SRCCOPY);

	EndPage(printerDC);
	EndDoc(printerDC);

	FreeData(temp);

}

void Print(GlobalParams* m) {

	PRINTDLG pd = {};
	pd.lStructSize = sizeof(PRINTDLG);
	pd.Flags = PD_RETURNDC | PD_ALLPAGES;

	PrintDlg(&pd);

	HDC printerDC = pd.hDC;

	PrintImageToPrinter((uint32_t*)m->imgdata, m->imgwidth, m->imgheight, printerDC);
}

void ConfirmCropBuffer(GlobalParams* m, void** buffer, int newW, int newH) {

	void* MyNewCropLifestyle = malloc(newW * newH * 4);

	for (int y = 0; y < newH; y++) {
		for (int x = 0; x < newW; x++) {
			uint32_t offsetX = m->leftP * (float)m->imgwidth;
			uint32_t offsetY = m->topP * (float)m->imgheight;
			*GetMemoryLocation(MyNewCropLifestyle, x, y, newW, newH) = *GetMemoryLocation(*buffer, x + offsetX, y + offsetY, m->imgwidth, m->imgheight);
		}
	}

	FreeData(*buffer);
	*buffer = malloc(newW * newH * 4);
	memcpy(*buffer, MyNewCropLifestyle, newW * newH * 4);

	FreeData(MyNewCropLifestyle);
}

void ConfirmCrop(GlobalParams* m) {

	float difPerX = m->rightP - m->leftP;
	float difPerY = m->bottomP - m->topP;

	int newW = difPerX * (float)m->imgwidth;
	int newH = difPerY * (float)m->imgheight;

	if (newW < 1 || newH < 1) {
		MessageBox(m->hwnd, "Your width or height is too small", "Crop Invalid", MB_OK | MB_ICONERROR);
		return;
	}

	createUndoStep(m, false);

	ConfirmCropBuffer(m, &m->imgdata, newW, newH);
	ConfirmCropBuffer(m, &m->imgoriginaldata, newW, newH);

	m->imgwidth = newW;
	m->imgheight = newH;
	
	m->shouldSaveShutdown = true;
	
	autozoom(m);
	m->isInCropMode = false;
	RedrawSurface(m);
}


void performResize(GlobalParams* m, void** memory, int owidth, int oheight, int nwidth, int nheight) {
	void* tempOldBuffer = malloc(owidth * oheight* 4);
	memcpy(tempOldBuffer, *memory, owidth * oheight * 4);

	FreeData(*memory);
	*memory = malloc(nwidth * nheight * 4);

	stbir_resize_uint8_linear((unsigned char*)tempOldBuffer, owidth, oheight, 0, (unsigned char*)*memory, nwidth, nheight, 0, STBIR_RGBA);

	m->shouldSaveShutdown = true;

	FreeData(tempOldBuffer);
}

void ResizeImageToSize(GlobalParams* m, int nwidth, int nheight) {

	createUndoStep(m, false);

	performResize(m, &m->imgdata, m->imgwidth, m->imgheight, nwidth, nheight);
	performResize(m, &m->imgoriginaldata, m->imgwidth, m->imgheight, nwidth, nheight);
	m->imgwidth = nwidth;
	m->imgheight = nheight;
	autozoom(m);
	RedrawSurface(m);
}

void rotatememory(GlobalParams* m, int owidth, int oheight, int nwidth, int nheight, void** memory) {
	void* tempOldBuffer = malloc(owidth * oheight * 4);
	memcpy(tempOldBuffer, *memory, owidth * oheight * 4);

	FreeData(*memory);
	*memory = malloc(nwidth * nheight * 4);

	// do
	for (size_t y = 0; y < nheight; y++) {
		for (size_t x = 0; x < nwidth; x++) {
			*GetMemoryLocation(*memory, x, y, nwidth, nheight) = *GetMemoryLocation(tempOldBuffer, y, nwidth - 1 - x, owidth, oheight);
		}
	}

	FreeData(tempOldBuffer);
}

void rotateImage90Degrees(GlobalParams* m) {

	createUndoStep(m, false);
	int oldw = m->imgwidth;
	int oldh = m->imgheight;
	int neww = m->imgheight;
	int newh = m->imgwidth;
	rotatememory(m, oldw,oldh,neww ,newh, &m->imgdata);
	rotatememory(m, m->imgwidth, m->imgheight, m->imgheight, m->imgwidth, &m->imgoriginaldata);
	m->imgwidth = neww;
	m->imgheight = newh;
	m->shouldSaveShutdown = true;
	autozoom(m);
	RedrawSurface(m);
}

int GetIndividualButtonPush(GlobalParams* m, int index) {
	return (m->iconSize + 4) + (m->toolbartable[index].isSeperator * 5);
}

int GetLocationFromButton(GlobalParams* m, int index) {
	// GetLocationFromButton, RenderToolbarButtons, getXbuttonID
	// Make sure they are all synced
	int p = m->starttoolbarloc;
	for (size_t i = 0; i < m->toolbartable.size(); i++) {
		if (i == index) {
			return p;
		}
		if (m->imgwidth < 1) {
			return p;
		}
		if (m->drawmode && i == 11) {
			return p;
		}
		p += GetIndividualButtonPush(m, i);
	}
	return p;
}


int getXbuttonID(GlobalParams* m, POINT mPos) {
	// GetLocationFromButton, RenderToolbarButtons, getXbuttonID
	// Make sure they are all synced

	if (mPos.y > m->toolheight || mPos.y < 2 || mPos.x < 2) {
		return -1;
	}

	int p = m->starttoolbarloc;

	for (size_t i = 0; i < m->toolbartable.size(); i++) {

		if (mPos.x >= p) {
			if (mPos.x <= (p+m->iconSize+4)) {
				return i;
			}
		}
		if (m->imgwidth < 1) {
			return -1;
		}
		if (m->drawmode && i == 11) {
			return -1;
		}
		p += GetIndividualButtonPush(m, i);
	}
	return -1;
}

void GetCropCoordinates(GlobalParams* m, uint32_t* outDistLeft, uint32_t* outDistRight, uint32_t* outDistTop, uint32_t* outDistBottom) {

	float realWidth = (float)(m->CoordRight - m->CoordLeft);
	float realHeight = (float)(m->CoordBottom - m->CoordTop);

	*outDistLeft = m->CoordLeft + (m->leftP * realWidth);
	*outDistTop = m->CoordTop + (m->topP * realHeight);
	*outDistRight = m->CoordLeft + (m->rightP * realWidth);
	*outDistBottom = m->CoordTop + (m->bottomP * realHeight);
}

void GetCropPercentagesFromCursor(GlobalParams* m, int cursorX, int cursorY, float* outX, float* outY) {

	float realWidth = (float)(m->CoordRight - m->CoordLeft);
	float realHeight = (float)(m->CoordBottom - m->CoordTop);
	
	*outX = (float)(cursorX - m->CoordLeft)/realWidth;
	*outY = (float)(cursorY - m->CoordTop) / realHeight;

	// clamp values
	if (*outX < 0.0f) { *outX = 0.0f; } if (*outX > 1.0f) { *outX = 1.0f; }
	if (*outY < 0.0f) { *outY = 0.0f; } if (*outY > 1.0f) { *outY = 1.0f; }
}


float log_base_1_25(float x) {
	float log_1_25 = log(1.25);
	return log(x) / log_1_25;
}

float roundzoom(float z) {
	return pow(1.25f, round(log_base_1_25(z)));
}

void no_offset(GlobalParams* m) {
	m->iLocX = 0;
	if (!m->fullscreen) {
		m->iLocY = m->toolheight / 2;
	}
	else {
		m->iLocY = 0;
	}

}

void autozoom(GlobalParams* m) {
	if(!m->imgdata) {
		return;
	}

	int toolheight0 = 0;
	if (!m->fullscreen) { toolheight0 = m->toolheight; }

	no_offset(m);

	float precentX = (float)m->width / (float)m->imgwidth;
	float precentY = (float)(m->height - toolheight0) / (float)m->imgheight;

	float e = fmin(precentX, precentY);
	
	float fzoom = e;

	if (m->imgheight < 50) { fzoom = e / 2; }
	// round to the nearest power of 1.25 (for easy zooming back to 100)
	m->mscaler = fzoom;
	//if (mscaler > 1.0f && imgheight > 5) mscaler = 1.0f;

}


void NewZoom(GlobalParams* m, float v, int mouse, bool shouldRoundZoom) {

	POINT p;
	GetCursorPos(&p);
	ScreenToClient(m->hwnd, &p);
	p.x -= m->width / 2;
	p.y -= m->height / 2;

	if (mouse == 2) {
		p.x = 0;
		p.y = 0;
	}

	int distance_x = m->iLocX - p.x;
	int distance_y = m->iLocY - p.y;
	int new_width = m->width * v;
	int new_height = m->height * v;
	if (mouse) {
		m->iLocX = p.x + distance_x * v;
		m->iLocY = p.y + distance_y * v;
	}
	m->mscaler *= v;
	if (shouldRoundZoom) {
		m->mscaler = roundzoom(m->mscaler);
	}

	RedrawSurface(m);
}


uint32_t InvertCC(uint32_t d, bool should) {
	if (should) {
		return (d & 0xFF00FF00) | ((d & 0x00FF0000) >> 16) | ((d & 0x000000FF) << 16);
	}
	else {
		return d;
	}
}

void InvertAllColorChannels(uint32_t* buffer, int w, int h) {
	
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			uint32_t* mem = GetMemoryLocation(buffer, x, y, w, h);
			*mem = InvertCC(*mem, true);
		}
	}
}

uint32_t lerp_u32(uint32_t c1, uint32_t c2, uint32_t a)
{
    if (c1 == c2 || a == 0)
        return c1;
    if (a == 255)
        return c2;

    uint32_t rb1 = c1 & 0x00FF00FF;
    uint32_t ag1 = (c1 >> 8) & 0x00FF00FF;
    uint32_t rb2 = c2 & 0x00FF00FF;
    uint32_t ag2 = (c2 >> 8) & 0x00FF00FF;

    rb1 += ((rb2 - rb1) * a) >> 8;
    ag1 += ((ag2 - ag1) * a) >> 8;

    return (rb1 & 0x00FF00FF) | ((ag1 & 0x00FF00FF) << 8);
}

const int TABLE_SIZE = 256;

float gamma_table[TABLE_SIZE];

void init_gamma_table(float gamma) {
	for (int i = 0; i < TABLE_SIZE; ++i) {
		gamma_table[i] = (i / 255.0f)*(i / 255.0f);
	}
}

bool AutoAdjustLevels(GlobalParams* m, uint32_t* buffer, double sigma) {
	// temporary blurred image buffer
	uint32_t* tempd = (uint32_t*)malloc(m->imgwidth*m->imgheight*4);

	// gaussian B previously
	gaussian_blur_real((uint32_t*)buffer, tempd, m->imgwidth, m->imgheight, sigma, m->imgwidth, m->imgheight, 0, 0);

	m->isMenuState = false;	
	int width = m->imgwidth;
	int height = m->imgheight;

	// examination
	int minR = 255;
	int minG = 255;
	int minB = 255;

	int maxR = 0;
	int maxG = 0;
	int maxB = 0;

	int minRo = 255;
	int minGo = 255;
	int minBo = 255;

	int maxRo = 0;
	int maxGo = 0;
	int maxBo = 0;

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			uint32_t pxlColor = *GetMemoryLocation(tempd, x, y, width, height);
			uint8_t a = (pxlColor >> 24) & 0xFF;
			uint8_t r = (pxlColor >> 16) & 0xFF;
			uint8_t g = (pxlColor >> 8) & 0xFF;
			uint8_t b = (pxlColor) & 0xFF;
			if (a == 255) {
				if (minR > r) { minR = r; }
				if (minG > g) { minG = g; }
				if (minB > b) { minB = b; }

				if (maxR < r) { maxR = r; }
				if (maxG < g) { maxG = g; }
				if (maxB < b) { maxB = b; }
			}
		}
	}

	free(tempd);

	// fix division by zero
	if (maxR == minR) { if (maxR < 255) { maxR++; } else { minR--; } }
	if (maxG == minG) { if (maxG < 255) { maxG++; } else { minG--; } }
	if (maxB == minB) { if (maxB < 255) { maxB++; } else { minB--; } }

	if (minR == 0 && minG == 0 && minB == 0 && maxR == 255 && maxG == 255 && maxB == 255) {
		MessageBox(m->hwnd, "There is no adjustment needed", "Automatic Adjust", MB_OK);
		return false;
	}

	// modify
	m->shouldSaveShutdown = true;
	createUndoStep(m, true);

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			uint32_t pxlColor = *GetMemoryLocation(m->imgdata, x, y, width, height);

			int a = (pxlColor >> 24) & 0xFF;
			int r = (pxlColor >> 16) & 0xFF;
			int g = (pxlColor >> 8) & 0xFF;
			int b = (pxlColor) & 0xFF;

			int newR = (std::clamp(r - minR, 0, 255) * (255) / (maxR - minR));
			int newG = (std::clamp(g - minG, 0, 255) * (255) / (maxG - minG));
			int newB = (std::clamp(b - minB, 0, 255) * (255) / (maxB - minB));

			uint8_t nR = std::clamp(newR, 0, 255);
			uint8_t nG = std::clamp(newG, 0, 255);
			uint8_t nB = std::clamp(newB, 0, 255);

			*GetMemoryLocation(m->imgdata, x, y, width, height) = change_alpha(RGB(nB, nG, nR), a);
		}
	}

	Beep(4000, 40);

	RedrawSurface(m);
	return true;
}



uint32_t multiplyColors(uint32_t color1, uint32_t color2) {
    uint8_t a1 = (color1 >> 24) & 0xFF;
    uint8_t r1 = (color1 >> 16) & 0xFF;
    uint8_t g1 = (color1 >> 8) & 0xFF;
    uint8_t b1 = color1 & 0xFF;

    uint8_t a2 = (color2 >> 24) & 0xFF;
    uint8_t r2 = (color2 >> 16) & 0xFF;
    uint8_t g2 = (color2 >> 8) & 0xFF;
    uint8_t b2 = color2 & 0xFF;

    uint8_t aOut = (uint8_t)((a1 * a2 + 127) / 255);
    uint8_t rOut = (uint8_t)((r1 * r2 + 127) / 255);
    uint8_t gOut = (uint8_t)((g1 * g2 + 127) / 255);
    uint8_t bOut = (uint8_t)((b1 * b2 + 127) / 255);

    return (aOut << 24) | (rOut << 16) | (gOut << 8) | bOut;
}


bool yes = 0;
uint32_t lerp_gc(uint32_t color1, uint32_t color2, float alpha) {


	float gamma = 2.2f;
	if (!yes) {
		init_gamma_table(gamma);
		yes = 1;
	}

	float a1 = gamma_table[(color1 >> 24) & 0xFF];
    float r1 = gamma_table[(color1 >> 16) & 0xFF];
    float g1 = gamma_table[(color1 >> 8) & 0xFF];
    float b1 = gamma_table[color1 & 0xFF];

    float a2 = gamma_table[(color2 >> 24) & 0xFF];
    float r2 = gamma_table[(color2 >> 16) & 0xFF];
    float g2 = gamma_table[(color2 >> 8) & 0xFF];
    float b2 = gamma_table[color2 & 0xFF];

    float a = (1 - alpha) * a1 + alpha * a2;
    float r = (1 - alpha) * r1 + alpha * r2;
    float g = (1 - alpha) * g1 + alpha * g2;
    float b = (1 - alpha) * b1 + alpha * b2;

    a = sqrt(a);
    r = sqrt(r);
    g = sqrt(g);
    b = sqrt(b);

    return (static_cast<uint32_t>(a * 255) << 24) | (static_cast<uint32_t>(r * 255) << 16) | (static_cast<uint32_t>(g * 255) << 8) | static_cast<uint32_t>(b * 255);
}



// Gaussian blur function
void gaussian_blur_real(uint32_t* input_buffer, uint32_t* output_buffer, int lW, int lH, double sigma, uint32_t width, uint32_t height, uint32_t offX, uint32_t offY) {
    int kernel_size = (int)ceil(sigma * 3.0) * 2 + 1;
    double* kernel = (double*)malloc(kernel_size * sizeof(double));
    double sum = 0.0;
    
    for (int i = 0; i < kernel_size; i++) {
        double x = (double)i - (double)(kernel_size - 1) / 2.0;
        kernel[i] = exp(-0.5 * (x * x) / (sigma * sigma));
        sum += kernel[i];
    }
    for (int i = 0; i < kernel_size; i++) kernel[i] /= sum;

    uint32_t* temp_buffer = (uint32_t*)malloc(lW * lH * sizeof(uint32_t));
    int half_k = (kernel_size - 1) / 2;

    for (int y = 0; y < lH; y++) {
        for (int x = 0; x < lW; x++) {
			if((y+offY) >= height || (x+offX) >= width || (x+offX) < 0 || (y+offY) < 0) continue;
            double r = 0, g = 0, b = 0, a = 0;
            for (int k = 0; k < kernel_size; k++) {
                int xk = x - half_k + k;
                if (xk < 0) xk = 0;
                if (xk >= lW) xk = lW - 1;

                uint32_t p = input_buffer[(y + offY) * width + (xk + offX)];
                a += (double)((p >> 24) & 0xFF) * kernel[k];
                r += (double)((p >> 16) & 0xFF) * kernel[k];
                g += (double)((p >> 8) & 0xFF) * kernel[k];
                b += (double)(p & 0xFF) * kernel[k];
            }
            temp_buffer[y * lW + x] = ((uint32_t)(a + 0.5) << 24) | ((uint32_t)(r + 0.5) << 16) | ((uint32_t)(g + 0.5) << 8) | (uint32_t)(b + 0.5);
        }
    }

    for (int x = 0; x < lW; x++) {
        for (int y = 0; y < lH; y++) {
			if((y+offY) >= height || (x+offX) >= width || (x+offX) < 0 || (y+offY) < 0) continue;
            double r = 0, g = 0, b = 0, a = 0;
            for (int k = 0; k < kernel_size; k++) {
                int yk = y - half_k + k;
                if (yk < 0) yk = 0;
                if (yk >= lH) yk = lH - 1;

                uint32_t p = temp_buffer[yk * lW + x];
                a += (double)((p >> 24) & 0xFF) * kernel[k];
                r += (double)((p >> 16) & 0xFF) * kernel[k];
                g += (double)((p >> 8) & 0xFF) * kernel[k];
                b += (double)(p & 0xFF) * kernel[k];
            }
            output_buffer[(y + offY) * width + (x + offX)] = ((uint32_t)(a + 0.5) << 24) | ((uint32_t)(r + 0.5) << 16) | ((uint32_t)(g + 0.5) << 8) | (uint32_t)(b + 0.5);
        }
    }

    free(kernel);
    free(temp_buffer);
}

uint32_t change_alpha(uint32_t color, uint8_t new_alpha) {
	return (color & 0xFFFFFF) | (static_cast<uint32_t>(new_alpha) << 24);
}


// Freetype Globals
FT_Library ft;
FT_Face* currentFace;

std::string GetWindowsFontsFolder() {
	HKEY hKey;
	char fontPath[MAX_PATH];
	DWORD size = MAX_PATH;

	// Open the registry key where the fonts directory path is stored
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
		// Query the "FontDir" value
		if (RegQueryValueEx(hKey, "FontDir", nullptr, nullptr, (LPBYTE)fontPath, &size) == ERROR_SUCCESS) {
			RegCloseKey(hKey);
			return std::string(fontPath);
		}
		RegCloseKey(hKey);
	}

	// Fallback: Use the standard Windows directory + "Fonts" if registry lookup fails
	if (GetWindowsDirectory(fontPath, MAX_PATH)) {
		PathAppend(fontPath, "Fonts");
		return std::string(fontPath);
	}

	// If everything fails, return an empty string
	return "Fail";
}

void initfontfolder(GlobalParams* m) {
	m->fontsfolder = GetWindowsFontsFolder();
	if (m->fontsfolder == "Fail") {
		MessageBox(m->hwnd, "Fonts directory not found\n", "Error", MB_OK | MB_ICONERROR);
	}
}

std::string ExePath() {
    char buffer[MAX_PATH] = { 0 };
    GetModuleFileName(nullptr, buffer, MAX_PATH );
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");
    return std::string(buffer).substr(0, pos);
}

bool alreadyInit = false;
FT_Face LoadFont(GlobalParams* m, std::string fontA) {
	if (!alreadyInit) {
		if (FT_Init_FreeType(&ft)) {
			MessageBox(m->hwnd, "Failed to initialize FreeType library\n", "Error", MB_OK | MB_ICONERROR);
			return nullptr;
		}
		FT_Library_SetLcdFilter(ft, FT_LCD_FILTER_MAX);
		alreadyInit = true;
	}
	
	if (m->fontsfolder == "") {
		initfontfolder(m);
	}
	std::string font_s = (m->fontsfolder + "\\" + fontA);
	FT_Face k;
	if (!(FT_New_Face(ft, font_s.c_str(), 0, &k))) {
		return k;
	}

	// try again
	std::string cdir = ExePath();
	font_s = (cdir + "\\" + fontA);
	if (!(FT_New_Face(ft, font_s.c_str(), 0, &k))) {
		return k;
	}

	std::string er = "Failed to load font: " + fontA + "\nYou may need to install this on your system";
	MessageBox(m->hwnd, er.c_str(), "Error Loading Font", MB_OK | MB_ICONERROR);
	return nullptr;
}


void SwitchFont(FT_Face& font) {
	currentFace = &font;
}

int opsPlaceStringGreyscale(GlobalParams* m, int size, const char* inputstr, uint32_t locX, uint32_t locY, uint32_t color, void* mem, int bufwidth, int bufheight, void* fromBuffer) {
	
	locY = bufheight - locY-size; // compensate for negative Y

	if (!*currentFace) {
		return 0;
	}

	FT_Set_Pixel_Sizes(*currentFace, 0, size);

    const FT_GlyphSlot glyph = (*currentFace)->glyph;
	
	for (const char* p = inputstr; *p; p++) {
		
		if (FT_Load_Char((*currentFace), *p, FT_LOAD_RENDER) != 0)
			continue;
		
		// width of char: glyph->bitmap.width
		// height of char: glyph->bitmap.rows

        const float vx = locX + glyph->bitmap_left * 1;
        const float vy = locY + glyph->bitmap_top * 1;

		// render
		for (int y = 0; y < glyph->bitmap.rows; ++y) {
			for (int x = 0; x < glyph->bitmap.width; ++x) {
				unsigned char pixelValue = glyph->bitmap.buffer[y * glyph->bitmap.pitch + x];
				
				uint32_t ptx = vx + x;
				uint32_t pty = vy-y;

				pty = bufheight - pty; // compensate for negative Y

				uint32_t* memoryPathFrom = GetMemoryLocation(fromBuffer, ptx, pty, bufwidth, bufheight);
				uint32_t existingColor = *memoryPathFrom;
				if (ptx >= 0 && pty >= 0 && ptx < bufwidth && pty < bufheight) {
					*GetMemoryLocation(mem, ptx, pty, bufwidth, bufheight) = lerp_u32(existingColor, color, pixelValue);
				}
			}
		}

        locX += (glyph->advance.x >> 6) * 1;
        locY += (glyph->advance.y >> 6) * 1;
		
	}
	return 1;
}



int opsPlaceStringLCD(GlobalParams* m, int size, const char* inputstr, uint32_t locX, uint32_t locY, uint32_t color, void* mem, int bufwidth, int bufheight, void* fromBuffer) {
	
	locY = bufheight - locY - size; // compensate for negative Y

	if (!*currentFace) {
		return false;
	}

	FT_Set_Pixel_Sizes(*currentFace, 0, size);
	const FT_GlyphSlot glyph = (*currentFace)->glyph;

	for (const char* p = inputstr; *p; p++) {
		
		if (FT_Load_Char((*currentFace), *p, FT_LOAD_RENDER | FT_LOAD_TARGET_LCD) != 0)
			continue;

		const int vx = locX + glyph->bitmap_left;
		const int vy = locY + glyph->bitmap_top;

		for (int y = 0; y < glyph->bitmap.rows; y++) {
			for (int x = 0; x < glyph->bitmap.width / 3; x++) {

				int i = y * glyph->bitmap.pitch + x * 3;

				float cr = glyph->bitmap.buffer[i + 0] / 255.0f;
				float cg = glyph->bitmap.buffer[i + 1] / 255.0f;
				float cb = glyph->bitmap.buffer[i + 2] / 255.0f;

				int ptx = vx + x;
				int pty = bufheight - (vy - y);

				if (ptx < 0 || pty < 0 || ptx >= bufwidth || pty >= bufheight)
					continue;

				uint32_t* dstPtr  = GetMemoryLocation(mem,        ptx, pty, bufwidth, bufheight);
				uint32_t* srcPtr  = GetMemoryLocation(fromBuffer, ptx, pty, bufwidth, bufheight);
				uint32_t dstColor = *srcPtr;

				uint8_t dstR = (dstColor >> 16) & 0xFF;
				uint8_t dstG = (dstColor >> 8)  & 0xFF;
				uint8_t dstB =  dstColor        & 0xFF;
				uint8_t dstA = (dstColor >> 24) & 0xFF;

				uint8_t srcR = (color >> 16) & 0xFF;
				uint8_t srcG = (color >> 8)  & 0xFF;
				uint8_t srcB =  color        & 0xFF;
				uint8_t srcA = (color >> 24) & 0xFF;

				uint8_t outR = (uint8_t)(srcR * cr + dstR * (1.0f - cr));
				uint8_t outG = (uint8_t)(srcG * cg + dstG * (1.0f - cg));
				uint8_t outB = (uint8_t)(srcB * cb + dstB * (1.0f - cb));

				float cov = cr;
				if (cg > cov) cov = cg;
				if (cb > cov) cov = cb;

				uint8_t outA = dstA + (uint8_t)((255 - dstA) * cov * (srcA / 255.0f));

				*dstPtr = (outA << 24) | (outR << 16) | (outG << 8) | outB;
			}
		}

		locX += (glyph->advance.x >> 6);
		locY += (glyph->advance.y >> 6);
	}

	return true;
}

int opsPlaceStringBuffer(GlobalParams* m, int size, const char* inputstr, uint32_t locX, uint32_t locY, uint32_t color, void* mem, int bufwidth, int bufheight, void* fromBuffer) {
	bool state = true;
	if (m->lcd) {
		state = state && opsPlaceStringLCD(m, size, inputstr, locX, locY, color, mem, bufwidth, bufheight, fromBuffer);
	} else {
		state = state && opsPlaceStringGreyscale(m, size, inputstr, locX, locY, color, mem, bufwidth, bufheight, fromBuffer);
	}

	return state;
}

int opsPlaceStringShadowObject(GlobalParams* m, int size, const char* inputstr, uint32_t locX, uint32_t locY, uint32_t color, void* mem, double sigma, int passes) {
	sigma*=3.5;
	int clearance = 10;
	unsigned int approx_textwidth = std::string(inputstr).length()*size;

	unsigned int tempbuffer_width = approx_textwidth+clearance*2;
	unsigned int tempbuffer_height = size+clearance*2;


	size_t memcount = tempbuffer_width*tempbuffer_height;
	size_t memsize = memcount*4;

	uint32_t* temp_buffer = (uint32_t*)malloc(memsize);
	memset(temp_buffer, 0xFF, memsize);

	int e = opsPlaceStringGreyscale(m, size, inputstr, clearance, clearance, 0x000000, temp_buffer, tempbuffer_width, tempbuffer_height, temp_buffer);

	// blur
	// gaussian B previously
	gaussian_blur_real(temp_buffer, temp_buffer, tempbuffer_width, tempbuffer_height, sigma, tempbuffer_width, tempbuffer_height, 0, 0);

	for(int i=0; i<passes; i++) {
		for(int y=0; y<tempbuffer_height; y++) {
			for(int x=0; x<tempbuffer_width; x++) {
				// render temp
				uint32_t putX = locX+x-clearance;
				uint32_t putY = locY+y-clearance;
				if(putX > 0 && putX <= m->width && putY > 0 && putY <= m->height) {
					uint32_t* scrbuf = GetMemoryLocation(mem, putX, putY, m->width, m->height);
					int from = (*GetMemoryLocation(temp_buffer, x, y, tempbuffer_width, tempbuffer_height) >> 8) & 0xFF;
					int factor = 255-from;
					*scrbuf = lerp_u32(*scrbuf, color, factor);
				}
			}
		}
	}

	free(temp_buffer);

	return e;
}
