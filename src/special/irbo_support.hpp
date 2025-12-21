#pragma once
#include <windows.h>
#include "../headers/ops.hpp"
#include "../vendor/stb_image.h"
#include "../vendor/stb_image_resize2.h"
#include "../vendor/stb_image_write.h"

const char* encodeirbo(const char* out_path, void* imgdata, int imgwidth, int imgheight, int channels);
void* decodeirbo(const char* filepath, int* imgwidth, int* imgheight);