#include "renderops.hpp"
#include <gl/gl.h>
#include <cmath>
#include <vector>
#include "font.hpp"

void gaussian_blur_render(GlobalParams* m, int lW, int lH, double sigma, uint32_t offX, uint32_t offY);