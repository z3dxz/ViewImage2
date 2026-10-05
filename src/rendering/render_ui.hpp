#pragma once
#include "../headers/ops.hpp"
#include "../headers/globalvar.hpp"
#include <algorithm>
#include <iostream>
#include <cmath>
#include <memory>

#define CanRenderToolbarMacro (((!m->fullscreen && m->height >= 250) || m->mpos.y < m->toolheight || m->isMenuState)&&!m->isInCropMode)

void DrawGUI(GlobalParams* m);

void SwitchSoftwareFont(FT_Face& font);