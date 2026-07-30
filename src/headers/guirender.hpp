#pragma once
#include "ops.hpp"
#include "globalvar.hpp"
#include <algorithm>
#include <iostream>
#include <cmath>
#include <memory>

void DrawGUI(GlobalParams* m);

FT_Face LoadFont(GlobalParams* m, std::string fontA);

void SwitchFont(FT_Face& font);