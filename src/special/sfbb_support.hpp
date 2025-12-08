#pragma once
#include <windows.h>

const char* encodesfbb(const char* out_path, void* imgdata, int imgwidth, int imgheight, int channels);
void* decodesfbb(const char* filepath, int* imgwidth, int* imgheight);