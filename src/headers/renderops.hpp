#pragma once
#include "globalvar.hpp"
#include "ops.hpp"
#include <algorithm>
#include <iostream>
#include <cmath>
#include <memory>
#include <execution>

enum GradientDirection {
	GradientLeftRight,
	GradientTopBottom
};

int PlaceString(GlobalParams* m, int size, const char* inputstr, uint32_t locX, uint32_t locY, uint32_t color);
int PlaceStringShadow(GlobalParams* m, int size, const char* inputstr, uint32_t locX, uint32_t locY, uint32_t color, float sigma, int shadowOffsetX, int shadowOffsetY, int passes, uint32_t shadowColor = 0x000000);

void CircleGenerator(GlobalParams* m, int circleDiameter, int locX, int locY, uint32_t color, bool onlyUnderToolbar);

void drawLine(GlobalParams* m, int startX, int startY, int len, bool horizontal, uint32_t color, float opacity);
void dDrawFilledRectangle(GlobalParams* m, int xloc, int yloc, int width, int height, uint32_t color, float opacity);
void PlaceFromAtlas(GlobalParams* m, void* source, int sourceWidth, int sourceHeight, int sourceX, int sourceY, int destX, int destY, int width, int height, uint32_t color_tint, float opacity);

void PlaceImageNN(GlobalParams* m, int rendertoolbar, void* memory, bool invert, POINT p, bool clip, RECT region);

void PlaceImageBI(GlobalParams* m, int rendertoolbar, void* memory, bool invert, POINT p, bool clip, RECT region);

void Tint(GlobalParams* m);

void boxBlur(GlobalParams* m, uint32_t kernelSize, int mode, int startOffset, int vsize);
void gaussian_blur(GlobalParams* m, int lW, int lH, double sigma, uint32_t offX, uint32_t offY);

void blur_toolbar(GlobalParams* m);