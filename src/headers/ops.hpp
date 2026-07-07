#pragma once
#include <iostream>
#include <algorithm>
#include "globalvar.hpp"
#include <Windows.h>
#include "rendering.hpp"
#include "events.hpp"
#include "../vendor/stb_image.h"
#include "../vendor/stb_image_write.h"

inline void FreeDatac(void*& b) {
	if (b) {
		free(b);
		b = nullptr;
	}
}

#define FreeData(x) \
	FreeDatac((void*&)x)
		

bool DwmDarken(HWND hwnd);
bool DwmExtend(HWND hwnd, int topHeight);

void DeleteTempFiles(GlobalParams* m, std::string folder);
bool DeleteDirectory(const char* directoryPath);

#define IfInMenu(pos, m) \
	((pos.x > m->actmenuX && pos.y > m->actmenuY && pos.x < (m->actmenuX + m->menuSX) && pos.y < (m->actmenuY + m->menuSY)))

#define bottomtoolmacro(mPP, m) ((!(m->drawMenuOffsetY > m->toolheight)) || mPP.y < m->height-m->toolheight)
#define dmguidemacro(mPP, m) (!(m->drawmode && mPP.x > m->dmguide_x && mPP.y > m->dmguide_y && mPP.x <= m->dmguide_x+m->dmguide_sx && mPP.y <= m->dmguide_y+m->dmguide_sy ))
#define extracases(mPP, m) (bottomtoolmacro(mPP, m) && dmguidemacro(mPP, m))
#define IsInImage(mPP, m) \
	((mPP.y > m->toolheight && mPP.x >= m->CoordLeft && mPP.y > m->CoordTop && mPP.x < m->CoordRight && mPP.y < m->CoordBottom) && extracases(mPP, m))

#define IsInSlider(slider)\
    ((mPP.x > ((slider.x+(*slider.parentX))-25) && mPP.x < ((slider.endX+(*slider.parentX))+25)) && (mPP.y > ((slider.y+(*slider.parentY))-7) && mPP.y < ((slider.endY+(*slider.parentY))+7)))

extern GlobalParams* mv;

#define GetMemoryLocation(start, x, y, widthfactor, heightfactor) \
	((uint32_t*)(start) + ((y) * (widthfactor)) + (x))

#define GetMemoryLocationTemplate(start, x, y, widthfactor, heightfactor) \
	 ((( (((y) * (widthfactor)) + (x)) < (widthfactor*heightfactor))&&((y) * (widthfactor)) + (x) > start) ) ? ((start) + ((y) * (widthfactor)) + (x))  : ((start)) ) 

bool isFile(const char* str, const char* suffix);
unsigned char* LoadImageFromResource(int resourceId, int& width, int& height, int& channels);
void Print(GlobalParams* m);
void rotateImage90Degrees(GlobalParams* m);
int GetLocationFromButton(GlobalParams* m, int index);
int GetIndividualButtonPush(GlobalParams* m, int index);
int getXbuttonID(GlobalParams* m, POINT mPos);
void GetCropCoordinates(GlobalParams* m, uint32_t* outDistLeft, uint32_t* outDistRight, uint32_t* outDistTop, uint32_t* outDistBottom);
void GetCropPercentagesFromCursor(GlobalParams* m, int cursorX, int cursorY, float* outX, float* outY);
void ConfirmCrop(GlobalParams* m);
float log_base_1_25(float x);
float roundzoom(float z);
void no_offset(GlobalParams* m);
void autozoom(GlobalParams* m);
void NewZoom(GlobalParams* m, float v, int mouse, bool shouldRoundZoom);
uint32_t InvertCC(uint32_t d, bool should);
void InvertAllColorChannels(uint32_t* buffer, int w, int h);
uint32_t lerp_u32(uint32_t c1, uint32_t c2, uint32_t a);
uint32_t lerp_gc(uint32_t color1, uint32_t color2, float alpha);
void ResizeImageToSize(GlobalParams* m, int width, int height);
double gaussian(double x, double sigma);
void gaussian_blur_real(uint32_t* input_buffer, uint32_t* output_buffer, int lW, int lH, double sigma, uint32_t width, uint32_t height, uint32_t offX, uint32_t offY);

void boxBlurRegion(uint32_t* src, uint32_t* dst, int width, int height, uint32_t kernelSize, int rx, int ry, int rw, int rh);

uint32_t multiplyColors(uint32_t color1, uint32_t color2);
bool CopyImageToClipboard(GlobalParams* m, void* imageData, int width, int height);
bool PasteImageFromClipboard(GlobalParams* m);
uint32_t change_alpha(uint32_t color, uint8_t new_alpha);

bool AutoAdjustLevels(GlobalParams* m, uint32_t* buffer, double sigma);


int opsPlaceStringBuffer(GlobalParams* m, int size, const char* inputstr, uint32_t locX, uint32_t locY, uint32_t color, void* mem, int bufwidth, int bufheight, void* fromBuffer);
int opsPlaceStringShadowObject(GlobalParams* m, int size, const char* inputstr, uint32_t locX, uint32_t locY, uint32_t color, void* mem, double sigma, int passes);