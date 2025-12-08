#pragma once
#include <iostream>
#include <iostream>
#include <string>
#include <cmath>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <windows.h>
#include <algorithm>
#include <stdlib.h>
#include <iostream>
#include "../vendor/stb_image.h"
#include "../vendor/stb_image_resize2.h"
#include "../vendor/stb_image_write.h"
#include <vector>

void* decode_m45(const char* filepath, int* imgwidth, int* imgheight);
bool encodedata(void* idd, uint32_t iw, uint32_t ih, const char* filepath);