#pragma once
#include <Windows.h>
#include <iostream>
#include <string>
#include "globalvar.hpp"
#include "../special/sfbb_support.hpp"
#include "../special/m45_support.hpp"
#ifdef _SVG
#include "../special/svg_support.hpp"
#endif
#include "../special/irbo_support.hpp"

enum LoadImageResult {
	LI_NotReady,
	LI_Failed,
	LI_Success
};

//void CombineBuffer(GlobalParams* m, uint32_t* first, uint32_t* second, int width, int height, bool invert);
//void FreeCombineBuffer(GlobalParams* m);
bool doIFSave(GlobalParams* m);

std::string ReplaceBitmapAndMetrics(GlobalParams* m, void*& buffer, const char* standardPath, int* w, int* h);
LoadImageResult OpenImageFromPath(GlobalParams* m, std::string kpath, bool isLeftRight);
void PrepareOpenImage(GlobalParams* m);

void ActuallySaveImage(GlobalParams* m, std::string res);
bool PrepareSaveImage(GlobalParams* m);
bool AllocateBlankImage(GlobalParams* m, uint32_t color);