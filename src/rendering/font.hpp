#pragma once
#include <vector>
#include <freetype/freetype.h>
#include <cstdint>
#include <stdio.h>
#include <cmath>
#include "../vendor/stb_image_write.h"
#include <GL/gl.h>

void PlaceStringBuf(FT_Face face, int size0, const char* inputstr, uint32_t locX0, uint32_t locY0, uint32_t color, float uiscale0, bool shadow);