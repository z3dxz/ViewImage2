#pragma once
#include "../headers/ops.hpp"
#include "../headers/globalvar.hpp"

// due to opengl renderer, renderer now includes all opengl-bound functions which have the same signature as the old rendering functions
#include "opengl.hpp"

void SwitchSoftwareFont(FT_Face& font);