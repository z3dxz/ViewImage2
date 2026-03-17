#pragma once
#include "ops.hpp"
#include "globalvar.hpp"
#include <algorithm>
#include <iostream>
#include <cmath>
#include <memory>

FT_Face LoadFont(GlobalParams* m, std::string fontA);

void UpdateBuffer(GlobalParams* m);
void RedrawSurfaceTextDialog(GlobalParams* m);

void RedrawSurface(GlobalParams* m, bool onlyImage = false, bool doesManualClip = false, bool bypassSleep = false, bool clip = false, RECT region = {-1,-1,-1,-1});

void SwitchFont(FT_Face& font);