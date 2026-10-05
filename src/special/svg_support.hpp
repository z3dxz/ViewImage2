#pragma once
#ifdef _SVG

#include "../headers/ops.hpp"

#include <lunasvg.h>
#include <iostream>

void* decodesvg(const char* filepath, int* imgwidth, int* imgheight);

#endif // _SVG